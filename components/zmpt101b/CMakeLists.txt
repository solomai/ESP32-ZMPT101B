idf_component_register(
    SRCS "zmpt101b.c"
    INCLUDE_DIRS "."
    REQUIRES esp_adc_cal
    PRIV_REQUIRES "driver"
)