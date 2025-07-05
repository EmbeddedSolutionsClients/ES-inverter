# Copyright (C) 2024 EmbeddedSolutions.pl

idf_component_register(
    SRCS
        "tcp_client.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_event
        em_buffer
    PRIV_REQUIRES
        em_scheduler
        em_wifi
        esp_netif
        esp_wifi
        nvs_flash
        tcp_transport
    EMBED_TXTFILES
        ${project_dir}/cert/server_cert.pem
)
