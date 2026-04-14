file(GLOB_RECURSE ASTRA_SRCS "astra/*.cpp")

idf_component_register(
    SRCS ${ASTRA_SRCS}
    INCLUDE_DIRS "astra"
    REQUIRES hal_esp32s3
)
