aux_source_directory(. SRC)

if (SRC)
    get_filename_component(TARGET ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    add_library(${TARGET} ${SRC})
endif()
