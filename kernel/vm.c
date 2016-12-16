#define LOG_TAG "vm"
#include <csrtypes.h>
#include <vm.h>
#include <debug.h>

void VmSendDmPrim(void *prim)
{
    LOGW("VmSendDmPrim : %x\n", *(u16*)prim);
}
