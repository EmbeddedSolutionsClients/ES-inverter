# Copyright (C) 2024 EmbeddedSolutions.pl

idf_component_register(
    SRCS
        "slip.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        em_buffer
)
