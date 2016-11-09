aux_source_directory(. SRC)

if (SRC)
    get_filename_component(TARGET ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    add_executable(${TARGET} ${SRC})
    target_link_libraries(${TARGET}
        hfp
        display_example_plugin
        display
        avrcp 
        power 
        connection
        bdaddr 
        region 
        service 
        codec 
        audio
        csr_voice_prompts_plugin 
        a2dp 
        csr_common_example_plugin 
        obex 
        sdp_parse 
        md5 
        usb_device_class 
        spp_common 
        spps 
        sppc 
        pblock 
        audio_plugin_common 
        csr_dut_audio_plugin 
        pio_common
        pbapc
        mapc
        fm_rx_plugin
        fm_rx_api
        swat
        csr_i2s_audio_plugin
        gatt
        gatt_imm_alert_client
        gatt_apple_notification_client
        gatt_battery_client
        gatt_device_info_client
        gatt_hid_client
        byte_utils
        gatt_manager
        gatt_battery_server
        gatt_server
        gatt_gap_server
        gatt_transmit_power_server
        gatt_imm_alert_server
        gatt_link_loss_server
        csr_multi_channel_plugin
        gatt_client
        gatt_scan_params_client
        kernel
        )
endif()
