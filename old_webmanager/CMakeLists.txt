idf_component_register(
                    SRCS "./cpp/webmanager.cc"
                    INCLUDE_DIRS "./cpp"
                    EMBED_FILES "dist_webui/compressed/app.html.br"
                    EMBED_TXTFILES "certificates/host.pem.crt"
                                    "certificates/host.pem.prvtkey"
                                    "certificates/rootCA.pem.crt"
                    REQUIRES main flatbuffers-23.5.26 esp_https_server app_update common esp_partition esp_timer)