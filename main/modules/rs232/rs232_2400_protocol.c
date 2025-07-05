/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/rs232_2400_protocol.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "RS232_2400"

typedef enum {
  PARSE_STATUS_UNKNOWN = 0,
  PARSE_STATUS_OK = 1,
  PARSE_STATUS_NO_PACKET = 2,
  PARSE_STATUS_INVALID_CRC = 3,
  PARSE_STATUS_RQ_REJECTED = 4,
  PARSE_STATUS_RSP_UNEXPECTED = 5,
  PARSE_STATUS_PACKET_MALFORMED = 6,
} parse_status_t;

static int crc_err_cnt = 0;
static int valid_cnt = 0;
static int packet_cnt = 0;

static uint16_t parse_uint24(const uint8_t *read_ptr, size_t len, uint32_t *rsp)
{
  assert(read_ptr != NULL);
  assert(rsp != NULL);
  const size_t msg_len = 3;

  if (len != msg_len) {
    return PARSE_STATUS_PACKET_MALFORMED;
  }

  *rsp = read_ptr[0];
  *rsp |= read_ptr[1] << 8;
  *rsp |= read_ptr[2] << 16;

  return PARSE_STATUS_OK;
}

static uint16_t parse_uint16(const uint8_t *read_ptr, size_t len, uint16_t *rsp)
{
  assert(read_ptr != NULL);
  assert(rsp != NULL);
  const size_t msg_len = 2;

  if (len != msg_len) {
    return PARSE_STATUS_PACKET_MALFORMED;
  }

  *rsp = read_ptr[0];
  *rsp |= read_ptr[1] << 8;

  return PARSE_STATUS_OK;
}


typedef enum {
    EM_INV_RS232_2400_FAULT_FAN_LOCKED = 1,
    EM_INV_RS232_2400_FAULT_OVER_TEMPERATURE = 2,
    EM_INV_RS232_2400_FAULT_BATTERY_VOLTAGE_HIGH = 3,
    EM_INV_RS232_2400_FAULT_BATTERY_VOLTAGE_LOW = 4,
    EM_INV_RS232_2400_FAULT_OUTPUT_SHORT_OR_OVER_TEMP = 5,
    EM_INV_RS232_2400_FAULT_OUTPUT_VOLTAGE_HIGH = 6,
    EM_INV_RS232_2400_FAULT_OVERLOAD_TIMEOUT = 7,
    EM_INV_RS232_2400_FAULT_BUS_VOLTAGE_HIGH = 8,
    EM_INV_RS232_2400_FAULT_BUS_SOFT_START_FAILED = 9,
    EM_INV_RS232_2400_FAULT_MAIN_RELAY_FAILED = 11,
    EM_INV_RS232_2400_FAULT_OVER_CURRENT_INVERTER = 51,
    EM_INV_RS232_2400_FAULT_INVERTER_SOFT_START_FAILED = 53,
    EM_INV_RS232_2400_FAULT_SELF_TEST_FAILED = 54,
    EM_INV_RS232_2400_FAULT_OVER_DC_VOLTAGE_OUTPUT = 55,
    EM_INV_RS232_2400_FAULT_BATTERY_CONNECTION_OPEN = 56,
    EM_INV_RS232_2400_FAULT_CURRENT_SENSOR_FAILED = 57,
    EM_INV_RS232_2400_FAULT_OUTPUT_VOLTAGE_LOW = 58,
    EM_INV_RS232_2400_FAULT_INVERTER_NEGATIVE_POWER = 60,
    EM_INV_RS232_2400_FAULT_PARALLEL_VERSION_DIFFERENT = 71,
    EM_INV_RS232_2400_FAULT_OUTPUT_CIRCUIT_FAILED = 72,
    EM_INV_RS232_2400_FAULT_CAN_COMMUNICATION_FAILED = 80,
    EM_INV_RS232_2400_FAULT_PARALLEL_HOST_LINE_LOST = 81,
    EM_INV_RS232_2400_FAULT_PARALLEL_SYNC_SIGNAL_LOST = 82,
    EM_INV_RS232_2400_FAULT_PARALLEL_BATTERY_VOLTAGE_DIFF = 83,
    EM_INV_RS232_2400_FAULT_PARALLEL_LINE_VOLTAGE_FREQ_DIFF = 84,
    EM_INV_RS232_2400_FAULT_PARALLEL_INPUT_CURRENT_UNBALANCED = 85,
    EM_INV_RS232_2400_FAULT_PARALLEL_OUTPUT_SETTING_DIFF = 86
} em_inv_rs232_2400_fault_code_t;

static int parse_rsp_is_nak(const uint8_t* read_ptr, size_t len) {
    assert(read_ptr != NULL);

    if (len < 3) { // Minimum length for "NAK"
        return -1;
    }

    return strncmp((const char*)read_ptr, "NAK", 3);
}

static int parse_rsp_qpi(const uint8_t* read_ptr, size_t len, uint32_t* protocol_id) {
    assert(read_ptr != NULL);
    assert(protocol_id != NULL);

    if (len < 4) { // Minimum length for "PI<NN>"
        return -1;
    }

    if (strncmp((const char*)read_ptr, "PI", 2) != 0) {
        return -2; // Malformed, expected "PI"
    }

    char protocol_id_str[2] = {0}; // To store the protocol ID as a string
    memcpy(protocol_id_str, &read_ptr[2], 2); // Extract the protocol ID
    *protocol_id = (uint32_t)atoi(protocol_id_str); // Convert to integer

    if (*protocol_id > 99) {
        return -3; // Protocol ID out of range
    }

    return 0;
}

static int parse_rsp_qid(const uint8_t* read_ptr, size_t len, qid_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);

    const size_t msg_len = 14; // Minimum length for a valid QID response "ABCDEEFFGXXXXX"

    if (len != msg_len) {
        return -1; // Malformed
    }

    response->main_production_type = read_ptr[1];
    response->sub_production_type = read_ptr[2];
    response->va_type = read_ptr[3];
    response->hlv_type = read_ptr[4];

    snprintf(response->year, sizeof(response->year), "%c%c", read_ptr[5], read_ptr[6]);
    snprintf(response->month, sizeof(response->month), "%c%c", read_ptr[7], read_ptr[8]);
    snprintf(response->manufacturer_id, sizeof(response->manufacturer_id), "%.*s", 5, &read_ptr[9]);
    return 0;
}

static int parse_rsp_qvfw(const uint8_t* read_ptr, size_t len, uint64_t* fw_ver) {
    assert(read_ptr != NULL);
    assert(fw_ver != NULL);
    const size_t msg_len = 14; // Minimum length for a valid QVFW response "VERFW:NNNNN.NN"

    *fw_ver = 0;

    if (len != msg_len) {
        return -1; // Malformed
    }

    // exclude VERFW: and parse NNNNN.NN
    char version_str[10] = {0};
    memcpy(version_str, &read_ptr[6], 5); // copy in NNNNN
    *fw_ver = strtoull(version_str, NULL, 10); // Convert to integer
    *fw_ver <<= 8;

    memcpy(version_str+5, &read_ptr[12], 2); // copy in NN
    *fw_ver |= (uint8_t)strtoull(version_str+5, NULL, 10); // Convert to integer

    return 0;
}

static int parse_rsp_qmd(const uint8_t* read_ptr, size_t len, char* model) {
    assert(read_ptr != NULL);
    assert(model != NULL);

    const size_t min_msg_len = 15; // (TTTTTTTTTTTTTTT WWWWWWW KK P/P MMM NNN RR BB.B

    if (len < min_msg_len) {
        return -1; // Malformed
    }

    memcpy(model, &read_ptr[1], 15); // Copy the model string
    model[15] = '\0'; // Null-terminate the string
    return 0;
}

static int parse_rsp_qpiri(const uint8_t* read_ptr, size_t len, qpiri_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t msg_len = 46; // Length for a valid QPIRI response BBB.B FF.F III.I EEE.E DDD.D AA.A GGG.G R MM T

    if (len != msg_len) {
        return -1; // Malformed
    }

    const char* data = (const char*)read_ptr;
    int parsed = sscanf(data,
        "%5f %4f %5f %5f %5f %4f %5f %1d %2d %1d",
        &response->grid_rating_voltage,
        &response->grid_rating_frequency,
        &response->grid_rating_current,
        &response->ac_output_rating_voltage,
        &response->ac_output_rating_current,
        &response->per_mppt_rating_current,
        &response->battery_rating_voltage,
        &response->mppt_track_number,
        &response->machine_type,
        &response->topology);

    if (parsed != 10) {
        return -2;
    }

    return 0;
}

static int parse_rsp_qpigs(const uint8_t* read_ptr, size_t len, qpigs_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t min_msg_len = 106;

    if (len < min_msg_len) { // Minimum length for a valid QPIGS response
        return -1; // Malformed
    }

    uint32_t status1 = 0;
    uint32_t status2 = 0;

/* 228.2 49.9 220.9 50.0 0132 0084 003 397 26.60 000 100 0031 00.1 050.0 00.00 00004 status1=00010/110 00 00 00009 010 */
/* 237.3 50.0 220.4 50.0 0242 0067 005 455 27.80 000 100 0029 01.7 088.6 00.00 00000 status1=11010/110 00 00 00157 110 */
/* STATUS 1 b7b6b5...b0
b7: add SBU priority version, 1:yes,0:no
b6: configuration status: 1: Change 0:
unchanged
b5: SCC firmware version 1: Updated 0:
unchanged
b4: Load status: 0: Load off 1:Load on
b3: battery voltage to steady while charging
b2: Charging status( Charging on/off)
b1: Charging status( SCC charging on/off)
b0: Charging status(AC charging on/off)
b2b1b0:
000: Do nothing
110: Charging on with SCC charge on
101: Charging on with AC charge on
111: Charging on with SCC and AC charge on
*/
    int parsed = sscanf((char*)read_ptr,
        "%f %f %f %f %d %d %d %d %f %d %d %d %f %f %f %d %lx %hhd %hhd %d %lx",
        &response->grid_voltage,
        &response->grid_frequency,
        &response->ac_output_voltage,
        &response->ac_output_frequency,
        &response->ac_output_appearent_power,
        &response->ac_output_active_power,
        &response->output_load_percent,
        &response->bus_voltage,
        &response->battery_voltage,
        &response->battery_charging_current,
        &response->battery_capacity,
        &response->temperature,
        &response->pv_input_current,
        &response->pv_input_voltage,
        &response->battery_voltage_from_SCC,
        &response->battery_discharging_current,
        &status1,
        &response->battery_offset,
        &response->EEPROM_version,
        &response->pv_input_power,
        &status2
    );

    

    for (int i = 0; i < 8; i++) {
        response->device_status_1 |= ((status1 >> i * 4) & 0x01) << i;
        response->device_status_2 |= ((status2 >> i * 4) & 0x01) << i;
    }

    if (parsed != 21) {
        ESP_LOGW(TAG, "Parsed %d fields, expected 21", parsed);
    }

    return 0;
}

static int parse_rsp_qflag(const uint8_t* read_ptr, size_t len, qflag_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t msg_len = 11;

    if (len != msg_len) { // "ExxxDxxx"
        return -1; // Malformed
    }

    if (read_ptr[0] != 'E' && read_ptr[0] != 'D') {
        return -2; // Malformed
    }

    bool enable = false;

    for (int i = 0; i < len; i++) {
        if (read_ptr[i] == 'E') {
            enable = true;
        } else if (read_ptr[i] == 'D') {
            enable = false;
        } else {
            switch (read_ptr[i]) {
                case 'A':
                    response->silence_buzzer = enable;
                    response->presence |= 0x01;
                    break;
                case 'B':
                    response->overload_bypass = enable;
                    response->presence |= 0x02;
                    break;
                case 'J':
                    response->power_saving = enable;
                    response->presence |= 0x04;
                    break;
                case 'K':
                    response->lcd_default_page_timeout = enable;
                    response->presence |= 0x08;
                    break;
                case 'U':
                    response->overload_restart = enable;
                    response->presence |= 0x10;
                    break;
                case 'V':
                    response->over_temperature_restart = enable;
                    response->presence |= 0x20;
                    break;
                case 'X':
                    response->backlight_on = enable;
                    response->presence |= 0x40;
                    break;
                case 'Y':
                    response->alarm_on_primary_interrupt = enable;
                    response->presence |= 0x80;
                    break;
                case 'Z':
                    response->fault_code_record = enable;
                    response->presence |= 0x100;
                    break;
                default:
                    return -3; // Unknown flag
            }
        }
    }

    if(response->presence != 0x1FF) {
        return -4; // Missing flag
    }

    return 0;
}

/*
Power On Mode P Power on mode
Standby Mode  S Standby mode
Bypass Mode   Y  Bypass mode
Line Mode     L Line Mode
Battery Mode  B Battery mode
Battery Test  T Battery test mode
Fault Mode    F Fault mode
Shutdown Mode D Shutdown Mode
Grid mode     Ｇ Grid mode
Charge mode   C Charge mode
*/
static int parse_rsp_qmod(const uint8_t* read_ptr, size_t len, uint32_t* mode) {
    assert(read_ptr != NULL);
    assert(mode != NULL);
    const size_t msg_len = 1;

    if (len != msg_len) {
        return -1; // Malformed
    }

    *mode = (uint32_t)read_ptr[0];

    return 0;
}

static int parse_rsp_qgov(const uint8_t* read_ptr, size_t len, hilo_voltage_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t min_msg_len = 11;

    if (len < min_msg_len) { // Minimum length for HHH.H LLL.L
        return -1; // Malformed
    }

    int parsed = sscanf((char*)read_ptr, "%d %d", &response->high_voltage, &response->low_voltage);
    if (parsed != 2) {
        return -2; // Parsing error
    }

    return 0;
}

static int parse_rsp_qgof(const uint8_t* read_ptr, size_t len, qgof_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t min_msg_len = 9;

    if (len < min_msg_len) { // Minimum length for "FF.F GG.G"
        return -1; // Malformed
    }

    int parsed = sscanf((char*)read_ptr, "%f %f", &response->high_frequency, &response->low_frequency);
    if (parsed != 2) {
        return -2; // Parsing error
    }

    return 0;
}

static int parse_rsp_qpiws(const uint8_t* read_ptr, size_t len, uint64_t* warning_flags) {
    assert(read_ptr != NULL);
    assert(warning_flags != NULL);

    if (len > 64) {
        len = 64;
    }

    *warning_flags = 0;

    for (int i = 0; i < len; i++) {
        if (read_ptr[i] == '1') {
            *warning_flags |= 0x1;
        } else if (read_ptr[i] == '0') {
        } else {
            ESP_LOGE(TAG, "Invalid character in warning flags: %c", read_ptr[i]);
        }

        *warning_flags <<= 1;
    }

    return 0;
}

static int parse_rsp_qpicf(const uint8_t* read_ptr, size_t len, uint32_t* fault) {
    assert(read_ptr != NULL);
    assert(fault != NULL);
    const size_t msg_len = 4;

    if (len != msg_len) {
        return -1; // Malformed
    }

    *fault = 0;

    for (int i = 0; i < len; i++) {
        *fault |= read_ptr[i];
        *fault <<= 8;
    }

    return 0;
}

static int parse_rsp_qopmp(const uint8_t* read_ptr, size_t len, int* max_output_power) {
    assert(read_ptr != NULL);
    assert(max_output_power != NULL);
    const size_t min_msg_len = 5;

    if (len < min_msg_len) { // Minimum length for LLLLL
        return -1; // Malformed
    }

    return strdec2int(max_output_power, (const char*)read_ptr, len);
}

static int parse_rsp_qmpptv(const uint8_t* read_ptr, size_t len, hilo_voltage_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t min_msg_len = 7;

    if (len < min_msg_len) { // Minimum length for HHH LLL
        return -1; // Malformed
    }

    int parsed = sscanf((char*)read_ptr, "%d %d", &response->high_voltage, &response->low_voltage);
    if (parsed != 2) {
        return -2; // Parsing error
    }

    return 0;
}

static int parse_rsp_qpvipv(const uint8_t* read_ptr, size_t len, hilo_voltage_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t min_msg_len = 7;

    if (len < min_msg_len) { // Minimum length for HHH LLL
        return -1; // Malformed
    }

    int parsed = sscanf((char*)read_ptr, "%d %d", &response->high_voltage, &response->low_voltage);
    if (parsed != 2) {
        return -2; // Parsing error
    }

    return 0;
}

static int parse_rsp_qlst(const uint8_t* read_ptr, size_t len, uint32_t* sleep_time) {
    assert(read_ptr != NULL);
    assert(sleep_time != NULL);
    const size_t min_msg_len = 2;

    if (len < min_msg_len) { // Minimum length for "LL"
        return -1; // Malformed
    }

    char buffer[3] = {0};
    memcpy(buffer, read_ptr, len - 1); // Exclude the trailing '\r'
    *sleep_time = (uint32_t)atoi(buffer);

    return 0;
}

static int parse_rsp_qtpr(const uint8_t* read_ptr, size_t len, qtpr_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t min_msg_len = 17; // Minimum length for "LLL.L SSS.S TTT.T"

    if (len < min_msg_len) {
        return -1; // Malformed
    }

    int parsed = sscanf((char*)read_ptr, "%f %f %f",
                        &response->boost_temp,
                        &response->inverter_temp,
                        &response->inner_temp);
    if (parsed != 3) {
        return -2; // Parsing error
    }

    return 0;
}

static int parse_rsp_qdi2(const uint8_t* read_ptr, size_t len, qdi2_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t min_msg_len = 13;

    if (len < min_msg_len) { // "HH.H LL.L NNN RESERVED"
        return -1; // Malformed
    }

    int parsed = sscanf((char*)read_ptr, "%f %f %lu",
                        &response->max_current,
                        &response->max_voltage,
                        &response->wait_time);
    if (parsed != 3) {
        return -2; // Parsing error
    }

    return 0;
}

static int parse_rsp_qgltv(const uint8_t* read_ptr, size_t len, hilo_voltage_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t msg_len = 7;

    if (len != msg_len) { // HHH LLL"
        return -1; // Malformed
    }

    int parsed = sscanf((char*)read_ptr, "%d %d", &response->high_voltage, &response->low_voltage);
    if (parsed != 2) {
        return -2; // Parsing error
    }

    return 0;
}

static int parse_rsp_qdm(const uint8_t* read_ptr, size_t len, uint32_t* model) {
    assert(read_ptr != NULL);
    const size_t msg_len = 3;

    if (len != msg_len) { // VVV
        return -1; // Malformed
    }

    int parsed = sscanf((char*)read_ptr, "%ld", model);

    if (parsed != 1) {
        return -2; // Parsing error
    }

    return 0;
}

/*RSP: KK YYYYMMDDHHMMSS AAA.A BBB.B CCC.C DDD.D EEE.E FFF.F GGG.G
HHH.H III.I JJ.J CKKK.K LLL MMM.M NNN.N OO.O PPP.P QQQ.Q
<b15b14b13b12b11b10b9b8b7b6b5b4b3b2b1b0><cr>*/
static int parse_rsp_qpihf(const uint8_t* read_ptr, size_t len, uint32_t* model) {
    assert(read_ptr != NULL);

    //if (len != msg_len) {
    //    return -1; // Malformed
   // }

    char temp[len];
    memcpy(temp, read_ptr, len); // Copy the model string
    temp[len] = '\0'; // Null-terminate the string
    ESP_LOGI(TAG, "parse_rsp_qpihf %s", temp);

    return -1;
}

static int parse_rsp_qchgs(const uint8_t* read_ptr, size_t len, qchgs_response_t* response) {
    assert(read_ptr != NULL);
    assert(response != NULL);
    const size_t msg_len = 20; // QCHGS response "AA.A BB.B CC.C DD.D<cr>"

    if (len != msg_len) {
        return -1; // Malformed
    }

    int parsed = sscanf((char*)read_ptr, "%f %f %f %f",
                        &response->charging_current,
                        &response->floating_voltage,
                        &response->max_current,
                        &response->bulk_voltage);
    if (parsed != 4) {
        return -2; // Parsing error
    }

    return 0;
}
// returns number of processed bytes
size_t rs232_2400_rsp_parse(uint16_t *rq_id, const uint8_t* data_in, size_t len_in, void* output, uint16_t *err) {
    assert(data_in != NULL);
    assert(output != NULL);
    size_t min_msg_len = 5;
    *err = PARSE_STATUS_NO_PACKET;

    if (len_in < min_msg_len) {
        return 0;
    }

    size_t packet_start_offset = 0xFFFFFFFF; // points to the '('
    size_t packet_end_offset = 0; // points to byte '\r'

    // find packet start - last '('
    for (size_t i = 0; i < len_in; i++) {
        if (data_in[i] == '(') {
            packet_start_offset = i;
        }
    }

    if (packet_start_offset == 0xFFFFFFFF) {
        //ESP_LOGW(TAG, "No packet start found");
        return len_in; // No valid packet found()
    }

    //ESP_LOGI(TAG, "Packet start found at %d", packet_start_offset);

    for (size_t i = packet_start_offset; i < len_in; i++) {
        if (data_in[i] == '\r') {
            packet_end_offset = i;
            break;
        }
    }

    if (packet_end_offset == 0) {
        //ESP_LOGW(TAG, "No packet end found");
        return packet_start_offset; // No valid packet found
    }

    size_t crc_ascii_len = 2; // uint16 in 2 ASCII

    // check if packet is long enough to parse CRC (uint16 in 2 ASCII) and '\r'
    if (packet_end_offset - packet_start_offset <= crc_ascii_len) {
        //ESP_LOGW(TAG, "Packet too short to parse CRC");
        return packet_start_offset;
    }

    ++packet_cnt;

    uint16_t act_crc = data_in[packet_end_offset-crc_ascii_len] << 8 | data_in[packet_end_offset-crc_ascii_len+1];

    //calculate crc
    uint16_t exp_crc = crc16_xmodem(&data_in[packet_start_offset], packet_end_offset - packet_start_offset - crc_ascii_len);

    if (exp_crc != act_crc) {
        ++crc_err_cnt;
        //ESP_LOGW(TAG, "CRC error exp=%04X act=%04X cnt=%d", exp_crc, act_crc, crc_err_cnt);
        if(crc_err_cnt % 10 == 0) {
            ESP_LOGI(TAG, "CRC errors=%d valid=%d pkt_cnt=%d", crc_err_cnt, valid_cnt, packet_cnt);
        }
        *err = PARSE_STATUS_INVALID_CRC;
        return packet_end_offset+1; // consume packet with '\r'
    }

    ((uint8_t*)data_in)[packet_end_offset-crc_ascii_len] = 0; // replace '(' with null to terminate the string for safe char* processing
    ((uint8_t*)data_in)[packet_end_offset-crc_ascii_len+1] = 0;

    size_t packet_len = packet_end_offset - packet_start_offset - crc_ascii_len - 1; // exclude CRC and '('
    const uint8_t* packet = (uint8_t*)&data_in[packet_start_offset+1];

    int nak_err = parse_rsp_is_nak(packet, packet_len);

    if(nak_err == 0){
        *err = PARSE_STATUS_RQ_REJECTED;
        //ESP_LOGW(TAG, "cmd %#x NAKed", cmd);
        memcpy(output, packet, packet_len);
        return packet_end_offset+1; // consume packet with '\r'
    }

    //ESP_LOGI(TAG, "Packet=%s", data_in + packet_start_offset + 1);
    //char log[packet_len + 1];'/0'
    //memcpy(log, packet, packet_len);
    //log[packet_len] = 0; // null terminate the string for logging
    // ESP_LOGI(TAG, "Valid packet=%s", log);

    switch (*rq_id) {
        case EM_RS232_2400_QPI: *err = parse_rsp_qpi(packet, packet_len, (uint32_t*)output); break;
        case EM_RS232_2400_QID: *err = parse_rsp_qid(packet, packet_len, (qid_response_t*)output); break;
        case EM_RS232_2400_QVFW:
        case EM_RS232_2400_QVFW2:
        case EM_RS232_2400_QVFW3: *err = parse_rsp_qvfw(packet, packet_len, (uint64_t*)output); break;
        case EM_RS232_2400_QPIRI: *err = parse_rsp_qpiri(packet, packet_len, (qpiri_response_t*)output); break;
        case EM_RS232_2400_QMD: *err = parse_rsp_qmd(packet, packet_len, (char*)output); break;
        case EM_RS232_2400_QFLAG: *err = parse_rsp_qflag(packet, packet_len, (qflag_response_t*)output); break;
        case EM_RS232_2400_QPIGS: *err = parse_rsp_qpigs(packet, packet_len, ((qpigs_response_t*)output)); break;
        case EM_RS232_2400_QMOD: *err = parse_rsp_qmod(packet, packet_len, (uint32_t*)output); break;
        case EM_RS232_2400_QGOV: *err = parse_rsp_qgov(packet, packet_len, (hilo_voltage_response_t*)output); break;
        case EM_RS232_2400_QGOF: *err = parse_rsp_qgof(packet, packet_len, (qgof_response_t*)output); break;
        case EM_RS232_2400_QOPMP: *err = parse_rsp_qopmp(packet, packet_len, (int*)output); break;
        case EM_RS232_2400_QMPPTV: *err = parse_rsp_qmpptv(packet, packet_len, (hilo_voltage_response_t*)output); break;
        case EM_RS232_2400_QPVIPV: *err = parse_rsp_qpvipv(packet, packet_len, (hilo_voltage_response_t*)output); break;
        case EM_RS232_2400_QLST: *err = parse_rsp_qlst(packet, packet_len, (uint32_t*)output); break;
        case EM_RS232_2400_QTPR: *err = parse_rsp_qtpr(packet, packet_len, (qtpr_response_t*)output); break;
        case EM_RS232_2400_QDI2: *err = parse_rsp_qdi2(packet, packet_len, (qdi2_response_t*)output); break;
        case EM_RS232_2400_QGLTV: *err = parse_rsp_qgltv(packet, packet_len, (hilo_voltage_response_t*)output); break;
        case EM_RS232_2400_QDM: *err = parse_rsp_qdm(packet, packet_len, (uint32_t*)output); break;
        case EM_RS232_2400_QPIHF: *err = parse_rsp_qpihf(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QBSDV: *err = parse_rsp_qbsdv(packet, packet_len, (float*)output); break;
        //case EM_RS232_2400_QPRIO: *err = parse_rsp_qprio(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QENF: *err = parse_rsp_qenf(packet, packet_len, (float*)output); break;
        //case EM_RS232_2400_QEBGP: *err = parse_rsp_qebgp(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QOPF: *err = parse_rsp_qopf(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QMDCC: *err = parse_rsp_qmdcc(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QPKT: *err = parse_rsp_qpkt(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QLDT: *err = parse_rsp_qldt(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QBSDP: *err = parse_rsp_qbsdp(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QPIGS2: *err = parse_rsp_qpigs2(packet, packet_len, ((float*)output), ((float*)output + 1)); break;
        case EM_RS232_2400_QCHGS: *err = parse_rsp_qchgs(packet, packet_len, (qchgs_response_t*)output); break;
        //case EM_RS232_2400_QVFTR: *err = parse_rsp_qvftr(packet, packet_len, (float*)output); break;
        case EM_RS232_2400_QPICF: *err = parse_rsp_qpicf(packet, packet_len, (uint32_t*)output); break;
        case EM_RS232_2400_QPIWS: *err = parse_rsp_qpiws(packet, packet_len, (uint64_t*)output); break;
        //case EM_RS232_2400_QDI: *err = parse_rsp_qdi(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QMCHGCR: *err = parse_rsp_qmchgcr(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QMUCHGCR: *err = parse_rsp_qmuchgcr(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QBOOT: *err = parse_rsp_qboot(packet, packet_len, (uint32_t*)output); break;
        //case EM_RS232_2400_QOPM: *err = parse_rsp_qopm(packet, packet_len, (uint32_t*)output); break;
        default:
          ESP_LOGW(TAG, "Unsupported rq %d", *rq_id);
          return packet_end_offset+1; // Unsupported command
    }

    if(*err == 0) {
        valid_cnt++;
    }

    return packet_end_offset+1; // consume packet with '\r'
}

uint8_t* rs232_2400_serialize(uint16_t cmd, const uint8_t* payload, size_t payload_len, size_t* out_len) {
    *out_len = payload_len + 2 + 1; //  + 2 ASCII CRC + '\r'

    uint8_t* ret = malloc(*out_len);
    if (ret == NULL) {
        assert(0);
        *out_len = 0;
        return NULL;
    }

    memcpy(ret, payload, payload_len);
    uint16_t crc = crc16_xmodem(ret, payload_len);
    ret[payload_len] = (crc >> 8); // MSB
    ret[payload_len+1] = crc & 0xFF; // LSB
    ret[payload_len+2] = '\r'; // End of packet

    return ret;
}


static const uint16_t crc16tab[256]= {
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
	0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
	0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
	0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
	0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
	0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
	0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
	0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
	0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
	0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
	0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
	0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
	0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
	0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
	0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
	0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
	0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
	0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
	0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
	0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
	0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
	0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
	0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
	0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
	0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
	0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
	0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
	0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
	0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
	0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

uint16_t crc16_xmodem(const void *buf, int len)
{
	register int counter;
	register uint16_t crc = 0;
	for( counter = 0; counter < len; counter++)
		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *(char *)buf++)&0x00FF];
	return crc;
}