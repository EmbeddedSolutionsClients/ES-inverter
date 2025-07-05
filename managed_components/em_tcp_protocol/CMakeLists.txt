# Copyright (C) 2024 EmbeddedSolutions.pl

idf_component_register(
    SRCS
        "parser.c"
        "receiver.c"
        "serializer.c"
        "sender.c"
    INCLUDE_DIRS
        "include"
    PRIV_INCLUDE_DIRS
        "private"
    REQUIRES
        em_buffer
    PRIV_REQUIRES
        mbedtls
)
