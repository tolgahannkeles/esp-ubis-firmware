idf_component_register(SRCS "bms_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES bms_interface bms_driver_jbd bms_driver_daly nvs_manager led_manager)