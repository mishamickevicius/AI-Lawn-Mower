idf_component_register(
    SRCS "src/rpi_connector.c"

    INCLUDE_DIRS "include"

    REQUIRES
        "freertos"
        "driver"
)