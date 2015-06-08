#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/value.h>
#include <sample_buffer.h>

static void mrb_shared_sample_buffer_ref(mrb_state *mrb, mrb_shared_sample_buffer *b)
{
  (void) mrb;
  if (!b) return;
  b->refcnt += 1;
}

static void mrb_shared_sample_buffer_unref(mrb_state *mrb, mrb_shared_sample_buffer *b)
{
  if (!b) return;
  b->refcnt -= 1;
  if (b->refcnt <= 0) {
    mrb_free(mrb, b);
  }
}

static mrb_shared_sample_buffer *mrb_shared_sample_buffer_alloc(mrb_state *mrb, mrb_int len)
{
  mrb_shared_sample_buffer *sb = mrb_malloc(mrb, sizeof(mrb_shared_sample_buffer) + sizeof(mrb_sample) * (len - 1));
  sb->refcnt = 1;
  sb->len = len;
  sb->offset = 0;
  sb->channel = mrb_nil_value();
  return sb;
}

static mrb_sample_buffer *mrb_sample_buffer_alloc(mrb_state *mrb)
{
  mrb_sample_buffer *b = mrb_malloc(mrb, sizeof(mrb_sample_buffer));
  b->len = 0;
  b->offset = 0;
  b->shared = 0;
  return b;
}

static void mrb_sample_buffer_destroy(mrb_state *mrb, void *x)
{
  mrb_sample_buffer *b = x;
  if (b) {
    mrb_shared_sample_buffer_unref(mrb, b->shared);
    mrb_free(mrb, b);
  }
}

static const mrb_data_type mrb_sample_buffer_type = {"SampleBuffer", mrb_sample_buffer_destroy};

MRB_API mrb_sample_buffer *mrb_sample_buffer_check(mrb_state *mrb, mrb_value b)
{
  return mrb_data_check_get_ptr(mrb, b, &mrb_sample_buffer_type);
}

static mrb_value mrb_sample_buffer_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_sample_buffer *b = 0;
  mrb_int len, offset = 0, i;
  mrb_get_args(mrb, "i|i", &len, &offset);

  if (len <= 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Length must be positive");
  if (offset < 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Offset can not be negative");

  b = mrb_sample_buffer_alloc(mrb);
  b->shared = mrb_shared_sample_buffer_alloc(mrb, len);
  b->shared->offset = offset;
  b->len = len;
  b->offset = 0;
  for (i = 0; i < len; ++i) {
    b->shared->buffer[i] = 0;
  }
  mrb_data_init(self, b, &mrb_sample_buffer_type);
  return self;
}

MRB_API mrb_value mrb_sample_buffer_new_zero(mrb_state *mrb, mrb_int len, mrb_int offset)
{
  mrb_value buf;
  mrb_sample_buffer *b = 0;
  mrb_int i;

  if (len <= 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Length must be positive");
  if (offset < 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Offset can not be negative");

  buf = mrb_obj_value(mrb_obj_alloc(mrb, MRB_TT_DATA, mrb_class_get(mrb, "SampleBuffer")));

  b = mrb_sample_buffer_alloc(mrb);
  b->shared = mrb_shared_sample_buffer_alloc(mrb, len);
  b->shared->offset = offset;
  b->len = len;
  b->offset = 0;
  for (i = 0; i < len; ++i) {
    b->shared->buffer[i] = 0;
  }
  mrb_data_init(buf, b, &mrb_sample_buffer_type);

  return buf;
}

MRB_API mrb_value mrb_sample_buffer_new_uninitialized(mrb_state *mrb, mrb_int len, mrb_int offset)
{
  mrb_value buf;
  mrb_sample_buffer *b = 0;

  if (len <= 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Length must be positive");
  if (offset < 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Offset can not be negative");

  buf = mrb_obj_value(mrb_obj_alloc(mrb, MRB_TT_DATA, mrb_class_get(mrb, "SampleBuffer")));

  b = mrb_sample_buffer_alloc(mrb);
  b->shared = mrb_shared_sample_buffer_alloc(mrb, len);
  b->shared->offset = offset;
  b->len = len;
  b->offset = 0;
  mrb_data_init(buf, b, &mrb_sample_buffer_type);

  return buf;
}

MRB_API mrb_value mrb_sample_buffer_new(mrb_state *mrb, mrb_int len, mrb_int offset, const mrb_sample *data)
{
  mrb_value buf;
  mrb_sample_buffer *b = 0;
  mrb_int i;

  if (len <= 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Length must be positive");
  if (offset < 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Offset can not be negative");

  buf = mrb_obj_value(mrb_obj_alloc(mrb, MRB_TT_DATA, mrb_class_get(mrb, "SampleBuffer")));

  b = mrb_sample_buffer_alloc(mrb);
  b->shared = mrb_shared_sample_buffer_alloc(mrb, len);
  b->shared->offset = offset;
  b->len = len;
  b->offset = 0;
  for (i = 0; i < len; ++i) {
    b->shared->buffer[i] = data[i];
  }
  mrb_data_init(buf, b, &mrb_sample_buffer_type);

  return buf;
}

MRB_API mrb_value mrb_sample_buffer_slice(mrb_state *mrb, mrb_value sample_buffer, mrb_int offset, mrb_int len)
{
  mrb_value buf;
  mrb_sample_buffer *b = 0, *other = 0;

  if (len <= 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Length must be positive");
  if (offset < 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Offset can not be negative");

  other = mrb_sample_buffer_check(mrb, sample_buffer);

  if (len > other->len - offset) mrb_raise(mrb, E_ARGUMENT_ERROR, "Slice must be completely inside the buffer");

  buf = mrb_obj_value(mrb_obj_alloc(mrb, MRB_TT_DATA, mrb_class_get(mrb, "SampleBuffer")));

  b = mrb_sample_buffer_alloc(mrb);
  b->shared = other->shared;
  mrb_shared_sample_buffer_ref(mrb, other->shared);
  b->len = len;
  b->offset = offset;
  mrb_data_init(buf, b, &mrb_sample_buffer_type);

  return buf;
}

static mrb_value mrb_sample_buffer_length_method(mrb_state *mrb, mrb_value self)
{
  mrb_sample_buffer *b;

  b = mrb_data_check_get_ptr(mrb, self, &mrb_sample_buffer_type);

  return mrb_fixnum_value(MRB_SAMPLE_BUFFER_LEN(b));
}

static mrb_value mrb_sample_buffer_offset_method(mrb_state *mrb, mrb_value self)
{
  mrb_sample_buffer *b;

  b = mrb_data_check_get_ptr(mrb, self, &mrb_sample_buffer_type);

  if (!b->shared) {
    return mrb_fixnum_value(b->offset);
  } else {
    return mrb_fixnum_value(b->offset + b->shared->offset);
  }
}

static mrb_value mrb_sample_buffer_get_method(mrb_state *mrb, mrb_value self)
{
  mrb_int i;
  mrb_sample_buffer *b;

  b = mrb_data_check_get_ptr(mrb, self, &mrb_sample_buffer_type);

  mrb_get_args(mrb, "i", &i);

  if (0 <= i && i < b->len) {
    return mrb_float_value(mrb, MRB_SAMPLE_BUFFER_DATA(b)[i]);
  } else {
    return mrb_nil_value();
  }
}

static mrb_value mrb_sample_buffer_set_method(mrb_state *mrb, mrb_value self)
{
  mrb_int i;
  mrb_float f;
  mrb_sample_buffer *b = mrb_data_check_get_ptr(mrb, self, &mrb_sample_buffer_type);

  mrb_get_args(mrb, "if", &i, &f);

  if (0 <= i && i < b->len) {
    MRB_SAMPLE_BUFFER_DATA(b)[i] = f;
    return mrb_float_value(mrb, f);
  } else {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Index out of bounds");
  }
}

static mrb_value mrb_sample_buffer_each_method(mrb_state *mrb, mrb_value self)
{
  mrb_int i;
  mrb_value block;
  mrb_bool block_given;
  mrb_sample_buffer *b = mrb_data_check_get_ptr(mrb, self, &mrb_sample_buffer_type);

  mrb_get_args(mrb, "|&?", &block, &block_given);

  if (block_given) {
    for (i = 0; i < b->len; ++i) {
      mrb_yield(mrb, block, mrb_float_value(mrb, MRB_SAMPLE_BUFFER_DATA(b)[i]));
    }
    return self;
  } else {
    return mrb_funcall(mrb, self, "enum_for", 1, mrb_symbol_value(mrb_intern_lit(mrb, "each")));
  }
}

static mrb_value mrb_sample_buffer_slice_method(mrb_state *mrb, mrb_value self)
{
  mrb_int len, offset = 0;

  mrb_get_args(mrb, "i|i", &len, &offset);

  if (len <= 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Length must be positive");

  return mrb_sample_buffer_slice(mrb, self, len, offset);
}

static mrb_value mrb_sample_buffer_channel_get_method(mrb_state *mrb, mrb_value self)
{
  mrb_sample_buffer *b = mrb_data_check_get_ptr(mrb, self, &mrb_sample_buffer_type);

  if (b->shared) {
    return b->shared->channel;
  } else {
    return mrb_nil_value();
  }
}

static mrb_value mrb_sample_buffer_channel_set_method(mrb_state *mrb, mrb_value self)
{
  mrb_sample_buffer *b = mrb_data_check_get_ptr(mrb, self, &mrb_sample_buffer_type);

  if (b->shared) {
    mrb_get_args(mrb, "o", &(b->shared->channel));
    return b->shared->channel;
  } else {
    return mrb_nil_value();
  }
}

void mrb_mruby_sample_buffer_gem_init(mrb_state* mrb)
{
  struct RClass *sample_buffer_class;
  sample_buffer_class = mrb_define_class(mrb, "SampleBuffer", mrb->object_class);
  MRB_SET_INSTANCE_TT(sample_buffer_class, MRB_TT_DATA);
  mrb_include_module(mrb, sample_buffer_class, mrb_module_get(mrb, "Enumerable"));
  mrb_define_method(mrb, sample_buffer_class, "initialize", mrb_sample_buffer_initialize, MRB_ARGS_ARG(1, 1));
  mrb_define_method(mrb, sample_buffer_class, "length", mrb_sample_buffer_length_method, MRB_ARGS_NONE());
  mrb_define_method(mrb, sample_buffer_class, "size", mrb_sample_buffer_length_method, MRB_ARGS_NONE());
  mrb_define_method(mrb, sample_buffer_class, "offset", mrb_sample_buffer_offset_method, MRB_ARGS_NONE());
  mrb_define_method(mrb, sample_buffer_class, "[]", mrb_sample_buffer_get_method, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, sample_buffer_class, "[]=", mrb_sample_buffer_set_method, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, sample_buffer_class, "each", mrb_sample_buffer_each_method, MRB_ARGS_BLOCK());
  mrb_define_method(mrb, sample_buffer_class, "slice", mrb_sample_buffer_slice_method, MRB_ARGS_ARG(1, 1));
  mrb_define_method(mrb, sample_buffer_class, "channel", mrb_sample_buffer_channel_get_method, MRB_ARGS_REQ(0));
  mrb_define_method(mrb, sample_buffer_class, "channel=", mrb_sample_buffer_channel_set_method, MRB_ARGS_REQ(1));
}

void mrb_mruby_sample_buffer_gem_final(mrb_state* mrb)
{
  /* finalizer */
}
