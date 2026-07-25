idf_component_register(SRCS "mqtt_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_manager mqtt
                    EMBED_FILES "ca.crt")