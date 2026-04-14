file(GLOB U8G2_SRCS "u8g2/*.c")

idf_component_register(
    SRCS
        "hal_esp32s3.cpp"
        "hal_oled.cpp"
        "hal_key.cpp"
        "hal/hal.cpp"
        ${U8G2_SRCS}
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer freertos
)

# u8g2 is plain C — suppress warnings in those translation units
set_source_files_properties(${U8G2_SRCS} PROPERTIES COMPILE_FLAGS "-w")
