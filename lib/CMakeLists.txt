file(GLOB SUB RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(TARGET in ${SUB})
    if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET})
        add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/include/${TARGET}.h
            COMMAND ${CMAKE_COMMAND} 
            ARGS -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/${TARGET}.h ${PROJECT_SOURCE_DIR}/include/${TARGET}.h
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/${TARGET}.h
            COMMENT "Install " ${PROJECT_SOURCE_DIR}/include/${TARGET}.h
            )
        add_custom_target(${TARGET}.h ALL DEPENDS ${PROJECT_SOURCE_DIR}/include/${TARGET}.h)

        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/${TARGET}_no_ble.h)
            add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/include/${TARGET}_no_ble.h
                COMMAND ${CMAKE_COMMAND} 
                ARGS -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/${TARGET}_no_ble.h ${PROJECT_SOURCE_DIR}/include/${TARGET}_no_ble.h
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/${TARGET}_no_ble.h
                COMMENT "Install " ${PROJECT_SOURCE_DIR}/include/${TARGET}_no_ble.h
                )
            add_custom_target(${TARGET}_no_ble.h ALL DEPENDS ${PROJECT_SOURCE_DIR}/include/${TARGET}_no_ble.h)
        endif()

        add_subdirectory(${TARGET})
    endif()
endforeach()
