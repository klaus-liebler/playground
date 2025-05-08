idf_component_register(SRCS "sml_attention_response.c" "sml_boolean.c" "sml_close_request.c" "sml_close_response.c" "sml_crc16.c" "sml_file.c" "sml_get_list_request.c" "sml_get_list_response.c" "sml_get_proc_parameter_request.c" "sml_get_proc_parameter_response.c" "sml_get_profile_list_request.c" "sml_get_profile_list_response.c" "sml_get_profile_pack_request.c" "sml_get_profile_pack_response.c" "sml_list.c" "sml_message.c" "sml_number.c" "sml_octet_string.c" "sml_open_request.c" "sml_open_response.c" "sml_set_proc_parameter_request.c" "sml_shared.c" "sml_status.c" "sml_time.c" "sml_transport.c" "sml_tree.c" "sml_value.c"   
                    INCLUDE_DIRS "include"
                    REQUIRES "driver")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=pointer-sign)
target_compile_definitions(${COMPONENT_LIB} PRIVATE SML_NO_UUID_LIB)