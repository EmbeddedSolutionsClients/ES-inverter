#!/bin/bash
# Copyright (C) 2025 EmbeddedSolutions.pl

esptool.py --chip ESP32-C3 merge_bin -o build/merged_es-energy_plug.bin --flash_mode dio --flash_size 4MB 0x0 build/bootloader/bootloader.bin 0xC000 build/partition_table/partition-table.bin 0x13000 build/ota_data_initial.bin 0x20000 build/es-energy_plug.bin
