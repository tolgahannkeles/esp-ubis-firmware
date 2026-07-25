idf_component_register(SRCS "web_server.c"
                    INCLUDE_DIRS "include"
                    EMBED_FILES "frontend/index.html"
                    REQUIRES esp_http_server network_manager led_manager)