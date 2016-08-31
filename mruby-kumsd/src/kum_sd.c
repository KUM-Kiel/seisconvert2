#include "kum_sd.h"

#include <string.h>
#include "number.h"

#define X ((uint8_t *) x)
#define SX ((char *) x)
#define ADV_O(a) o += (a); if (o >= 512) return -1
#define SKIP0() while (!X[o]) { ADV_O(1); }

int kum_sd_header_read(kum_sd_header *header, const void *x)
{
  int o = 0, n, i, nc;
  if (!header || !x) return -1;
  /* The last byte must be zero. */
  if (X[511] != 0) return -1;
  /* Time */
  if (memcmp(X + o, "time", 4)) return -1;
  ADV_O(4);
  memcpy(header->start_time, X + o, 6);
  ADV_O(6);
  /* Sync/Skew */
  if (!memcmp(X + o, "sync", 4)) {
    header->sync_type = KUM_SD_SYNC;
    memcpy(header->sync_time, X + o + 4, 6);
    header->skew = ld_i32_be(X + o + 10);
  } else if (!memcmp(X + o, "skew", 4)) {
    header->sync_type = KUM_SD_SKEW;
    memcpy(header->sync_time, X + o + 4, 6);
    header->skew = ld_i32_be(X + o + 10);
  } else {
    header->sync_type = KUM_SD_NONE;
    memset(header->sync_time, 0, 6);
    header->skew = 0;
  }
  ADV_O(14);
  /* Address */
  if (memcmp(X + o, "addr", 4)) return -1;
  ADV_O(4);
  header->address = ld_u32_be(X + o);
  ADV_O(4);
  /* Sample Rate */
  if (memcmp(X + o, "rate", 4)) return -1;
  ADV_O(4);
  header->sample_rate = ld_u16_be(X + o);
  ADV_O(2);
  /* Written Samples */
  if (memcmp(X + o, "writ", 4)) return -1;
  ADV_O(4);
  header->written_samples = ld_u64_be(X + o);
  ADV_O(8);
  /* Lost Samples */
  if (memcmp(X + o, "lost", 4)) return -1;
  ADV_O(4);
  header->lost_samples = ld_u32_be(X + o);
  ADV_O(4);
  /* Channel Count */
  if (memcmp(X + o, "chan", 4)) return -1;
  ADV_O(4);
  nc = header->channel_count = ld_u8_be(X + o);
  if (nc < 1 || nc > KUM_SD_MAX_CHANNEL_COUNT) return -1;
  ADV_O(1);
  /* Gain */
  if (memcmp(X + o, "gain", 4)) return -1;
  ADV_O(4);
  memcpy(header->gain, X + o, nc);
  if (nc < KUM_SD_MAX_CHANNEL_COUNT) {
    memset(header->gain + nc, 0, KUM_SD_MAX_CHANNEL_COUNT - nc);
  }
  ADV_O(nc);
  /* Bit Depth */
  if (memcmp(X + o, "bitd", 4)) return -1;
  ADV_O(4);
  header->bit_depth = ld_u8_be(X + o);
  ADV_O(1);
  /* Recorder ID */
  if (memcmp(X + o, "rcid", 4)) return -1;
  ADV_O(4);
  n = strlen(SX + o);
  if (n >= sizeof(header->recorder_id)) return -1;
  if (n) memcpy(header->recorder_id, X + o, n);
  memset(header->recorder_id + n, 0, sizeof(header->recorder_id) - n);
  ADV_O(n + 1);
  SKIP0();
  /* RTC ID */
  if (memcmp(X + o, "rtci", 4)) return -1;
  ADV_O(4);
  n = strlen(SX + o);
  if (n >= sizeof(header->rtc_id)) return -1;
  if (n) memcpy(header->rtc_id, X + o, n);
  memset(header->rtc_id + n, 0, sizeof(header->rtc_id) - n);
  ADV_O(n + 1);
  SKIP0();
  /* Latitude */
  if (memcmp(X + o, "lati", 4)) return -1;
  ADV_O(4);
  n = strlen(SX + o);
  if (n >= sizeof(header->latitude)) return -1;
  if (n) memcpy(header->latitude, X + o, n);
  memset(header->latitude + n, 0, sizeof(header->latitude) - n);
  ADV_O(n + 1);
  SKIP0();
  /* Longitude */
  if (memcmp(X + o, "logi", 4)) return -1;
  ADV_O(4);
  n = strlen(SX + o);
  if (n >= sizeof(header->longitude)) return -1;
  if (n) memcpy(header->longitude, X + o, n);
  memset(header->longitude + n, 0, sizeof(header->longitude) - n);
  ADV_O(n + 1);
  SKIP0();
  /* Channel Names */
  if (memcmp(X + o, "alia", 4)) return -1;
  ADV_O(4);
  for (i = 0; i < nc; ++i) {
    n = strlen(SX + o);
    if (n >= sizeof(header->channel_names[i])) return -1;
    if (n) memcpy(header->channel_names[i], X + o, n);
    memset(header->channel_names[i] + n, 0, sizeof(header->channel_names[i]) - n);
    ADV_O(n + 1);
  }
  SKIP0();
  /* Comment */
  if (memcmp(X + o, "cmnt", 4)) return -1;
  ADV_O(4);
  n = strlen(SX + o);
  if (n >= sizeof(header->comment)) return -1;
  if (n) memcpy(header->comment, X + o, n);
  memset(header->comment + n, 0, sizeof(header->comment) - n);
  return 0;
}
