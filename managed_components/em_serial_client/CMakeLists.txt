# Copyright (C) 2025 EmbeddedSolutions.pl

idf_component_register(
    SRCS
        "serial_client.c"
        "tx.c"
        "rx.c"
        "task.c"
    INCLUDE_DIRS
        "include"
        "private"
    REQUIRES
      em_ringbuf
      em_uart
)
