#define _FILE_OFFSET_BITS 64
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <sample_buffer.h>
#include <stdio.h>
#include "bcd.h"
#include "kum_sd.h"
#include "number.h"

#define MRB_LARGE_INT(n) mrb_fixnum_value(n)

#define C_OBJECT (mrb->object_class)
#define C_KUM_SD (mrb_module_get(mrb, "KumSd"))
#define C_KUM_SD_FILE (mrb_class_get_under(mrb, C_KUM_SD, "KumSdFile"))
#define C_TIME (mrb_class_get(mrb, "Time"))
#define E_EXCEPTION (mrb->eException_class)

typedef struct {
  FILE *f;
  int channel_count;
  mrb_value base_time;
  mrb_int last_address;
  mrb_int pos;
} kum_sd_file;

static kum_sd_file *mrb_kum_sd_file_alloc(mrb_state *mrb)
{
  kum_sd_file *file = mrb_malloc(mrb, sizeof(kum_sd_file));
  file->f = 0;
  file->channel_count = 0;
  file->base_time = mrb_nil_value();
  file->last_address = 0;
  file->pos = 0;
  return file;
}

static void mrb_kum_sd_file_destroy(mrb_state *mrb, void *x)
{
  kum_sd_file *file = x;
  if (file && file->f) {
    fclose(file->f);
  }
  mrb_free(mrb, x);
}

static const mrb_data_type kum_sd_file_data_type = {"KumSdFile", mrb_kum_sd_file_destroy};

static mrb_value parse_header(mrb_state *mrb, mrb_value self)
{
  mrb_int l;
  int i;
  char *block;
  kum_sd_header header[1];
  mrb_value hash, channels, t;
  mrb_get_args(mrb, "s", &block, &l);
  if (l < 512) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Block must be 512 bytes long");
  }
  if (kum_sd_header_read(header, block)) {
    mrb_raise(mrb, E_EXCEPTION, "Malformed KUM SD header");
  }
  hash = mrb_hash_new(mrb);
  if (bcd_valid((char *) header->start_time)) {
    t = mrb_funcall(mrb, mrb_obj_value(mrb_class_get(mrb, "Time")), "utc", 6,
      mrb_fixnum_value(bcd_int(header->start_time[BCD_YEAR]) + 2000),
      mrb_fixnum_value(bcd_int(header->start_time[BCD_MONTH])),
      mrb_fixnum_value(bcd_int(header->start_time[BCD_DAY])),
      mrb_fixnum_value(bcd_int(header->start_time[BCD_HOUR])),
      mrb_fixnum_value(bcd_int(header->start_time[BCD_MINUTE])),
      mrb_fixnum_value(bcd_int(header->start_time[BCD_SECOND])));
  } else {
    t = mrb_nil_value();
  }
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "time")), t);
  if (bcd_valid((char *) header->sync_time)) {
    t = mrb_funcall(mrb, mrb_obj_value(mrb_class_get(mrb, "Time")), "utc", 6,
      mrb_fixnum_value(bcd_int(header->sync_time[BCD_YEAR]) + 2000),
      mrb_fixnum_value(bcd_int(header->sync_time[BCD_MONTH])),
      mrb_fixnum_value(bcd_int(header->sync_time[BCD_DAY])),
      mrb_fixnum_value(bcd_int(header->sync_time[BCD_HOUR])),
      mrb_fixnum_value(bcd_int(header->sync_time[BCD_MINUTE])),
      mrb_fixnum_value(bcd_int(header->sync_time[BCD_SECOND])));
  } else {
    t = mrb_nil_value();
  }
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "sync_time")), t);
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "skew")), MRB_LARGE_INT(header->skew));
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "address")), MRB_LARGE_INT(header->address));
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "sample_rate")), MRB_LARGE_INT(header->sample_rate));
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "written_samples")), MRB_LARGE_INT(header->written_samples));
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "lost_samples")), MRB_LARGE_INT(header->lost_samples));
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "recorder_id")), mrb_str_new_cstr(mrb, (char *) header->recorder_id));
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "rtc_id")), mrb_str_new_cstr(mrb, (char *) header->rtc_id));
  channels = mrb_ary_new_capa(mrb, header->channel_count);
  for (i = 0; i < header->channel_count; ++i) {
    mrb_ary_push(mrb, channels, mrb_symbol_value(mrb_intern_cstr(mrb, (char *) header->channel_names[i])));
  }
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "channels")), channels);
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "comment")), mrb_str_new_cstr(mrb, (char *) header->comment));
  return hash;
}

static mrb_value mrb_kum_sd_file_open(mrb_state *mrb, mrb_value self)
{
  kum_sd_file *file = 0;
  char *filename = 0;
  FILE *sdcard = 0;
  mrb_get_args(mrb, "z", &filename);

  if ((file = DATA_PTR(self))) {
    mrb_kum_sd_file_destroy(mrb, file);
  }
  mrb_data_init(self, 0, &kum_sd_file_data_type);

  sdcard = fopen(filename, "rb");
  if (!sdcard) mrb_raise(mrb, E_EXCEPTION, "Could not open SD card");
  file = mrb_kum_sd_file_alloc(mrb);
  file->f = sdcard;
  file->pos = 0;
  mrb_data_init(self, file, &kum_sd_file_data_type);
  return self;
}

static mrb_value mrb_kum_sd_file_close(mrb_state *mrb, mrb_value self)
{
  kum_sd_file *file = DATA_CHECK_GET_PTR(mrb, self, &kum_sd_file_data_type, kum_sd_file);
  if (file && file->f) {
    fclose(file->f);
    file->f = 0;
  }
  return self;
}

static mrb_value mrb_kum_sd_file_read(mrb_state *mrb, mrb_value self)
{
  mrb_int n;
  mrb_value s;
  size_t l;
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  mrb_get_args(mrb, "i", &n);
  if (n < 0) mrb_raise(mrb, E_RANGE_ERROR, "Can not read negative number of bytes");
  if (file && file->f) {
    s = mrb_str_buf_new(mrb, n);
    if (n > 0) {
      l = fread(RSTRING_PTR(s), 1, n, file->f);
      RSTR_SET_LEN(RSTRING(s), l);
      file->pos += l;
    }
    return s;
  } else {
    return mrb_nil_value();
  }
}

static mrb_value mrb_kum_sd_file_goto_addr(mrb_state *mrb, mrb_value self)
{
  mrb_int n;
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  mrb_get_args(mrb, "i", &n);
  if (n < 0) mrb_raise(mrb, E_RANGE_ERROR, "Can not go to negative address");
  if (n > MRB_INT_MAX / 512) mrb_raise(mrb, E_RANGE_ERROR, "Address too large");
  if (file && file->f) {
    if (fseek(file->f, n * 512, SEEK_SET) == -1) mrb_raise(mrb, E_RUNTIME_ERROR, "Seek failed");
    file->pos = n * 512;
  }
  return self;
}

#define IO_ERROR(x) mrb_raise(mrb, E_RUNTIME_ERROR, "Could not read from SD card" x)
static mrb_value mrb_kum_sd_file_read_frame(mrb_state *mrb, mrb_value self)
{
  char frame[4 * KUM_SD_MAX_CHANNEL_COUNT];
  int32_t type;
  uint64_t x;
  mrb_int i;
  mrb_value r, a[2];
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
start:
  if (file && file->f && file->channel_count) {
    if (file->pos + 4 > file->last_address * 512)
      return mrb_obj_new(mrb,
        mrb_class_get_under(mrb, C_KUM_SD, "EndFrame"),
        0, a);
    if (!fread(frame, 4, 1, file->f)) IO_ERROR("");
    file->pos += 4;
    type = ld_i32_be(frame);
    if (type % 2) {
      /* Control Frame */
      if (file->pos + 12 > file->last_address * 512)
        return mrb_obj_new(mrb,
          mrb_class_get_under(mrb, C_KUM_SD, "EndFrame"),
          0, a);
      if (!fread(frame + 4, 12, 1, file->f)) IO_ERROR("");
      file->pos += 12;
      switch (type) {
        case 1: /* Timestamp */
          a[0] = mrb_funcall(mrb, file->base_time, "+", 1, mrb_float_value(mrb, ld_i32_be(frame + 4)));
          return mrb_obj_new(mrb,
            mrb_class_get_under(mrb, C_KUM_SD, "Timestamp"),
            1, a);
        case 3: /* VBat/Humidity */
          a[0] = mrb_float_value(mrb, ld_i16_be(frame + 4) * 0.01);
          a[1] = mrb_float_value(mrb, ld_i16_be(frame + 6));
          return mrb_obj_new(mrb,
            mrb_class_get_under(mrb, C_KUM_SD, "VoltageHumidity"),
            2, a);
        case 5: /* Temperature */
          a[0] = mrb_float_value(mrb, ld_i16_be(frame + 4) * 0.01);
          return mrb_obj_new(mrb,
            mrb_class_get_under(mrb, C_KUM_SD, "Temperature"),
            1, a);
        case 7: /* Lost Frames */
          a[0] = mrb_fixnum_value(ld_u32_be(frame + 10));
          return mrb_obj_new(mrb,
            mrb_class_get_under(mrb, C_KUM_SD, "LostFrames"),
            1, a);
        case 9: /* Check Frame */
          a[0] = mrb_str_new(mrb, frame + 4, 6);
          return mrb_obj_new(mrb,
            mrb_class_get_under(mrb, C_KUM_SD, "Check"),
            1, a);
        case 11: /* Reboot */
          return mrb_obj_new(mrb,
            mrb_class_get_under(mrb, C_KUM_SD, "Reboot"),
            0, a);
        case 13: /* End Frame */
          return mrb_obj_new(mrb,
            mrb_class_get_under(mrb, C_KUM_SD, "EndFrame"),
            0, a);
        case 15: /* Frame Number. */
          x = ld_u64_be(frame + 4);
          a[0] = MRB_LARGE_INT(x);
          return mrb_obj_new(mrb,
            mrb_class_get_under(mrb, C_KUM_SD, "FrameNumber"),
            1, a);
        default:
          goto start;
      }
    } else {
      /* Data Frame */
      i = 4 * (file->channel_count - 1);
      if (file->pos + i > file->last_address * 512)
        return mrb_obj_new(mrb,
          mrb_class_get_under(mrb, C_KUM_SD, "EndFrame"),
          0, a);
      if (i) {
        if (!fread(frame + 4, i, 1, file->f)) IO_ERROR("");
        file->pos += i;
      }
      r = mrb_ary_new(mrb);
      mrb_ary_push(mrb, r, mrb_fixnum_value(type));
      for (i = 1; i < file->channel_count; ++i) {
        mrb_ary_push(mrb, r, mrb_fixnum_value(ld_i32_be(frame + 4 * i)));
      }
      return r;
    }
  } else {
    return mrb_nil_value();
  }
}

static mrb_value mrb_kum_sd_file_pos(mrb_state *mrb, mrb_value self)
{
  mrb_int n;
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  if (file && file->f) {
    n = file->pos;
    return MRB_LARGE_INT(n);
  } else {
    return mrb_nil_value();
  }
}

static mrb_value mrb_kum_sd_file_channel_count(mrb_state *mrb, mrb_value self)
{
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  if (file) {
    return mrb_fixnum_value(file->channel_count);
  } else {
    return mrb_nil_value();
  }
}

static mrb_value mrb_kum_sd_file_channel_count_set(mrb_state *mrb, mrb_value self)
{
  mrb_int c;
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  mrb_get_args(mrb, "i", &c);
  if (c < 1 || c > KUM_SD_MAX_CHANNEL_COUNT) mrb_raise(mrb, E_RANGE_ERROR, "Unsupported channel count");
  if (file) {
    file->channel_count = c;
  }
  return self;
}

static mrb_value mrb_kum_sd_file_base_time(mrb_state *mrb, mrb_value self)
{
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  if (file) {
    return file->base_time;
  } else {
    return mrb_nil_value();
  }
}

static mrb_value mrb_kum_sd_file_base_time_set(mrb_state *mrb, mrb_value self)
{
  mrb_value v;
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  mrb_get_args(mrb, "o", &v);
  if (!(mrb_obj_is_kind_of(mrb, v, C_TIME))) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Base time must be of class Time");
  }
  if (file) {
    file->base_time = v;
  }
  return self;
}

static mrb_value mrb_kum_sd_file_last_address(mrb_state *mrb, mrb_value self)
{
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  if (file) {
    return mrb_fixnum_value(file->last_address);
  } else {
    return mrb_nil_value();
  }
}

static mrb_value mrb_kum_sd_file_last_address_set(mrb_state *mrb, mrb_value self)
{
  mrb_int c;
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  mrb_get_args(mrb, "i", &c);
  if (c < 0) mrb_raise(mrb, E_RANGE_ERROR, "Address can not be negative");
  if (file) {
    file->last_address = c;
  }
  return self;
}

static mrb_value mrb_kum_sd_file_percent(mrb_state *mrb, mrb_value self)
{
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  if (file->last_address == 0) {
    return mrb_fixnum_value(0);
  } else {
    return mrb_fixnum_value(100 * file->pos / (file->last_address * 512));
  }
}

#define READ(n, o) do {\
  if (file->pos + n > end_offset) goto end;\
  if (!fread(buffer + (o), (n), 1, file->f)) IO_ERROR("");\
  file->pos += (n);\
} while (0)

#define BUFFER_LEN 65536

typedef struct {
  mrb_state *mrb;
  mrb_value block;
  mrb_value sample_buffer;
  mrb_sample_buffer *b;
  mrb_int pos;
} channel;

static inline void channel_push(channel *c, mrb_sample s)
{
  MRB_SAMPLE_BUFFER_DATA(c->b)[c->pos] = s;
  c->pos += 1;
  if (c->pos == BUFFER_LEN) {
    mrb_yield(c->mrb, c->block, c->sample_buffer);
    c->pos = 0;
  }
}

static inline void channel_flush(channel *c)
{
  int i;
  if (c->pos > 0) {
    i = mrb_gc_arena_save(c->mrb);
    mrb_yield(c->mrb, c->block, mrb_sample_buffer_slice(c->mrb, c->sample_buffer, c->pos, 0));
    c->pos = 0;
    mrb_gc_arena_restore(c->mrb, i);
  }
}

#define SCALE (1.0 / 2147483648.0)

enum {
  CHANNEL_TEMPERATURE = KUM_SD_MAX_CHANNEL_COUNT,
  CHANNEL_HUMIDITY,
  CHANNEL_VOLTAGE,
  CHANNEL_RTC_VOLTAGE,

  N_CHANNELS
};

static mrb_value mrb_kum_sd_file_process(mrb_state *mrb, mrb_value self)
{
  int i;
  kum_sd_file *file = mrb_data_check_get_ptr(mrb, self, &kum_sd_file_data_type);
  kum_sd_header start_header[1], end_header[1];
  mrb_value base_time;
  mrb_value argv[2];
  mrb_value block;
  mrb_bool block_given;
  mrb_int sample_number;
  mrb_int start_offset, end_offset;

  char buffer[512];

  channel channels[N_CHANNELS];

  mrb_get_args(mrb, "|&?", &block, &block_given);
  if (!block_given) return mrb_nil_value();

  if (!file && !file->f) mrb_raise(mrb, E_RUNTIME_ERROR, "File already closed");

  /* Rewind */
  if (fseek(file->f, 0, SEEK_SET) == -1) mrb_raise(mrb, E_RUNTIME_ERROR, "Seek failed");
  file->pos = 0;

  /* Read headers */
  if (!fread(buffer, 512, 1, file->f)) IO_ERROR(" (Start Header)");\
  file->pos += 512;
  if (kum_sd_header_read(start_header, buffer))
    mrb_raise(mrb, E_EXCEPTION, "Malformed start header");
  if (!fread(buffer, 512, 1, file->f)) IO_ERROR(" (End Header)");\
  file->pos += 512;
  if (kum_sd_header_read(end_header, buffer))
    mrb_raise(mrb, E_EXCEPTION, "Malformed end header");

  /* Calculate base time */
  if (bcd_valid((char *) start_header->start_time)) {
    base_time = mrb_funcall(mrb, mrb_obj_value(mrb_class_get(mrb, "Time")), "utc", 6,
      mrb_fixnum_value(bcd_int(start_header->start_time[BCD_YEAR]) + 2000),
      mrb_fixnum_value(bcd_int(start_header->start_time[BCD_MONTH])),
      mrb_fixnum_value(bcd_int(start_header->start_time[BCD_DAY])),
      mrb_fixnum_value(bcd_int(start_header->start_time[BCD_HOUR])),
      mrb_fixnum_value(bcd_int(start_header->start_time[BCD_MINUTE])),
      mrb_fixnum_value(bcd_int(start_header->start_time[BCD_SECOND])));
  } else {
    mrb_raise(mrb, E_EXCEPTION, "Invalid start time");
  }

  /* Initialize the channels */
  for (i = 0; i < N_CHANNELS; ++i) {
    channels[i].mrb = mrb;
    channels[i].block = block;
    channels[i].sample_buffer = mrb_sample_buffer_new_zero(mrb, BUFFER_LEN, 0);
    channels[i].b = mrb_sample_buffer_check(mrb, channels[i].sample_buffer);
    channels[i].pos = 0;
    if (i < start_header->channel_count) {
      channels[i].b->shared->channel = mrb_symbol_value(mrb_intern_cstr(mrb, (char *) start_header->channel_names[i]));
    }
  }

  start_offset = (mrb_int) start_header->address * 512;
  end_offset = (mrb_int) end_header->address * 512;

  /* Go to data */
  if (fseek(file->f, start_offset, SEEK_SET) == -1) mrb_raise(mrb, E_RUNTIME_ERROR, "Seek failed");
  file->pos = start_offset;

  sample_number = 0;

  while (file->pos < end_offset) {
    /* Read a frame */
    READ(4, 0);
    if (ld_i32_be(buffer) % 2) {
      /* Control frame */
      READ(12, 4);
      switch (ld_i32_be(buffer)) {
      case 1: /* Timestamp */
        i = mrb_gc_arena_save(mrb);
        argv[0] = mrb_fixnum_value(sample_number);
        argv[1] = mrb_funcall(mrb, base_time, "+", 1, mrb_float_value(mrb, ld_i32_be(buffer + 4)));
        mrb_yield(mrb, block, mrb_obj_new(mrb, mrb_class_get(mrb, "SampleTimestamp"), 2, argv));
        mrb_gc_arena_restore(mrb, i);
        break;
      case 7: /* Lost Frames */
        for (i = 0; i < N_CHANNELS; ++i) {
          channel_flush(&channels[i]);
        }
        sample_number += ld_u32_be(buffer + 10);
        break;
      case 13: /* End */
        goto end;
      case 15: /* Frame Number */
        if (sample_number != ld_u64_be(buffer + 4)) {
          for (i = 0; i < N_CHANNELS; ++i) {
            channel_flush(&channels[i]);
          }
          sample_number = ld_u64_be(buffer + 4);
        }
        break;
      default:
        break;
      }
    } else {
      /* Data frame */
      if (start_header->channel_count > 1) {
        READ(4 * (start_header->channel_count - 1), 4);
      }
      for (i = 0; i < start_header->channel_count; ++i) {
        channel_push(&channels[i], ld_i32_be(buffer + 4 * i) * SCALE);
      }
      sample_number += 1;
    }
  }
end:
  /* Flush remaining samples. */
  for (i = 0; i < N_CHANNELS; ++i) {
    channel_flush(&channels[i]);
  }
  return self;
}

void mrb_mruby_kumsd_gem_init(mrb_state* mrb)
{
  struct RClass *kumsd, *kumsdfile;
  /* KumSd */
  kumsd = mrb_define_module(mrb, "KumSd");
  mrb_define_module_function(mrb, kumsd, "parse_header", parse_header, MRB_ARGS_REQ(1));
  /* KumSd::KumSdFile */
  kumsdfile = mrb_define_class_under(mrb, kumsd, "KumSdFile", C_OBJECT);
  MRB_SET_INSTANCE_TT(kumsdfile, MRB_TT_DATA);
  mrb_define_method(mrb, kumsdfile, "initialize", mrb_kum_sd_file_open, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, kumsdfile, "close", mrb_kum_sd_file_close, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, kumsdfile, "read", mrb_kum_sd_file_read, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, kumsdfile, "goto_addr", mrb_kum_sd_file_goto_addr, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, kumsdfile, "read_frame", mrb_kum_sd_file_read_frame, MRB_ARGS_NONE());
  mrb_define_method(mrb, kumsdfile, "pos", mrb_kum_sd_file_pos, MRB_ARGS_NONE());
  mrb_define_method(mrb, kumsdfile, "percent", mrb_kum_sd_file_percent, MRB_ARGS_NONE());
  mrb_define_method(mrb, kumsdfile, "channel_count", mrb_kum_sd_file_channel_count, MRB_ARGS_NONE());
  mrb_define_method(mrb, kumsdfile, "channel_count=", mrb_kum_sd_file_channel_count_set, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, kumsdfile, "base_time", mrb_kum_sd_file_base_time, MRB_ARGS_NONE());
  mrb_define_method(mrb, kumsdfile, "base_time=", mrb_kum_sd_file_base_time_set, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, kumsdfile, "last_address", mrb_kum_sd_file_last_address, MRB_ARGS_NONE());
  mrb_define_method(mrb, kumsdfile, "last_address=", mrb_kum_sd_file_last_address_set, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, kumsdfile, "process", mrb_kum_sd_file_process, MRB_ARGS_BLOCK());
}

void mrb_mruby_kumsd_gem_final(mrb_state* mrb)
{
  /* finalizer */
}
