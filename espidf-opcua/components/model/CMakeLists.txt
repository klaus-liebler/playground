idf_component_register(SRCS "DHT22.c" "model.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "driver" "open62541lib")
component_compile_options(-Wno-error=format= -Wno-format)