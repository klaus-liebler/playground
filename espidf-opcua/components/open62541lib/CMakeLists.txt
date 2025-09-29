idf_component_register(SRCS "open62541.c"
                    INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format -Wempty-body)
add_compile_definitions("UA_ARCHITECTURE_FREERTOSLWIP")