idf_component_register(SRCS "network_manager.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_wifi esp_http_server nvs_manager lwip)