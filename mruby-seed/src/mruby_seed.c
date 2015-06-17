#include <math.h>
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <stdio.h>
#include <string.h>
#include <sample_buffer.h>

#include "number.h"
#include "seed.h"

static seed_btime_t mrb_time2btime(mrb_state *mrb, mrb_value t)
{
  mrb_value tt, i;
  seed_btime_t bt;
  if (!(mrb_obj_is_kind_of(mrb, t, mrb_class_get(mrb, "Time")))) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Not a Time");
  }
  tt = mrb_funcall(mrb, t, "utc", 0, 0);
  i = mrb_funcall(mrb, tt, "year", 0, 0);
  bt.year = mrb_fixnum(i);
  i = mrb_funcall(mrb, tt, "yday", 0, 0);
  bt.julian_day = mrb_fixnum(i);
  i = mrb_funcall(mrb, tt, "hour", 0, 0);
  bt.hour = mrb_fixnum(i);
  i = mrb_funcall(mrb, tt, "min", 0, 0);
  bt.minute = mrb_fixnum(i);
  i = mrb_funcall(mrb, tt, "sec", 0, 0);
  bt.second = mrb_fixnum(i);
  i = mrb_funcall(mrb, tt, "usec", 0, 0);
  bt.tenth_ms = (mrb_fixnum(i) + 50) / 100;
  return bt;
}

static mrb_value mrb_btime2time(mrb_state *mrb, const seed_btime_t *bt)
{
  mrb_value t;
  t = mrb_funcall(mrb, mrb_obj_value(mrb_class_get(mrb, "Time")), "utc", 7,
    mrb_fixnum_value(bt->year),
    mrb_fixnum_value(1),
    mrb_fixnum_value(1),
    mrb_fixnum_value(bt->hour),
    mrb_fixnum_value(bt->minute),
    mrb_fixnum_value(bt->second),
    mrb_fixnum_value(bt->tenth_ms * 100));
  t = mrb_funcall(mrb, t, "+", 1, mrb_float_value(mrb, bt->julian_day * 86400.0));
  return t;
}

typedef struct {
  mrb_int size;
  seed_data_record_header_t header;
  uint8_t data[1];
} data_record;

static data_record *mrb_data_record_alloc(mrb_state *mrb, mrb_int size)
{
  data_record *r = mrb_malloc(mrb, sizeof(data_record) + size);
  memset(r->data, 0, size);
  r->size = size;
  memset(&r->header, 0, sizeof(r->header));
  memcpy(r->header.station_identifier, "     ", 6);
  memcpy(r->header.location_identifier, "  ", 3);
  memcpy(r->header.channel_identifier, "   ", 4);
  memcpy(r->header.network_code, "  ", 3);
  r->header.data_offset = 0x30;
  r->header.blockette_offset = 0x30;
  r->header.sample_rate_multiplier = 1;
  seed_write_data_record_header(r->data, &r->header);
  return r;
}

static const mrb_data_type data_record_type = {"DataRecord", mrb_free};

static mrb_value mrb_data_record_initialize(mrb_state *mrb, mrb_value self)
{
  data_record *r = 0;

  if ((r = DATA_PTR(self))) {
    mrb_free(mrb, r);
  }
  mrb_data_init(self, 0, &data_record_type);

  r = mrb_data_record_alloc(mrb, 4096);
  mrb_data_init(self, r, &data_record_type);
  return self;
}

static mrb_value mrb_data_record_set_start_time(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  mrb_value t;
  mrb_float f = 0;
  mrb_get_args(mrb, "o|f", &t, &f);
  if (f * 1e4 > INT32_MAX || f * 1e4 < INT32_MIN) {
    mrb_raise(mrb, E_RANGE_ERROR, "Correction is too large");
  }
  r->header.start_time = mrb_time2btime(mrb, t);
  r->header.time_correction = f * 1e4;
  seed_write_data_record_header(r->data, &r->header);
  return self;
}

static mrb_value mrb_data_record_add_blockette(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  char *s;
  mrb_int slen;
  if (r->header.num_samples) mrb_raise(mrb, E_RUNTIME_ERROR, "Already have samples");
  mrb_get_args(mrb, "s", &s, &slen);
  memcpy(r->data + r->header.data_offset, s, slen);
  r->header.data_offset += (slen + 15) & (~15);
  r->header.blockette_count += 1;
  seed_write_data_record_header(r->data, &r->header);
  return self;
}

static mrb_value mrb_data_record_to_s(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  return mrb_str_new(mrb, (char *) r->data, r->size);
}

static mrb_value mrb_data_record_sequence_number(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  return mrb_fixnum_value(r->header.sequence_number);
}

static mrb_value mrb_data_record_sequence_number_set(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  mrb_int i;
  mrb_get_args(mrb, "i", &i);
  if (i < 0 || i > 999999) mrb_raise(mrb, E_RANGE_ERROR, "Sequence number out of range");
  r->header.sequence_number = i;
  seed_write_data_record_header(r->data, &r->header);
  return mrb_fixnum_value(i);
}

static mrb_value mrb_data_record_sample_rate(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  return mrb_fixnum_value(r->header.sample_rate_factor);
}

static mrb_value mrb_data_record_sample_rate_set(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  mrb_int i;
  mrb_get_args(mrb, "i", &i);
  if (i < 1 || i > 32767) mrb_raise(mrb, E_RANGE_ERROR, "Sample rate out of range");
  r->header.sample_rate_factor = i;
  seed_write_data_record_header(r->data, &r->header);
  return mrb_fixnum_value(i);
}

static mrb_value mrb_data_record_channel_name(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  mrb_int l = 0;
  while (r->header.channel_identifier[l] != ' ' && r->header.channel_identifier[l] != 0) {
    ++l;
  }
  return mrb_str_new(mrb, (char *) r->header.channel_identifier, l);
}

static mrb_value mrb_data_record_channel_name_set(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  char *s;
  mrb_int slen, i;
  mrb_get_args(mrb, "s", &s, &slen);
  if (slen > 3) mrb_raise(mrb, E_ARGUMENT_ERROR, "Channel name must not be longer than 3 characters");
  for (i = 0; i < slen; ++i) {
    if (s[i] == ' ') mrb_raise(mrb, E_ARGUMENT_ERROR, "Channel name can not contain spaces");
  }
  memcpy(r->header.channel_identifier, "   ", 4);
  memcpy(r->header.channel_identifier, s, slen);
  seed_write_data_record_header(r->data, &r->header);
  return mrb_str_new(mrb, s, slen);
}

static mrb_value mrb_data_record_push(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  mrb_int i;
  mrb_get_args(mrb, "i", &i);
  if (i < INT32_MIN || i > INT32_MAX)
    mrb_raise(mrb, E_RANGE_ERROR, "Sample out of range");
  if (r->header.num_samples >= (r->size - r->header.data_offset) / 4)
    mrb_raise(mrb, E_RUNTIME_ERROR, "DataRecord is full");
  st_i32_be(r->data + r->header.data_offset + r->header.num_samples * 4, i);
  r->header.num_samples += 1;
  st_u16_be(r->data + 30, r->header.num_samples);
  return self;
}

#define SCALE (2147483648.0)

static mrb_value mrb_data_record_push_sample_buffer(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  mrb_value buffer_value;
  mrb_sample_buffer *buffer;
  mrb_int i, l;
  mrb_int m = (r->size - r->header.data_offset) / 4;
  mrb_sample x;

  mrb_get_args(mrb, "o", &buffer_value);
  if (!(buffer = mrb_sample_buffer_check(mrb, buffer_value)))
    mrb_raise(mrb, E_RANGE_ERROR, "Not a SampleBuffer");

  if (r->header.num_samples >= m)
    return mrb_fixnum_value(0);

  l = MRB_SAMPLE_BUFFER_LEN(buffer);

  i = 0;
  while (r->header.num_samples < m && i < l) {
    x = MRB_SAMPLE_BUFFER_DATA(buffer)[i] * SCALE;
    if (x >= SCALE) {
      st_i32_be(r->data + r->header.data_offset + r->header.num_samples * 4, INT32_MAX);
    } else if (x < -SCALE) {
      st_i32_be(r->data + r->header.data_offset + r->header.num_samples * 4, INT32_MIN);
    } else {
      st_i32_be(r->data + r->header.data_offset + r->header.num_samples * 4, lround(x));
    }
    r->header.num_samples += 1;
    i += 1;
  }

  st_u16_be(r->data + 30, r->header.num_samples);

  return mrb_fixnum_value(i);
}

static mrb_value mrb_data_record_full(mrb_state *mrb, mrb_value self)
{
  data_record *r = DATA_CHECK_GET_PTR(mrb, self, &data_record_type, data_record);
  if (r->header.num_samples >= (r->size - r->header.data_offset) / 4) {
    return mrb_true_value();
  } else {
    return mrb_false_value();
  }
}

void mrb_mruby_seed_gem_init(mrb_state* mrb)
{
  struct RClass *seed_module, *data_record_class;
  /* Seed */
  seed_module = mrb_define_module(mrb, "Seed");
  /* Seed::DataRecord */
  data_record_class = mrb_define_class_under(mrb, seed_module, "DataRecord", mrb->object_class);
  MRB_SET_INSTANCE_TT(data_record_class, MRB_TT_DATA);
  mrb_define_method(mrb, data_record_class, "initialize", mrb_data_record_initialize, MRB_ARGS_NONE());
  mrb_define_method(mrb, data_record_class, "add_blockette", mrb_data_record_add_blockette, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, data_record_class, "set_start_time", mrb_data_record_set_start_time, MRB_ARGS_ARG(1, 1));
  mrb_define_method(mrb, data_record_class, "sequence_number", mrb_data_record_sequence_number, MRB_ARGS_NONE());
  mrb_define_method(mrb, data_record_class, "sequence_number=", mrb_data_record_sequence_number_set, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, data_record_class, "sample_rate", mrb_data_record_sample_rate, MRB_ARGS_NONE());
  mrb_define_method(mrb, data_record_class, "sample_rate=", mrb_data_record_sample_rate_set, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, data_record_class, "channel_name", mrb_data_record_channel_name, MRB_ARGS_NONE());
  mrb_define_method(mrb, data_record_class, "channel_name=", mrb_data_record_channel_name_set, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, data_record_class, "push", mrb_data_record_push, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, data_record_class, "<<", mrb_data_record_push, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, data_record_class, "full", mrb_data_record_full, MRB_ARGS_NONE());
  mrb_define_method(mrb, data_record_class, "push_sample_buffer", mrb_data_record_push_sample_buffer, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, data_record_class, "to_s", mrb_data_record_to_s, MRB_ARGS_NONE());
}

void mrb_mruby_seed_gem_final(mrb_state* mrb)
{
  /* finalizer */
}
