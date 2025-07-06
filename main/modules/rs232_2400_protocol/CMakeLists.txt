# Copyright (C) 2025 EmbeddedSolutions.pl

target_sources(${COMPONENT_LIB} PRIVATE)
target_sources(${COMPONENT_LIB} PRIVATE "rs232_2400_protocol.c")

target_include_directories(${COMPONENT_LIB} PRIVATE "include")
target_include_directories(${COMPONENT_LIB} PRIVATE "private")
