#define _FILE_OFFSET_BITS 64
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <sample_buffer.h>
#include <stdio.h>
#include <string.h>
#include "number.h"

#define MAX_CHANNELS 8

typedef struct {
  FILE *handle;
  int channel_count;
  mrb_sym channels[MAX_CHANNELS];
  int headers_written;
} zft_file;

static void zft_destroy(mrb_state *mrb, void *p)
{
  zft_file *file = p;
  if (file) {
    if (file->handle) {
      fclose(file->handle);
    }
    mrb_free(mrb, file);
  }
}

static const mrb_data_type zft_type[1] = {{"Zft", zft_destroy}};

zft_file *zft_check(mrb_state *mrb, mrb_value v)
{
  zft_file *file = mrb_data_check_get_ptr(mrb, v, zft_type);
  if (file && file->handle) {
    return file;
  } else {
    mrb_raise(mrb, E_RUNTIME_ERROR, "Zft file already closed");
  }
}

static mrb_value zft_initialize(mrb_state *mrb, mrb_value self)
{
  char *filename;
  zft_file *file;

  mrb_get_args(mrb, "z", &filename);

  file = mrb_malloc(mrb, sizeof(*file));
  file->channel_count = 0;
  file->headers_written = 0;

  file->handle = fopen(filename, "wb");
  if (!file->handle) {
    mrb_free(mrb, file);
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not open zft file");
  }

  mrb_data_init(self, file, zft_type);

  return self;
}

static mrb_value zft_close(mrb_state *mrb, mrb_value self)
{
  zft_file *file = zft_check(mrb, self);
  fclose(file->handle);
  file->handle = 0;
  return self;
}

static const char header_template[3600] =
  "C 1                                                                             "
  "C 2                                                                             "
  "C 3                                                                             "
  "C 4                                                                             "
  "C 5                                                                             "
  "C 6                                                                             "
  "C 7                                                                             "
  "C 8                                                                             "
  "C 9                                                                             "
  "C10                                                                             "
  "C11                                                                             "
  "C12                                                                             "
  "C13                                                                             "
  "C14                                                                             "
  "C15                                                                             "
  "C16                                                                             "
  "C17                                                                             "
  "C18                                                                             "
  "C19                                                                             "
  "C20                                                                             "
  "C21                                                                             "
  "C22                                                                             "
  "C23 START OF KUM HEADER                                                         "
  "C24 START RECORDING TIME                                                        "
  "C25 STOP RECORDING TIME                                                         "
  "C26 SYNCHRONISATION TIME                                                        "
  "C27 SKEW TIME                                                                   "
  "C28 SKEW IN MICROSECS                                                           "
  "C29 SAMPLE PERIOD IN MICROSECS                                                  "
  "C30 CHANNEL NO                          TOTAL NO OF CHANNELS                    "
  "C31 CHANNEL GAIN                                                                "
  "C32 STATION NUMBER                      STAT NAME                               "
  "C33 STATION COMMENT                                                             "
  "C34 EXPERIMENT NO                       EXP NAME                                "
  "C35 EXP COMMENT                                                                 "
  "C36 LOGGER SPEC                                                                 "
  "C37 LOGGER SPEC                                                                 "
  "C38 LOGGER SPEC                                                                 "
  "C39 SEG Y REV1                                                                  "
  "C40 END TEXTUAL HEADER                                                          "
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\001\000\000\000\000\000\000"
  "\000\000\000\000\000\002\000\001\000\001\000\001\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001\000\001\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\001\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000";

static mrb_value zft_add_channel(mrb_state *mrb, mrb_value self)
{
  zft_file *file = zft_check(mrb, self);
  mrb_sym channel_name;
  mrb_value options;
  mrb_bool options_given;
  char header[3600], buffer[128];
  mrb_value v;
  double x;
  int i;

  if (file->headers_written)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Headers have already been written");

  if (file->channel_count >= MAX_CHANNELS)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Can not add more channels");

  mrb_get_args(mrb, "n|H?", &channel_name, &options, &options_given);

  file->channels[file->channel_count] = channel_name;
  file->channel_count += 1;

  memcpy(header, header_template, 3600);

  if (options_given) {
    v = mrb_hash_get(mrb, options, mrb_symbol_value(mrb_intern_lit(mrb, "rate")));
    if (mrb_fixnum_p(v)) {
      x = 1000000 / mrb_fixnum(v);
      st_i32_be(header + 3216, x);
      i = snprintf(buffer, sizeof(buffer), "%08d", (int) x);
      memcpy(header + 2271, buffer, i);
    }
    v = mrb_hash_get(mrb, options, mrb_symbol_value(mrb_intern_lit(mrb, "start_time")));
    if (mrb_obj_is_kind_of(mrb, v, mrb_class_get(mrb, "Time"))) {
      i = 0;
      i += snprintf(buffer + i, sizeof(buffer) - i, "%04d.",
        (int) mrb_fixnum(mrb_funcall(mrb, v, "year", 0)));
      i += snprintf(buffer + i, sizeof(buffer) - i, "%03d.",
        (int) mrb_fixnum(mrb_funcall(mrb, v, "yday", 0)));
      i += snprintf(buffer + i, sizeof(buffer) - i, "%02d.",
        (int) mrb_fixnum(mrb_funcall(mrb, v, "hour", 0)));
      i += snprintf(buffer + i, sizeof(buffer) - i, "%02d.",
        (int) mrb_fixnum(mrb_funcall(mrb, v, "min", 0)));
      i += snprintf(buffer + i, sizeof(buffer) - i, "%02d.",
        (int) mrb_fixnum(mrb_funcall(mrb, v, "sec", 0)));
      memcpy(header + 1871, buffer, i);
      x = (mrb_fixnum(mrb_funcall(mrb, v, "year", 0)) * 512 +
        mrb_fixnum(mrb_funcall(mrb, v, "yday", 0))) * 2048;
      st_u32_be(header + 3200, x);
    }
    v = mrb_hash_get(mrb, options, mrb_symbol_value(mrb_intern_lit(mrb, "channel_number")));
    if (mrb_fixnum_p(v)) {
      x = mrb_fixnum(v);
      st_u32_be(header + 3208, x);
      i = snprintf(buffer, sizeof(buffer), "%03d", (int) x);
      memcpy(header + 2351, buffer, i);
    }
    v = mrb_hash_get(mrb, options, mrb_symbol_value(mrb_intern_lit(mrb, "channel_count")));
    if (mrb_fixnum_p(v)) {
      x = mrb_fixnum(v);
      i = snprintf(buffer, sizeof(buffer), "%03d", (int) x);
      memcpy(header + 2381, buffer, i);
    }
    v = mrb_hash_get(mrb, options, mrb_symbol_value(mrb_intern_lit(mrb, "frame_count")));
    if (mrb_fixnum_p(v)) {
      x = mrb_fixnum(v);
      st_u32_be(header + 3220, x);
    }
  }

  if (fwrite(header, 3600, 1, file->handle) != 1)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not write header");

  return self;
}

static mrb_value zft_push_frame(mrb_state *mrb, mrb_value self)
{
  zft_file *file = zft_check(mrb, self);
  mrb_value *a;
  mrb_int alen;
  char frame[4 * MAX_CHANNELS];
  int i;

  if (file->channel_count == 0)
    mrb_raise(mrb, E_RUNTIME_ERROR, "No channels added");

  mrb_get_args(mrb, "a", &a, &alen);

  if (alen != file->channel_count)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Wrong sample count");

  for (i = 0; i < file->channel_count; ++i) {
    if (!mrb_fixnum_p(a[i]))
      mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid sample format");
    st_i32_be(frame + i * 4, mrb_fixnum(a[i]));
  }

  file->headers_written = 1;

  if (fwrite(frame, 4 * file->channel_count, 1, file->handle) != 1)
    mrb_raise(mrb, E_RUNTIME_ERROR, "IO Error");

  return self;
}

void mrb_mruby_zft_gem_init(mrb_state *mrb)
{
  struct RClass *zft;
  zft = mrb_define_class(mrb, "Zft", mrb->object_class);
  MRB_SET_INSTANCE_TT(zft, MRB_TT_DATA);
  mrb_define_method(mrb, zft, "initialize", zft_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, zft, "close", zft_close, MRB_ARGS_NONE());
  mrb_define_method(mrb, zft, "add_channel", zft_add_channel, MRB_ARGS_ARG(1, 1));
  mrb_define_method(mrb, zft, "push_frame", zft_push_frame, MRB_ARGS_REQ(1));
}

void mrb_mruby_zft_gem_final(mrb_state *mrb)
{

}
