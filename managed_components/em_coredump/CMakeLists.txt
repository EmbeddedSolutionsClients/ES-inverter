# Copyright (C) 2024 EmbeddedSolutions.pl

idf_component_register(
    SRCS
      "coredump.c"
    INCLUDE_DIRS
      "include"
    PRIV_REQUIRES
      espcoredump
      spi_flash
      nvs_flash
)
