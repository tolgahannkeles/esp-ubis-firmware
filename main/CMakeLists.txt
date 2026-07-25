idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_manager network_manager web_server bms_manager mqtt_manager app_task button_manager led_manager)