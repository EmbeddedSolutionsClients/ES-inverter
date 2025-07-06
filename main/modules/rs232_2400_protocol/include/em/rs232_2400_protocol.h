/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef RS232_2400_H_
#define RS232_2400_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// rsp from https://forums.aeva.asn.au/uploads/293/HS_MS_MSX_RS232_Protocol_20140822_after_current_upgrade.pdf
typedef enum {
    EM_RS232_2400_NONE = 0,
    // request commands
    EM_RS232_2400_QPI,      // Protocol ID
    EM_RS232_2400_QID,      // Inverter ID
    EM_RS232_2400_QVFW,     // Firmware Version
    EM_RS232_2400_QVFW2,    // Firmware Version 2
    EM_RS232_2400_QVFW3,    // Firmware Version 3
    EM_RS232_2400_QPIRI,    // Power Rating Information
    EM_RS232_2400_QMD,      // Device model
    EM_RS232_2400_QFLAG,    // Flag Status
    EM_RS232_2400_QPIGS,    // General Status
    EM_RS232_2400_QMOD,     // Device mode
    EM_RS232_2400_QT,       // Time inquiry
    EM_RS232_2400_QET,      // Inquiry total energy
    EM_RS232_2400_QEY,      // Inquiry total energy in the year
    EM_RS232_2400_QEM,      // Inquiry total energy in the month
    EM_RS232_2400_QED,      // Inquiry total energy in the day
    EM_RS232_2400_QEH,      // Inquiry total energy in the hour
    EM_RS232_2400_QGOV,     // The grid output voltage range inquiry
    EM_RS232_2400_QGOF,     // The grid output frequency range inquiry
    EM_RS232_2400_QOPMP,    // The current max output power inquiry
    EM_RS232_2400_QMPPTV,   // The PV input voltage range inquiry for MPPT
    EM_RS232_2400_QPVIPV,   // The PV input voltage range inquiry
    EM_RS232_2400_QLST,     // The LCD sleep time inquiry
    EM_RS232_2400_QTPR,     // The temperature inquiry
    EM_RS232_2400_QDI2,     // The default setting value information
    EM_RS232_2400_QGLTV,    // The grid long time average voltage range inquiry
    EM_RS232_2400_QCHGS,    // Charger status inquiry
    EM_RS232_2400_QDM,      // The model of device inquiry
    EM_RS232_2400_QVFTR,    // The grid information range can be set inquiry
    EM_RS232_2400_QPIHF,    // The historical fault inquiry
    EM_RS232_2400_QPICF,    // The current fault inquiry
    EM_RS232_2400_QBSDV,    // The discharge cut-off voltage inquiry
    EM_RS232_2400_QPRIO,    // The PV energy supply priority
    EM_RS232_2400_QENF,     // Function enable/disable status inquiry
    EM_RS232_2400_QEBGP,    // Feeding power adjust and battery type inquiry
    EM_RS232_2400_QOPF,     // Feed-in power factor inquiry
    EM_RS232_2400_QMDCC,    // Inquire battery Max. discharged current in hybrid mode
    EM_RS232_2400_QPKT,     // AC charging time inquiry
    EM_RS232_2400_QLDT,     // AC output ON/OFF time inquiry
    EM_RS232_2400_QBSDP,     // Battery stop discharge percentage inquiry
    EM_RS232_2400_QPIGS2,   // General Status 2
    EM_RS232_2400_QPIWS,    // Warning Status
    EM_RS232_2400_QDI,      // Default Settings
    EM_RS232_2400_QMCHGCR,  // Max Charging Current
    EM_RS232_2400_QMUCHGCR, // Max Utility Charging Current
    EM_RS232_2400_QBOOT,    // Boot Information
    EM_RS232_2400_QOPM,     // Output Mode
    EM_RS232_2400_QPGS,     // Parallel Information

    //control commands
    EM_RS232_2400_SON,      // Turn of
    EM_RS232_2400_SOF,      // Turn off

    // setting parameters commands
    EM_RS232_2400_PE,       // Parameters enable
    EM_RS232_2400_PD,       // Parameters disable
    EM_RS232_2400_PF,       // Setting control parameter to default value
    EM_RS232_2400_F,         // Set Battery Re-charge Voltage
    EM_RS232_2400_POP,      // Set Device Output Source Priority
    EM_RS232_2400_PBCV,     // Set Battery Re-charge Voltage
    EM_RS232_2400_PBDV,     // Set Battery Re-discharge Voltage
    EM_RS232_2400_PCP,      // Set Device Charger Priority
    EM_RS232_2400_PGR,      // Set Device Grid Working Range
    EM_RS232_2400_PBT,      // Set Battery Type
    EM_RS232_2400_PSDV,     // Set Battery Cut-off Voltage
    EM_RS232_2400_PCVV,     // Set Battery Constant Voltage Charging Voltage
    EM_RS232_2400_PBFT,     // Set Battery Float Charging Voltage
    EM_RS232_2400_DAT,        // Set Date and Time
    EM_RS232_2400_GOLF,       // Set grid output frequency low loss point
    EM_RS232_2400_GOHF,       // Set grid output frequency high loss point
    EM_RS232_2400_GOLV,       // Set grid output voltage low loss point
    EM_RS232_2400_GOHV,       // Set grid output voltage high loss point
    EM_RS232_2400_OPMP,       // Set the max output power
    EM_RS232_2400_MPPTHV,     // Set the PV input high voltage for MPPT
    EM_RS232_2400_MPPTLV,     // Set the PV input low voltage for MPPT
    EM_RS232_2400_PVIPHV,     // Set the upper limit of PV input voltage
    EM_RS232_2400_PVIPLV,     // Set the lowest limit of PV input voltage
    EM_RS232_2400_LST,        // Set LCD sleep time
    EM_RS232_2400_MCHGC,      // Setting max charging current
    EM_RS232_2400_MCHGV,      // Setting floating charging voltage
    EM_RS232_2400_BCHGV,      // Setting bulk charging voltage
    EM_RS232_2400_GLTHV,      // Set the grid long time average voltage high loss point
    EM_RS232_2400_BSDV,       // Setting discharge cut-off voltage
    EM_RS232_2400_DSUBV,      // Setting re-discharge voltage
    EM_RS232_2400_PRIO,       // Set the PV energy supply priority
    EM_RS232_2400_ENF,        // Setting function enable/disable
    EM_RS232_2400_LBF,        // Setting Battery type
    EM_RS232_2400_SOPF,       // Setting feed-in power factor
    EM_RS232_2400_SMDCC,      // Setting battery Max. discharged current in hybrid mode
    EM_RS232_2400_ABGP,       // Setting grid power adjustment
    EM_RS232_2400_PKT,        // Setting AC charge time
    EM_RS232_2400_LDT,        // Setting AC output ON/OFF time
    EM_RS232_2400_BSDP,       // Setting battery stop discharge percentage
    EM_RS232_2400_DMODEL,      // Setting model of device
    EM_RS232_2400_PSPB      // Set Solar Power Balance
} em_rs232_2400_cmd_e;

typedef struct {
    char main_production_type;
    char sub_production_type;
    char va_type;
    char hlv_type;
    char year[3]; // Two digits + null terminator
    char month[3]; // Two digits + null terminator
    char manufacturer_id[6]; // Up to 5 characters + null terminator
} qid_response_t;

typedef struct {
    float max_current;
    float max_voltage;
    uint32_t wait_time;
} qdi2_response_t;

typedef struct {
    float boost_temp;
    float inverter_temp;
    float inner_temp;
} qtpr_response_t;

typedef struct {
    float high_frequency;
    float low_frequency;
} qgof_response_t;

typedef struct {
    float grid_rating_voltage;   // Grid rating voltage (V)
    float grid_rating_frequency; // Grid rating frequency (Hz)
    float grid_rating_current;   // Grid rating current (A)
    float ac_output_rating_voltage; // AC output rating voltage (V)
    float ac_output_rating_current; // AC output rating current (A)
    float per_mppt_rating_current;  // Per MPPT rating current (A)
    float battery_rating_voltage;   // Battery rating voltage (V)
    int mppt_track_number;          // MPPT track number
    int machine_type;               // Machine type (e.g., 00: Grid tie, 01: Off Grid, etc.)
    int topology;                   // Topology (e.g., 0: transformerless, 1: transformer)
} qpiri_response_t;

typedef struct {
    uint16_t presence;
    bool silence_buzzer;                  // Enable/disable silence buzzer or open buzzer
    bool overload_bypass;                 // Enable/Disable overload bypass function
    bool power_saving;                    // Enable/Disable power saving
    bool lcd_default_page_timeout;        // Enable/Disable LCD display escape to default page after 1min timeout
    bool overload_restart;                // Enable/Disable overload restart
    bool over_temperature_restart;        // Enable/Disable over temperature restart
    bool backlight_on;                    // Enable/Disable backlight on
    bool alarm_on_primary_interrupt;      // Enable/Disable alarm on when primary source interrupt
    bool fault_code_record;               // Enable/Disable fault code record
} qflag_response_t;

typedef struct {
    float grid_voltage;          // Grid voltage (V)
    float grid_frequency;        // Grid frequency (Hz)
    float ac_output_voltage;     // AC output voltage (V)
    float ac_output_frequency;   // AC output frequency (Hz)
    int ac_output_active_power;   // AC output power (W)
    int ac_output_appearent_power; // AC output power (W)
    int output_load_percent;     // Output load percentage (%)
    int bus_voltage;         //  BUS voltage (V)
    float battery_voltage;     // P battery voltage (V)
    int battery_charging_current; // P battery charging current (A)
    int battery_discharging_current; // P battery charging current (A)
    float battery_voltage_from_SCC;
    int battery_capacity;        // Battery capacity (%)
    int temperature;       //  temperature (°C)
    float pv_input_current;        // PV input current  (W)
    float pv_input_voltage;    // PV input voltage  (V)
    int pv_input_power;     // PV input power (W)
    uint16_t device_status_1;      // Device status (binary flags)
    uint8_t battery_offset;
    uint8_t EEPROM_version;
    uint8_t device_status_2;      // Device status (binary flags)
} qpigs_response_t;

typedef struct {
    int high_voltage;
    int low_voltage;
} hilo_voltage_response_t;

typedef struct {
    float charging_current;
    float floating_voltage;
    float max_current;
    float bulk_voltage;
} qchgs_response_t;

size_t rs232_2400_rsp_parse(uint16_t *rq_id, const uint8_t* data_in, size_t len_in, void* output, uint16_t *err);
uint8_t* rs232_2400_serialize(uint16_t cmd, const uint8_t* payload, size_t payload_len, size_t* out_len);
uint16_t crc16_xmodem(const void *buf, int len);

#endif /* RS232_2400_H_ */
