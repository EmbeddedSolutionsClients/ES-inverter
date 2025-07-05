/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/buffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_err.h>

#include <esp_log.h>
#define LOG_TAG "em_slip"

#define SLIP_BEGIN     (uint8_t)(0xCF) /* Start of packet */
#define SLIP_END       (uint8_t)(0xC0) /* End of packet */
#define SLIP_ESC       (uint8_t)(0xDB) /* Escape character */
#define SLIP_ESC_BEGIN (uint8_t)(0xDE) /* Escaped BEGIN character */
#define SLIP_ESC_END   (uint8_t)(0xDC) /* Escaped END character */
#define SLIP_ESC_ESC   (uint8_t)(0xDD) /* Escaped ESC character */

// Define error messages
#define ERR_NONE                    (uint8_t)(0)
#define ERR_INCOMPLETE_PACKET       (uint8_t)(1)
#define ERR_INCOMPLETE_ESC_SEQUENCE (uint8_t)(2)
#define ERR_INVALID_ESC_SEQUENCE    (uint8_t)(3)
#define ERR_PACKET_START_NOT_FOUND  (uint8_t)(4)
#define ERR_PACKET_CUT              (uint8_t)(5)
#define ERR_INVALID_ARG             (uint8_t)(6)
#define ERR_NO_MEMORY               (uint8_t)(7)
#define ERR_UNKNOWN                 (uint8_t)(8)

typedef int32_t slip_err_t;

typedef struct {
  buffer_t decoded;
  buffer_t encoded;
  bool insideOfPacket;
  bool escActive;
} slip_codec_t;

static slip_err_t decode(buffer_t *decoded, buffer_t *encoded, uint16_t *processedNum, bool *insideOfPacket,
                         bool *escActive)
{
  *processedNum = 0;

  if (encoded->len == 0) {
    return ERR_PACKET_START_NOT_FOUND;
  }

  for (; *processedNum < encoded->len;) {
    if (*processedNum >= decoded->cap) {
      return ERR_NO_MEMORY;
    }

    uint8_t encodedByte = encoded->data[*processedNum];
    ++(*processedNum);

    if (!*insideOfPacket && encodedByte != SLIP_BEGIN) {
      continue;
    }

    switch (encodedByte) {
    case SLIP_BEGIN:
      if (!*insideOfPacket) {
        *insideOfPacket = true;
      } else {
        /* Put back SLIP_BEGIN to use it when decoding next packet */
        *insideOfPacket = false;
        --processedNum;
        return ERR_PACKET_CUT;
      }
      break;

    case SLIP_ESC:
      if (*insideOfPacket) {
        *escActive = true;
      }
      continue;

    case SLIP_ESC_BEGIN:
      if (*escActive) {
        *escActive = 0;
        buffer_push(decoded, SLIP_BEGIN);
      } else {
        buffer_push(decoded, encodedByte);
      }
      break;

    case SLIP_ESC_END:
      if (*escActive) {
        *escActive = false;
        buffer_push(decoded, SLIP_END);
      } else {
        buffer_push(decoded, encodedByte);
      }
      break;

    case SLIP_ESC_ESC:
      if (*escActive) {
        *escActive = 0;
        buffer_push(decoded, SLIP_ESC);
      } else {
        buffer_push(decoded, encodedByte);
      }
      break;

    case SLIP_END:
      if (!*insideOfPacket) {
        continue;
      }

      *insideOfPacket = false;

      if (*escActive) {
        *escActive = 0;
        return ERR_INCOMPLETE_ESC_SEQUENCE;
      }

      /* Successfully decoded packet */
      return 0;

    default:
      if (*escActive) {
        *escActive = 0;
        *insideOfPacket = false;
        return ERR_INVALID_ESC_SEQUENCE;
      }

      buffer_push(decoded, encodedByte);
      break;
    }
  }

  if (*insideOfPacket) {
    return ERR_INCOMPLETE_PACKET;
  }

  return ERR_PACKET_START_NOT_FOUND;
}

void slip_clear(slip_codec_t *s)
{
  s->decoded.len = 0;
  s->encoded.len = 0;
}

// TODO: add description
/* encodedDataLen is buffer size at input.
 *  encodedDataLen is set to number of encoded bytes at output
 */
static esp_err_t slip_encode(uint8_t *encodedData, uint32_t *encodedDataLen, const uint8_t *data, uint32_t dataLen)
{
  if (data == NULL || dataLen == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  uint32_t maxEndocedDataLen = *encodedDataLen;

  encodedData[0] = SLIP_BEGIN;
  *encodedDataLen = 1;

  for (uint32_t i = 0; i < dataLen; i++) {
    switch (data[i]) {
    case SLIP_BEGIN:
      if (*encodedDataLen + 2 > maxEndocedDataLen) {
        return ESP_ERR_NO_MEM;
      }
      encodedData[(*encodedDataLen)++] = SLIP_ESC;
      encodedData[(*encodedDataLen)++] = SLIP_ESC_BEGIN;
      break;
    case SLIP_END:
      if (*encodedDataLen + 2 > maxEndocedDataLen) {
        return ESP_ERR_NO_MEM;
      }
      encodedData[(*encodedDataLen)++] = SLIP_ESC;
      encodedData[(*encodedDataLen)++] = SLIP_ESC_END;
      break;
    case SLIP_ESC:
      if (*encodedDataLen + 2 > maxEndocedDataLen) {
        return ESP_ERR_NO_MEM;
      }
      encodedData[(*encodedDataLen)++] = SLIP_ESC;
      encodedData[(*encodedDataLen)++] = SLIP_ESC_ESC;
      break;
    default:
      if (*encodedDataLen + 1 > maxEndocedDataLen) {
        return ESP_ERR_NO_MEM;
      }
      encodedData[(*encodedDataLen)++] = data[i];
      break;
    }
  }

  if (*encodedDataLen + 1 > maxEndocedDataLen) {
    return ESP_ERR_NO_MEM;
  }
  encodedData[(*encodedDataLen)++] = SLIP_END;

  return ESP_OK;
}

static esp_err_t slip_add_data(slip_codec_t *s, const void *newEncodedData, uint32_t newEncodedDataLen)
{
  if (s == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  if (newEncodedDataLen == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  uint32_t neededBufLen = s->encoded.len + newEncodedDataLen;

  if (s->encoded.cap < neededBufLen) {
    return ESP_ERR_NO_MEM;
  }

  memcpy(s->encoded.data + s->encoded.len, newEncodedData, newEncodedDataLen);
  s->encoded.len += newEncodedDataLen;

  return ESP_OK;
}

static slip_err_t slip_decode(slip_codec_t *s)
{
  uint32_t err = ERR_INVALID_ARG;

  /* ToDo: 16 stands for? */
  for (uint32_t x = 0; x < 16; x++) {

    uint16_t processedNum = 0;
    err = decode(&s->decoded, &s->encoded, &processedNum, &s->insideOfPacket, &s->escActive);

    /* Discard processed data */
    buffer_pop_front(&s->encoded, processedNum);

    switch (err) {
    case ERR_PACKET_CUT:
      continue;
    case ERR_INCOMPLETE_PACKET:
    /* Keep encoded data and get more in another iteration */
    case ERR_NONE:
      return err;

    case ERR_INCOMPLETE_ESC_SEQUENCE:
    case ERR_INVALID_ESC_SEQUENCE:
    case ERR_PACKET_START_NOT_FOUND:
    case ERR_NO_MEMORY:
      buffer_clean(&s->decoded);
      return err;

    default:
      err = ERR_UNKNOWN;
      buffer_clean(&s->decoded);
      return err;
    }
  }

  return ERR_UNKNOWN;
}

esp_err_t slip_static_data_decoder(const uint8_t *data_in, uint32_t len_in, uint8_t **data_out, uint32_t *len_out)
{
  assert(len_in <= CONFIG_EM_SLIP_DECODED_DATA_BUF_SIZE);

  static uint8_t dec_data_buf[CONFIG_EM_SLIP_DECODED_DATA_BUF_SIZE];
  static uint8_t enc_data_buf[CONFIG_EM_SLIP_ENCODED_DATA_BUF_SIZE];

  /* ToDo: Seems like decoder member's values can vary, are we ok with that ? */
  static slip_codec_t decoder = {
    .decoded.data = dec_data_buf,
    .decoded.cap = sizeof(dec_data_buf),
    .decoded.len = 0,
    .encoded.data = enc_data_buf,
    .encoded.cap = sizeof(enc_data_buf),
    .encoded.len = 0,
    .escActive = false,
    .insideOfPacket = false,
  };

  /* Discard previous processed data */
  buffer_pop_front(&decoder.decoded, decoder.decoded.len);

  memset(dec_data_buf, 0x00, sizeof(dec_data_buf));
  memset(enc_data_buf, 0x00, sizeof(enc_data_buf));

  esp_err_t ret = slip_add_data(&decoder, data_in, len_in);
  if (ret != ERR_NONE) {
    ESP_LOGE(LOG_TAG, "Can't add slip data:%i", ret);
    return ret;
  }

  bool done = false;
  slip_err_t ret_decode;

  /* ToDo: Refactor use esp_err_t instead of slip_err_t. Not intuitive that slip_decode returning +4 means that the data
   * was parsed without errors
   */
  while (!done) {
    /* Keep decoding until no more packets */
    ret_decode = slip_decode(&decoder);
    switch (ret_decode) {
    case ERR_NONE:
      /* Data ready */
      ESP_LOGD(LOG_TAG, "Slip data decoded");
      *data_out = dec_data_buf;
      *len_out = decoder.decoded.len;
      break;
    case ERR_NO_MEMORY:
    case ERR_INVALID_ARG:
    case ERR_INCOMPLETE_ESC_SEQUENCE:
    case ERR_INVALID_ESC_SEQUENCE:
    case ERR_UNKNOWN:
      ESP_LOGE(LOG_TAG, "Can't decode packet:%li", ret_decode);
      [[fallthrough]];
    default:

      done = true;
      break;
    }
  }

  slip_clear(&decoder);
  return (ret_decode == ERR_PACKET_START_NOT_FOUND) ? ESP_OK : ESP_FAIL;
}

esp_err_t slip_static_data_encoder(const uint8_t *data_in, uint32_t len_in, uint8_t **data_out, uint32_t *len_out)
{
  assert(len_in <= CONFIG_EM_SLIP_ENCODED_DATA_BUF_SIZE);

  static uint8_t enc_data_buf[CONFIG_EM_SLIP_ENCODED_DATA_BUF_SIZE];
  memset(enc_data_buf, 0x00, sizeof(enc_data_buf));

  uint32_t enc_data_buf_len = sizeof(enc_data_buf);
  const esp_err_t ret = slip_encode(enc_data_buf, &enc_data_buf_len, data_in, len_in);
  if (ret) {
    ESP_LOGE(LOG_TAG, "Can't encode data:%i", ret);
    return ret;
  }

  *data_out = enc_data_buf;
  *len_out = enc_data_buf_len;
  return ESP_OK;
}
