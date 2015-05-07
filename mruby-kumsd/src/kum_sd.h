#pragma once

#include <stdint.h>

#define KUM_SD_MAX_CHANNEL_COUNT 8

#define KUM_SD_NONE 0
#define KUM_SD_SYNC 1
#define KUM_SD_SKEW 2

typedef struct {
  uint8_t start_time[6];
  int sync_type;
  uint8_t sync_time[6];
  int64_t skew;
  uint32_t address;
  uint16_t sample_rate;
  uint64_t written_samples;
  uint64_t lost_samples;
  int channel_count;
  int gain[KUM_SD_MAX_CHANNEL_COUNT];
  int bit_depth;
  uint8_t recorder_id[32];
  uint8_t rtc_id[32];
  uint8_t latitude[32];
  uint8_t longitude[32];
  uint8_t channel_names[KUM_SD_MAX_CHANNEL_COUNT][32];
  uint8_t comment[512];
} kum_sd_header;

/* Read a KUM SD header from a 512 byte block.
 * Returns 0 on success and -1 on failure. */
extern int kum_sd_header_read(kum_sd_header *header, const void *x);
