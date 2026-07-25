idf_component_register(SRCS "bms_driver_jbd.c"
                    INCLUDE_DIRS "include"
                    REQUIRES bms_interface esp_driver_uart)