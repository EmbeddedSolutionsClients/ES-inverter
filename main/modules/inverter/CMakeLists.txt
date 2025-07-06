# Copyright (C) 2025 EmbeddedSolutions.pl

target_sources(${COMPONENT_LIB} PRIVATE)
target_sources(${COMPONENT_LIB} PRIVATE "inverter.c")
target_sources(${COMPONENT_LIB} PRIVATE "msg_handlers.c")

target_include_directories(${COMPONENT_LIB} PRIVATE "include")
target_include_directories(${COMPONENT_LIB} PRIVATE "private")
