#ifndef SEED_H
#define SEED_H

#include <stdint.h>

/* Tells type of record stored in buffer. */
extern int seed_record_type(const uint8_t *buffer);

/* Struct for BTIMEs. */
#define SEED_BTIME_BYTES 10
typedef struct seed_btime_s seed_btime_t;
struct seed_btime_s {
  uint16_t year;
  uint16_t julian_day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t tenth_ms;
};

/* Read and write btime. */
extern int seed_read_btime(seed_btime_t *btime, const uint8_t *buffer);
extern int seed_write_btime(uint8_t *buffer, const seed_btime_t *btime);

/* Struct for data record headers. */
#define SEED_DATA_RECORD_HEADER_BYTES 48
typedef struct seed_data_record_header_s seed_data_record_header_t;
struct seed_data_record_header_s {
  int64_t sequence_number;
  uint8_t station_identifier[6];
  uint8_t location_identifier[3];
  uint8_t channel_identifier[4];
  uint8_t network_code[3];
  seed_btime_t start_time;
  uint16_t num_samples;
  int16_t sample_rate_factor;
  int16_t sample_rate_multiplier;
  uint8_t activity_flags;
  uint8_t io_flags;
  uint8_t data_quality_flags;
  uint8_t blockette_count;
  int32_t time_correction;
  uint16_t data_offset;
  uint16_t blockette_offset;
};

/* Reads a data record header. */
extern int seed_read_data_record_header(seed_data_record_header_t *header, const uint8_t *buffer);
extern int seed_write_data_record_header(uint8_t *buffer, const seed_data_record_header_t *header);

/* Struct for blockette 1000 (data only seed). */
#define SEED_BLOCKETTE_1000_BYTES 8
typedef struct seed_blockette_1000_s seed_blockette_1000_t;
struct seed_blockette_1000_s {
  uint16_t next_blockette;
  uint8_t encoding;
  uint8_t word_order;
  uint8_t data_record_length;
};

/* Reads a blockette 1000 from buffer. */
extern int seed_read_blockette_1000(seed_blockette_1000_t *blockette, const uint8_t *buffer);
extern int seed_write_blockette_1000(uint8_t *buffer, const seed_blockette_1000_t *blockette);

/* Converts between seed samplerate and double sample rate. */
extern double seed_sample_rate(int16_t factor, int16_t multiplier);
extern int seed_sample_rate_from_int(uint32_t sample_rate, int16_t *factor, int16_t *multiplier);

#endif
