/****************************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    gatt_connect.c        

DESCRIPTION
    Functions for the connection.
        GattConnectRequest() ->
        GATT_CONNECT_CFM_T <-

NOTES

*/

/***************************************************************************
    Header Files
*/

#include "gatt.h"
#include "gatt_private.h"

#include <bdaddr.h>
#include <vm.h>

/****************************************************************************
NAME
    GattConnectRequest

DESCRIPTION
    API Function called by application.

RETURN

*/
void GattConnectRequest(
        Task                    theAppTask,
        const typed_bdaddr      *taddr,
        gatt_connection_type    conn_type,
        bool                    conn_timeout
        )
{
    MAKE_GATT_MESSAGE(GATT_INTERNAL_CONNECT_REQ);
    message->theAppTask = theAppTask;

    if (taddr)
        message->taddr = *taddr;
    else
        BdaddrTypedSetEmpty(&message->taddr);

    message->connection = conn_type;
    message->connection_timeout = conn_timeout;
    MessageSend(gattGetTask(), GATT_INTERNAL_CONNECT_REQ, message);
}

/****************************************************************************
NAME
    gattConvertL2capConnectStatus

DESCRIPTION
    Static. Convert L2CAP connection result to GATT status

RETURNS
    GATT status code
*/
static gatt_status_t gattConvertL2capConnectStatus(l2ca_conn_result_t result)
{
    switch(result)
    {
        case L2CA_CONNECT_SUCCESS:
            return gatt_status_success;
        break;
        case L2CA_CONNECT_REJ_PSM:
            return gatt_status_rej_psm;
        break;
        case L2CA_CONNECT_KEY_MISSING:
            return gatt_status_key_missing;
        break;
        case L2CA_CONNECT_TIMEOUT:
            return gatt_status_connection_timeout;
        break;
        case L2CA_CONNECT_INITIATING:
            return gatt_status_initialising;
        break;
        case L2CA_CONNECT_RETRYING:
            return gatt_status_retrying;
        break;
        case L2CA_CONNECT_PEER_ABORTED:
            return gatt_status_peer_aborted;
        break;
        case L2CA_CONNECT_LINK_LOSS:
            return gatt_status_link_loss;
        break;
        default:
            return gatt_status_failure;
    }
}

/****************************************************************************
NAME
    gattSendConnectCfm

DESCRIPTION
    Static. Send the GATT_CONNECT_CFM message.

RETURNS

*/
static void gattSendConnectionCfm(
    Task task, 
    gatt_status_t status, 
    typed_bdaddr *taddr, 
    u16 flags, 
    u16 mtu, 
    u16 cid
    )
{
    MAKE_GATT_MESSAGE(GATT_CONNECT_CFM);

    message->status = status;

    if (taddr)
        message->taddr = *taddr;
    else
        BdaddrTypedSetEmpty(&message->taddr);

    message->flags  = flags;
    message->cid    = cid;
    message->mtu    = mtu;

    MessageSend(task, GATT_CONNECT_CFM, message);
}

/****************************************************************************
NAME
    gattHandleInternalConnectReq

DESCRIPTION
    Handle the internal connection request. Queue message if a connection is 
    already being processed.

RETURN

*/
void gattHandleInternalConnectReq(
    gattState *theGatt, 
    GATT_INTERNAL_CONNECT_REQ_T *req
    )
{
    /* Reject this if there are already the maximum number of ATT connections.*/
    if (gattMaxConnections())
    {
        gattSendConnectionCfm(
            req->theAppTask,
            gatt_status_max_connections,
            0,  /* TYPED_BD_ADDR */
            0,  /* flags */
            0, /* mtu */
            0  /* cid */
            );
    }
    /* Check the scenario state - proceed if there is no connect_req already
     * in progress.
     */
    else if (!theGatt->conn_req_task)
    {
        MAKE_ATT_PRIM(ATT_CONNECT_REQ);

        /* Set the scenario state. */
        theGatt->conn_req_task = req->theAppTask;

        BdaddrConvertTypedVmToBluestack(&prim->addrt, &req->taddr);

        switch(req->connection)
        {
            case gatt_connection_bredr_master:
                prim->flags = 
                    L2CA_CONFLAG_ENUM(L2CA_CONNECTION_BR_EDR);
                break;
            case gatt_connection_ble_master_directed:
                prim->flags = 
                    L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_MASTER_DIRECTED);
                break;
            case gatt_connection_ble_master_whitelist:
                prim->flags =
                    L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_MASTER_WHITELIST);
                break;
            case gatt_connection_ble_slave_directed_high_duty:
                prim->flags =
                    L2CA_CONFLAG_ENUM(
                            L2CA_CONNECTION_LE_SLAVE_DIRECTED_HIGH_DUTY
                            );
                break;
            case gatt_connection_ble_slave_whitelist:
                prim->flags = 
                    L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_SLAVE_WHITELIST);
                break;
            case gatt_connection_ble_slave_undirected:
                prim->flags = 
                    L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_SLAVE_UNDIRECTED);
                break;
            case gatt_connection_ble_slave_directed_low_duty:
                prim->flags =
                    L2CA_CONFLAG_ENUM(
                            L2CA_CONNECTION_LE_SLAVE_DIRECTED_LOW_DUTY
                            );
                break;
        }

        if (req->connection_timeout)
            prim->flags |= L2CA_CONFLAG_PAGE_TIMEOUT;

        VmSendAttPrim(prim);
    }
    /* Otherwise, queue this request to try again later. */
    else
    {
        MAKE_GATT_MESSAGE(GATT_INTERNAL_CONNECT_REQ);
        
        *(message) = *(req);
        
        MessageSendConditionallyOnTask(
            gattGetTask(),
            GATT_INTERNAL_CONNECT_REQ,
            message,
            &theGatt->conn_req_task
            );
    }
}            

/****************************************************************************
NAME
    gattHandleAttConnectCfm

DESCRIPTION
    Handles the ATT_CONNECT_CFM from BlueStack. Pass on the INITIATING state
    so that the app can 'disconnect' the CID before the connection is 
    established.


    Also, unless prepared, an ATT_CONNECT_CFM can be received for an ATT over
    BR/EDR slave connection and the GATT library won't know what to do with 
    it. In that case Panic in debug and ignore in default lib version.

RETURNS

*/
void gattHandleAttConnectCfm(gattState *theGatt, ATT_CONNECT_CFM_T *cfm)
{

    /* PENDING is received for a master BR/EDR connection, after INITIALISING
     * has already been received, so just ignore it.
     */
    if (L2CA_CONNECT_PENDING != cfm->result)
    {
        cid_map_t *conn;
        gatt_status_t status;
        Task theAppTask = theGatt->conn_req_task;
        typed_bdaddr taddr;

        BdaddrConvertTypedBluestackToVm(&taddr, &cfm->addrt);

        switch (cfm->result)
        {
            /* INITIATING is received for any BLE connection. This is when the
             * CID for the connection is known and can be passed to the app.
             */
            case L2CA_CONNECT_INITIATING:
                if ( !gattCidIsValid(cfm->cid) )
                    gattAddCid(cfm->cid, theGatt->conn_req_task);

                gattLockCid(cfm->cid, gatt_ms_connect_req);
                status = gatt_status_initialising;
                break;

            case L2CA_CONNECT_SUCCESS:
                /* By the time the SUCCESS is received, the cid & task should 
                 * already be registered with GATT.
                 */
                conn = PanicNull(gattFindConn(cfm->cid));
                theAppTask = conn->task;

                /* Clear the connection scenario lock for this cid.  */
                gattUnlockCid(cfm->cid);

                status = gatt_status_success;
                conn->mtu = cfm->mtu;
                theGatt->conn_req_task = 0;

                if (L2CA_CONFLAG_IS_LE(cfm->flags))
                    conn->bredr = FALSE;
                else
                    conn->bredr = TRUE;

                break;


            default:
                if ( (conn = gattFindConn(cfm->cid)) )
                    theAppTask = conn->task;

                /* If we are responding to a BR/EDR connect and it failed
                 * Don't clear the con_req_task, as an LE connection could
                 * still be coming in.
                 */
                if (!conn || conn->data.scenario != gatt_ms_connect_rsp)
                    theGatt->conn_req_task = 0;

                status = gattConvertL2capConnectStatus(cfm->result);
                gattDeleteCid(cfm->cid);
                
                GATT_DEBUG_INFO((
                            "gattHandleAttConnectCfm: result 0x%x\n", 
                            cfm->result
                            ));
                break;
        }

        /* theAppTask can be 0, if max BLE connections have been made. Send
         * cfm to task that initialised GATT instead.
         */
        if (!theAppTask)
            theAppTask = gattGetAppTask();

        gattSendConnectionCfm(
                theAppTask,
                status,
                &taddr,
                cfm->flags,
                cfm->mtu,
                cfm->cid
                );
    }
}
/****************************************************************************
NAME
    gattHandleAttConnectInd

DESCRIPTION
    Handles the ATT_CONNECT_IND for initiating a GATT connection  
    from BlueStack. Pass it straight through, no need to store state.
    
    The CID is locked until application responds to GATT_CONNECT_IND
    to prevent race condition when using BLE.

RETURNS

*/
void gattHandleAttConnectInd(ATT_CONNECT_IND_T *ind)
{
    MAKE_GATT_MESSAGE(GATT_CONNECT_IND);

    BdaddrConvertTypedBluestackToVm(&message->taddr, &ind->addrt);
    message->flags  = ind->flags;
    message->cid    = ind->cid;
    message->mtu    = ind->mtu;

    /* If BLE map cid to apptask, as there wont be a connect CFM */
    if ( L2CA_CONFLAG_IS_LE(ind->flags) )
    {
        /* Map the CID to the AppTask. Fails if max connections have been
         * made, in which case - disconnect. 
         */
        if ( gattAddCid(ind->cid, gattGetAppTask()) ) 
        {
            /* Lock the CID so that the App can queue messages to send. */
            gattLockCid(ind->cid, gatt_ms_ble_connect_rsp);
        }
        else
        {
            /* Failed to map CID send negative connect response */
            GattConnectResponse(gattGetAppTask(), ind->cid, ind->flags, FALSE);
            return;
        }
    }

    MessageSend(gattGetAppTask(), GATT_CONNECT_IND, message);
}

/****************************************************************************
NAME
    GattConnectResponse

DESCRIPTION
    Response from app to the GATT_CONNECT_IND_T message.

RETURNS

*/
void GattConnectResponse(
        Task theAppTask,
        u16 cid,
        u16 flags,
        bool accept
        )
{
 
    /* Different handling for BLE or BR/EDR */
    if ( L2CA_CONFLAG_IS_LE(flags) )
    {
        if (!accept)
            GattDisconnectRequest(cid);
        else /* Clear the cid locked on Connect Ind.  */
            gattUnlockCid(cid);
    }
    else /* BR/EDR connection */
    {
        cid_map_t *conn;
        
        MAKE_ATT_PRIM(ATT_CONNECT_RSP);

        /* default is to reject due to lack of resource.*/
        prim->response = L2CA_CONNECT_REJ_RESOURCES;
        prim->cid = cid;

        /* Add the CID and AppTask to the GATT cid/Task map unless 
         * max connections have been made. Has to be done even for
         * rejection as there will be a CFM message.
         */
        if ( (conn = gattAddCid( cid, theAppTask)) )
        {
            /* Lock the CID so that the App can queue messages to send
             * as soon as the connection is completed.
             */
            gattLockCid(cid, gatt_ms_connect_rsp);
            conn->bredr = TRUE;

            /* If the application is accepting the connection... */
            if (accept)
            { 
                prim->response = L2CA_CONNECT_SUCCESS;
            }
        }
        else
            /* Send cfm to indicate why the connection failed. */
        {
            tp_bdaddr tpaddr;

            /* Incase the cid does not return a valid address. */
            typed_bdaddr *p_taddr = 0;

            if ( (VmGetBdAddrtFromCid(cid, &tpaddr)) )
                p_taddr = &tpaddr.taddr;

            gattSendConnectionCfm(
                    theAppTask,
                    gatt_status_max_connections,
                    p_taddr,
                    0,
                    0,
                    cid
                    );
        }
        VmSendAttPrim(prim);
    }
}

