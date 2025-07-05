# Copyright (C) 2024 EmbeddedSolutions.pl

target_sources(${COMPONENT_LIB} PRIVATE "sys_evt.c")
target_sources(${COMPONENT_LIB} PRIVATE "led_scene_evt.c")

target_include_directories(${COMPONENT_LIB} PRIVATE "include")
