file(GLOB_RECURSE SIXBACK_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

idf_component_register(
    SRCS ${SIXBACK_SOURCES}
    INCLUDE_DIRS "."
    REQUIRES framework-arduinoespressif32 microlink
)
