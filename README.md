# README #

This README has been created based on:
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#manual-installation

## Setting up the project (Linux oriented) / ESP-IDF Manual installation ##

### Expected project architecture ###
```bash
# Make it easy for clang/InteliSense/fullci and other tools to recursively find all of the files
esp-idf
    - esp-idf SDK
    - esp-idf toolchain
    - es-energy_plug
```

### Install Prerequisites ###
```bash
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

###  Clone the ESP-IDF Repository and set up the tools ###
```bash
git clone -b v5.4 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
export IDF_TOOLS_PATH="$PWD/esp_toolchain"
./install.sh esp32c3
```
### Set up the Environment Variables ###
```bash
cd ~
sudo nano .bashrc
# ESP-IDF-PATH is a PATH to the esp-idf directory created in previous step
alias get_idf='. <ESP-IDF-PATH>/export.sh'
source .bashrc
cd esp-idf
# Brougt up the IDF CLI
get_idf
```

### Get the repository ###
```bash
cd esp-idf
git clone git@github.com:EmbeddedSolutionsClients/ES-energy_plug.git es-energy_plug
cd es-energy_plug
```

### Troubleshooting ###
Some of the Python versions are not supported by the ESP-IDF.
3.10.12 seems to be working without an issue.
If you get an error pointing that 'lzma module not found" it might mean that you have unsupported Python version.

## Building and Flashing
Application is compatible with __*dkm1*__, __*ep_v3*__ called in the rest of the readme as `hw`.
__*esp32c3_dkm1*__ is default hw which means that the first call of pristine build without `-HW=<hw>` builds firmware for
__*esp32c3_dkm1*__. Specify the `-HW=<hw>` for any other hw.

### Pristine build ###
``` bash
sudo rm sdkconfig
idf.py fullclean build -HW=<hw>
```
### Rebuild ###
```bash
idf.py build
```
### Flashing ###
```bash
idf.py flash
```
### Monitoring / Logging ###
```bash
idf.py monitor
```
### Coredump debugging (Read from FLASH directly) ###
```bash
# Prints coredump summary
idf.py coredump-info -p <USB port: /dev/ttyACM1>
# Starts GDB session
idf.py coredump-debug -p <USB port: /dev/ttyACM1>
```
### Coredump debugging (Read from file) ###
```bash
# Prints coredump summary
idf.py coredump-info -c <coredump_file>.elf
# Starts GDB session
idf.py coredump-debug -c <coredump_file>.elf
```
