# Copyright (C) 2024 EmbeddedSolutions.pl

idf_component_register(
    SRCS
        "time.c"
				"math.c"
				"device.c"
    INCLUDE_DIRS
        "include"
				"private"
    REQUIRES
    PRIV_REQUIRES
        efuse
)
