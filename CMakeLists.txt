# Copyright (C) 2024 EmbeddedSolutions.pl

cmake_minimum_required(VERSION 3.16)

# Out of tree components directory - uncomment when custom component added in /components folder
# set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/components)

# sdkconfig.defaults has to be set first to not be overwritten by next appended files
list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults")

if(NOT DEFINED HW)
  set(HW dkm1)
endif()

set(BOARD_SDKCONFIG ${CMAKE_SOURCE_DIR}/boards/sdkconfig.${HW})

if(DEFINED RELEASE)
  list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.release")
else()
  list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.debug")
endif()

if(NOT DEFINED ENV)
  set(ENV prod)
endif()

set(ENV_CONFIG ${CMAKE_SOURCE_DIR}/environment/sdkconfig.${ENV})

list(APPEND SDKCONFIG_DEFAULTS ${BOARD_SDKCONFIG} ${ENV_CONFIG})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(es-energy_plug)

message(" |===================================")
if(DEFINED RELEASE)
  message(" |-------- RELEASE")
else()
  message(" |-------- debug")
endif()
message(" |-------- HW=${HW}")
message(" |-------- ENV=${ENV}")
message(" |-------- ver=${CONFIG_APP_PROJECT_VER}")
message(" |===================================")
