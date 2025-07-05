/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/protocol.h"
#include "em/scheduler.h"
#include "em/storage.h"
#include "em/utils.h"

#include <stdint.h>
#include <string.h>
#include <esp_rom_crc.h>
#include <esp_system.h>
#include <common.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <sys/queue.h>

#include <esp_log.h>
#define LOG_TAG "STOR"

#define ENTRY_NOT_FOUND (UINT32_MAX)
// 84 bytes
#define DEFAULT_SCHEDULE_STATES                                                                                                                                \
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,  \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,      \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,      \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

typedef enum {
  NVS_READ,
  NVS_WRITE,
} nvs_db_action_t;

static uint32_t dump_request;

static StaticSemaphore_t storage_mtx_data;
static SemaphoreHandle_t storage_mtx;

typedef struct on_changed_cb_node {
  void (*callback)(void);
  SLIST_ENTRY(on_changed_cb_node) next;
} on_changed_cb_node_t;

static SLIST_HEAD(on_changed_cb_slist, on_changed_cb_node) storage_slist;

static const database_t factory_config = {
  .device =
    {
      .perm =
        {
          .unique_id = 0,
          .coord = // Warsaw: 52.2297° N, 21.0122° E factor -5
          {
            .latitude = 5222970,
            .longitude = 2101220,
          },
        },
    },
};

static database_t database;

static struct nvs_mapper_s {
  /* Pointer in database */
  void *entry_db_ptr;
  /* Size of that flash entry */
  size_t entry_size;
  /* Entry type */
  nvs_type_t entry_type;
  /* NVS key, for key-value pair*/
  const char *key;
  /* Occuppied storage section (in database not in flash) */
  storage_section_t section;
  /* Backward compatibility field -> An NVS key-value pair may exist even if the application no longer uses that entry.
   * This field explicitly defines that and enables tracking of changes over time.
   *
   * Entries no longer in use are deleted, but we must retain all historical NVS keys between previous and current
   * firmware versions to avoid misinterpreting key-value pairs.
   * e.g.
   * Firmware version 0.0.1 uses the key "d/dummy." After an update to version 0.0.2, the "d/dummy" key is no longer
   * used, so the is_used parameter is set to false. Version 0.0.2 will then erase that key when
   * clear_unused_db_entries() is called, removing it from the NVS namespace. If version 0.0.3 later uses the "d/dummy"
   * key again for any reason, the erasure by version 0.0.2 ensures that 0.0.3 does not accidentally read any previous
   * value associated with "d/dummy."
   * Version 0.0.3 is allowed to delete such entry from the nvs_mapper table below if it is not going to use it.
   */
  bool is_used;
} nvs_mapper[] = {{
                    .entry_db_ptr = &database.device.perm.checksum,
                    .entry_size = sizeof(database.device.perm.checksum),
                    .entry_type = NVS_TYPE_U32,
                    .key = "d/crc",
                    .section = STORAGE_DEVICE,
                    .is_used = true,
                  },
                  {
                    .entry_db_ptr = &database.device.perm.unique_id,
                    .entry_size = sizeof(database.device.perm.unique_id),
                    .entry_type = NVS_TYPE_U32,
                    .key = "d/unique_id",
                    .section = STORAGE_DEVICE,
                    .is_used = true,
                  },
                  {
                    .entry_db_ptr = &database.device.perm.coord.latitude,
                    .entry_size = sizeof(database.device.perm.coord.latitude),
                    .entry_type = NVS_TYPE_I64,
                    .key = "d/c/lat",
                    .section = STORAGE_DEVICE,
                    .is_used = true,
                  },
                  {
                    .entry_db_ptr = &database.device.perm.coord.longitude,
                    .entry_size = sizeof(database.device.perm.coord.longitude),
                    .entry_type = NVS_TYPE_I64,
                    .key = "d/c/long",
                    .section = STORAGE_DEVICE,
                    .is_used = true,
                  },
                  {
                    .entry_db_ptr = &database.ds.perm.checksum,
                    .entry_size = sizeof(database.ds.perm.checksum),
                    .entry_type = NVS_TYPE_U32,
                    .key = "ds/crc",
                    .section = STORAGE_DS,
                    .is_used = true,
                  },
                };

static esp_err_t process_db_entry(nvs_handle_t handle, void *entry_db_pointer, size_t entry_size, const char *key, nvs_type_t nvs_type, nvs_db_action_t action)
{
  if (!entry_db_pointer || !entry_size || !key) {
    ESP_LOGE(LOG_TAG, "Can't process entry=%p, invalid arg", entry_db_pointer);
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = ESP_OK;

  switch (nvs_type) {
  case NVS_TYPE_U8: {
    assert(entry_size == sizeof(uint8_t));
    if (action == NVS_WRITE) {
      ret = nvs_set_u8(handle, key, *((uint8_t *)entry_db_pointer));
    } else if (action == NVS_READ) {
      ret = nvs_get_u8(handle, key, (uint8_t *)entry_db_pointer);
    }
  } break;

  case NVS_TYPE_I8: {
    assert(entry_size == sizeof(int8_t));
    if (action == NVS_WRITE) {
      ret = nvs_set_i8(handle, key, *((int8_t *)entry_db_pointer));
    } else if (action == NVS_READ) {
      ret = nvs_get_i8(handle, key, (int8_t *)entry_db_pointer);
    }
  } break;

  case NVS_TYPE_U16: {
    assert(entry_size == sizeof(uint16_t));
    if (action == NVS_WRITE) {
      ret = nvs_set_u16(handle, key, *((uint16_t *)entry_db_pointer));
    } else if (action == NVS_READ) {
      ret = nvs_get_u16(handle, key, (uint16_t *)entry_db_pointer);
    }
  } break;

  case NVS_TYPE_I16: {
    assert(entry_size == sizeof(int16_t));
    if (action == NVS_WRITE) {
      ret = nvs_set_i16(handle, key, *((int16_t *)entry_db_pointer));
    } else if (action == NVS_READ) {
      ret = nvs_get_i16(handle, key, (int16_t *)entry_db_pointer);
    }
  } break;

  case NVS_TYPE_U32: {
    assert(entry_size == sizeof(uint32_t));
    if (action == NVS_WRITE) {
      ret = nvs_set_u32(handle, key, *((uint32_t *)entry_db_pointer));
    } else if (action == NVS_READ) {
      ret = nvs_get_u32(handle, key, (uint32_t *)entry_db_pointer);
    }
  } break;

  case NVS_TYPE_I32: {
    assert(entry_size == sizeof(int32_t));
    if (action == NVS_WRITE) {
      ret = nvs_set_i32(handle, key, *((int32_t *)entry_db_pointer));
    } else if (action == NVS_READ) {
      ret = nvs_get_i32(handle, key, (int32_t *)entry_db_pointer);
    }
  } break;

  case NVS_TYPE_U64: {
    assert(entry_size == sizeof(uint64_t));
    if (action == NVS_WRITE) {
      ret = nvs_set_u64(handle, key, *((uint64_t *)entry_db_pointer));
    } else if (action == NVS_READ) {
      ret = nvs_get_u64(handle, key, (uint64_t *)entry_db_pointer);
    }
  } break;

  case NVS_TYPE_I64: {
    assert(entry_size == sizeof(int64_t));
    if (action == NVS_WRITE) {
      ret = nvs_set_i64(handle, key, *((int64_t *)entry_db_pointer));
    } else if (action == NVS_READ) {
      ret = nvs_get_i64(handle, key, (int64_t *)entry_db_pointer);
    }
  } break;

  case NVS_TYPE_STR: {
    if (action == NVS_WRITE) {
      ret = nvs_set_str(handle, key, (const char *)entry_db_pointer);
    } else if (action == NVS_READ) {
      size_t read_len = entry_size;
      ret = nvs_get_str(handle, key, entry_db_pointer, &read_len);
    }
  } break;

  case NVS_TYPE_BLOB: {
    if (action == NVS_WRITE) {
      ret = nvs_set_blob(handle, key, entry_db_pointer, entry_size);
    } else if (action == NVS_READ) {
      size_t read_len = entry_size;
      ret = nvs_get_blob(handle, key, entry_db_pointer, &read_len);
    }
  } break;

  default:
    ret = ESP_ERR_NOT_SUPPORTED;
    break;
  }

  return ret;
}

static uint32_t find_entry_db_idx(void *entry_db_ptr)
{
  if (!entry_db_ptr) {
    return ENTRY_NOT_FOUND;
  }

  for (uint32_t i = 0; i < ARRAY_LENGTH(nvs_mapper); i++) {
    if (!nvs_mapper[i].is_used) {
      continue;
    }

    if (entry_db_ptr == nvs_mapper[i].entry_db_ptr) {
      return i;
    }
  }

  return ENTRY_NOT_FOUND;
}

static void clear_unused_db_entries(nvs_handle_t handle)
{
  for (uint32_t i = 0; i < ARRAY_LENGTH(nvs_mapper); i++) {
    if (nvs_mapper[i].is_used) {
      continue;
    }

    /* Entry no longer used, delete such key */
    const esp_err_t ret = nvs_erase_key(handle, nvs_mapper[i].key);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
      ESP_LOGI(LOG_TAG, "Key=%s is already deleted", nvs_mapper[i].key);
      continue;
    }

    if (ret) {
      ESP_LOGE(LOG_TAG, "Can't erase key=%s:%i", nvs_mapper[i].key, ret);
      continue;
    }

    ESP_LOGI(LOG_TAG, "Key=%s deletion success", nvs_mapper[i].key);
  }
}

static void read_db_entries(nvs_handle_t handle)
{
  for (uint32_t i = 0; i < ARRAY_LENGTH(nvs_mapper); i++) {
    if (!nvs_mapper[i].is_used) {
      continue;
    }

    const esp_err_t ret = process_db_entry(handle, nvs_mapper[i].entry_db_ptr, nvs_mapper[i].entry_size, nvs_mapper[i].key, nvs_mapper[i].entry_type, NVS_READ);
    if (ret == ESP_OK || ret == ESP_ERR_NVS_NOT_FOUND) {
      continue;
    }

    ESP_LOGE(LOG_TAG, "Can't read entry=%lu:0x%04X", i, ret);
    /* On failure the value set in database won't be overwritten */
    ESP_LOG_BUFFER_HEX(LOG_TAG " Use default:", nvs_mapper[i].entry_db_ptr, nvs_mapper[i].entry_size);
    continue;
  }
}

static void write_db_entries(nvs_handle_t handle)
{
  for (uint32_t i = 0; i < ARRAY_LENGTH(nvs_mapper); i++) {
    if (!nvs_mapper[i].is_used) {
      continue;
    }

    const esp_err_t ret =
      process_db_entry(handle, nvs_mapper[i].entry_db_ptr, nvs_mapper[i].entry_size, nvs_mapper[i].key, nvs_mapper[i].entry_type, NVS_WRITE);
    if (ret) {
      ESP_LOGE(LOG_TAG, "Can't write database entry=%lu:0x%04X", i, ret);
    }
  }
}

static void save_crc_of_section(nvs_handle_t handle, storage_section_t section)
{
  void *crc_ptr = NULL;

  switch (section) {
  case STORAGE_DEVICE:
    crc_ptr = &database.device.perm.checksum;
    break;

  case STORAGE_DS:
    crc_ptr = &database.ds.perm.checksum;

  default:
    return;
  }

  uint32_t idx = find_entry_db_idx(crc_ptr);
  if (idx == ENTRY_NOT_FOUND) {
    return;
  }

  ESP_ERROR_CHECK(nvs_set_u32(handle, nvs_mapper[idx].key, *((uint32_t *)crc_ptr)));
}

static void dump_check(uint32_t param, void *user_ctx)
{
  ESP_UNUSED(param);
  ESP_UNUSED(user_ctx);

  ESP_LOGD(LOG_TAG, "%s", __func__);
  xSemaphoreTake(storage_mtx, portMAX_DELAY);
  if (!(dump_request & STORAGE_MASK)) {
    xSemaphoreGive(storage_mtx);
    return;
  }

  uint32_t crc32 = 0;

  if (dump_request & (1U << STORAGE_DEVICE)) {
    /* Omit checksum member in checksum calculation */
    crc32 = esp_rom_crc32_le(0, ((uint8_t *)&database.device.perm + sizeof(database.device.perm.checksum)),
                             sizeof(database.device.perm) - sizeof(database.device.perm.checksum));

    if (crc32 == database.device.perm.checksum) {
      dump_request &= ~(1U << STORAGE_DEVICE);
    } else {
      database.device.perm.checksum = crc32;
    }
  }

  if (dump_request & (1U << STORAGE_DS)) {
    /* Omit checksum member in checksum calculation */
    crc32 =
      esp_rom_crc32_le(0, ((uint8_t *)&database.ds.perm + sizeof(database.ds.perm.checksum)), sizeof(database.ds.perm) - sizeof(database.ds.perm.checksum));
    if (crc32 == database.ds.perm.checksum) {
      dump_request &= ~(1U << STORAGE_DS);
    } else {
      database.ds.perm.checksum = crc32;
    }
  }

  if (!dump_request) {
    /* Checksum did not change */
    ESP_LOGI(LOG_TAG, "No change in storage checksum");
    xSemaphoreGive(storage_mtx);
    return;
  }

  xSemaphoreGive(storage_mtx);

  ESP_LOGI(LOG_TAG, "Dump request=0x%08lX", dump_request);

  nvs_handle_t handle = 0;
  ESP_ERROR_CHECK(nvs_open(__STRINGIFY(STORAGE), NVS_READWRITE, &handle));

  if (dump_request & (1U << STORAGE_DEVICE)) {
    save_crc_of_section(handle, STORAGE_DEVICE);
  }

  if (dump_request & (1U << STORAGE_DS)) {
    save_crc_of_section(handle, STORAGE_DS);
  }

  dump_request = 0U;
  /* Save all pending nvs_set_* calls */
  ESP_ERROR_CHECK(nvs_commit(handle));
  nvs_close(handle);

  /* Call on storage changed callbacks */
  on_changed_cb_node_t *node, *temp;

  SLIST_FOREACH_SAFE(node, &storage_slist, next, temp)
  {
    node->callback();
  }
}

static void section_dump_request(enum storage_section_e section)
{
  dump_request |= (1 << section);

  /* Track how often the section_dump_request is being called. It should be called on certain events */
  ESP_LOGD(LOG_TAG, "%s", __func__);
  /* scheduler_set_callback acts like a debouncer to limit flash operations */
  scheduler_set_callback(dump_check, SCH_PARAM_NONE, SCH_CTX_NONE, CONFIG_EM_STORAGE_DUMP_DEBOUNCE_MS);
}

static void storage_save_db_entry(void *entry_db_ptr)
{
  assert(entry_db_ptr);

  uint32_t idx = find_entry_db_idx(entry_db_ptr);
  if (idx == ENTRY_NOT_FOUND) {
    ESP_LOGE(LOG_TAG, "Can't find such entry in database");
    return;
  }

  nvs_handle_t handle = 0;
  esp_err_t ret = nvs_open(__STRINGIFY(STORAGE), NVS_READWRITE, &handle);
  assert(ret == ESP_OK || ret == ESP_ERR_NVS_NOT_FOUND);

  ret = process_db_entry(handle, nvs_mapper[idx].entry_db_ptr, nvs_mapper[idx].entry_size, nvs_mapper[idx].key, nvs_mapper[idx].entry_type, NVS_WRITE);
  if (ret) {
    ESP_LOGE(LOG_TAG, "Can't write entry=%lu:0x%04X", idx, ret);
    nvs_close(handle);
    return;
  }

  nvs_close(handle);

  section_dump_request(nvs_mapper[idx].section);
}

static void save_database(void)
{
  nvs_handle_t handle = 0;
  ESP_ERROR_CHECK(nvs_open(__STRINGIFY(STORAGE), NVS_READWRITE, &handle));

  xSemaphoreTake(storage_mtx, portMAX_DELAY);
  write_db_entries(handle);
  ESP_ERROR_CHECK(nvs_commit(handle));
  xSemaphoreGive(storage_mtx);

  nvs_close(handle);
}

static bool remove_node(void (*cb)(void))
{
  if (SLIST_EMPTY(&storage_slist)) {
    return false;
  }

  on_changed_cb_node_t *node, *temp;

  SLIST_FOREACH_SAFE(node, &storage_slist, next, temp)
  {
    if (node->callback == cb) {
      SLIST_REMOVE(&storage_slist, node, on_changed_cb_node, next);
      free(node);
      return true;
    }
  }

  return false;
}

static void insert_node(void (*cb)(void))
{
  on_changed_cb_node_t *new_node = calloc(1, sizeof(on_changed_cb_node_t));
  assert(new_node);

  new_node->callback = cb;

  SLIST_INSERT_HEAD(&storage_slist, new_node, next);
}

void storage_register_on_changed_callback(void (*callback)(void))
{
  (void)remove_node(callback);
  insert_node(callback);
}

void storage_unregister_on_changed_callback(void (*callback)(void))
{
  (void)remove_node(callback);
}

void storage_force_dump_panic(void)
{
  save_database();
  ESP_LOGI(LOG_TAG, "Storage force dump handled");
}

void storage_init(void)
{
  storage_mtx = xSemaphoreCreateMutexStatic(&storage_mtx_data);
  assert(storage_mtx);

  /* Fill up application database with factory config. If there were any modifications of it. The defaults will be
   * overwritten by read_db_entries().
   *
   */
  database = factory_config;

  nvs_handle_t handle = 0;
  esp_err_t ret = nvs_open(__STRINGIFY(STORAGE), NVS_READWRITE, &handle);
  assert(ret == ESP_OK || ret == ESP_ERR_NVS_NOT_FOUND);
  if (ret == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(LOG_TAG, "Storage not yet initialized at least once");
    return;
  }

  clear_unused_db_entries(handle);
  read_db_entries(handle);

  nvs_close(handle);
  ESP_LOGI(LOG_TAG, "Storage initialized");
}

storage_datasets_t storage_datasets(void)
{
  xSemaphoreTake(storage_mtx, portMAX_DELAY);
  storage_datasets_t ds = database.ds;
  xSemaphoreGive(storage_mtx);
  return ds;
}

storage_device_t storage_device(void)
{
  xSemaphoreTake(storage_mtx, portMAX_DELAY);
  storage_device_t device = database.device;
  xSemaphoreGive(storage_mtx);
  return device;
}

void storage_set_device_unique_id(uint32_t unique_id)
{
  xSemaphoreTake(storage_mtx, portMAX_DELAY);
  database.device.perm.unique_id = unique_id;
  storage_save_db_entry(&database.device.perm.unique_id);
  xSemaphoreGive(storage_mtx);
}

void storage_set_coordinates(int64_t latitude, int64_t longitude)
{
  xSemaphoreTake(storage_mtx, portMAX_DELAY);
  database.device.perm.coord.longitude = longitude;
  storage_save_db_entry(&database.device.perm.coord.longitude);
  database.device.perm.coord.latitude = latitude;
  storage_save_db_entry(&database.device.perm.coord.latitude);
  xSemaphoreGive(storage_mtx);
}

int32_t storage_meter_value(uint16_t meas_type, time_t *timestamp)
{
  int32_t ret = INT32_MAX;

  switch (meas_type) {
  case MEASTYPE_VOLTAGE_RMS: {
    xSemaphoreTake(storage_mtx, portMAX_DELAY);
    ret = database.meter_voltage.current_val;
    *timestamp = database.meter_voltage.current_val_time;
    xSemaphoreGive(storage_mtx);
  } break;

  case MEASTYPE_CURRENT_RMS: {
    xSemaphoreTake(storage_mtx, portMAX_DELAY);
    ret = database.meter_current.current_val;
    *timestamp = database.meter_current.current_val_time;
    xSemaphoreGive(storage_mtx);
  } break;

  case MEASTYPE_POWER_ACTIVE: {
    xSemaphoreTake(storage_mtx, portMAX_DELAY);
    ret = database.meter_power.current_val;
    *timestamp = database.meter_power.current_val_time;
    xSemaphoreGive(storage_mtx);
  } break;

  default:
    ESP_LOGE(LOG_TAG, "Unknown meas type=%d", meas_type);
  }

  return ret;
}

int storage_set_meter_value(time_t datetime, uint16_t meas_type, int32_t value)
{
  switch (meas_type) {
  case MEASTYPE_VOLTAGE_RMS: {
    xSemaphoreTake(storage_mtx, portMAX_DELAY);
    database.meter_voltage.current_val = value;
    database.meter_voltage.current_val_time = datetime;
    xSemaphoreGive(storage_mtx);
  } break;

  case MEASTYPE_CURRENT_RMS: {
    xSemaphoreTake(storage_mtx, portMAX_DELAY);
    database.meter_current.current_val = value;
    database.meter_current.current_val_time = datetime;
    xSemaphoreGive(storage_mtx);
  } break;

  case MEASTYPE_POWER_ACTIVE: {
    xSemaphoreTake(storage_mtx, portMAX_DELAY);
    database.meter_power.current_val = value;
    database.meter_power.current_val_time = datetime;
    xSemaphoreGive(storage_mtx);
  } break;

  default:
    ESP_LOGE(LOG_TAG, "Unknown meas type=%d", meas_type);
    return -1;
  }

  return 0;
}

/* type is meas_type_t*/
bool storage_add_meter_entry(time_t datetime, uint16_t meas_type, int32_t value)
{
  if (datetime < em_utils_get_compile_time()) {
    ESP_LOGE(LOG_TAG, "Meter invalid datetime");
    return false;
  }

  storage_meter_t *meter = NULL;

  switch (meas_type) {
  case MEASTYPE_VOLTAGE_RMS: {
    meter = &database.meter_voltage;
  } break;

  case MEASTYPE_POWER_ACTIVE: {
    meter = &database.meter_power;
  } break;

  default:
    ESP_LOGE(LOG_TAG, "Unknown meas type=%d", meas_type);
    return false;
  }

  xSemaphoreTake(storage_mtx, portMAX_DELAY);

  bool flush = meter->len >= ARRAY_LENGTH(meter->entries);

  if (flush) {
    xSemaphoreGive(storage_mtx);
    ESP_LOGE(LOG_TAG, "Meas[%d] not saved", meas_type);
    return flush;
  }

  if (meter->len == 0) {
    meter->ref_timestamp = datetime;
  }

  meter->t_offset[meter->len] = datetime - meter->ref_timestamp;
  meter->entries[meter->len] = value;
  meter->len++;

  ESP_LOGI(LOG_TAG, "Meas[%d] saved", meas_type);

  if (!flush) {
    flush = meter->len >= ARRAY_LENGTH(meter->entries);
  }

  xSemaphoreGive(storage_mtx);
  return flush;
}

bool storage_add_energy_entry(time_t datetime, uint64_t value)
{
  if (datetime < em_utils_get_compile_time()) {
    ESP_LOGE(LOG_TAG, "Energy invalid datetime");
    return false;
  }

  storage_energy_t *energy = &database.meter_energy;

  xSemaphoreTake(storage_mtx, portMAX_DELAY);

  bool flush = energy->len >= ARRAY_LENGTH(energy->entries);

  if (flush) {
    xSemaphoreGive(storage_mtx);
    ESP_LOGE(LOG_TAG, "Energy not saved");
    return flush;
  }

  if (energy->len == 0) {
    energy->ref_timestamp = datetime;
  } else {
    if (energy->interval == 0) {
      energy->interval = (datetime - energy->ref_timestamp);
    } else {
      if (datetime != energy->ref_timestamp + energy->len * energy->interval) {
        ESP_LOGW(LOG_TAG, "Energy timestamp %lld does not match expected %lld", datetime, energy->ref_timestamp + energy->len * energy->interval);
      }
    }
  }

  energy->entries[energy->len] = value;
  energy->len++;

  ESP_LOGI(LOG_TAG, "Energy %.3f kWh", value / 3600 / 1000.0);

  if (!flush) {
    flush = energy->len >= ARRAY_LENGTH(energy->entries);
  }

  xSemaphoreGive(storage_mtx);
  return flush;
}

void storage_set_factory_config(void)
{
  xSemaphoreTake(storage_mtx, portMAX_DELAY);
  database = factory_config;
  xSemaphoreGive(storage_mtx);
  save_database();
}
