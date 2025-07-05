# Copyright (C) 2024 EmbeddedSolutions.pl

idf_build_get_property(project_dir PROJECT_DIR)

idf_component_register(
    SRCS
        "http_ota.c"
    INCLUDE_DIRS
        "include"
    REQUIRES esp_netif
    PRIV_REQUIRES
        esp_http_client
        esp_https_ota
        app_update
        esp_event
        espressif__esp_encrypted_img
    EMBED_TXTFILES
        ${project_dir}/cert/encryption_key.pem
)

create_esp_enc_img(${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    ${project_dir}/cert/encryption_key.pem ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}_secure.bin app)
