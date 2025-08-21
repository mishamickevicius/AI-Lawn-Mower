idf_component_register(
    SRCS "src/accelerometer.c"

    INCLUDE_DIRS "include"

    REQUIRES 
        "freertos"
        "driver"
)