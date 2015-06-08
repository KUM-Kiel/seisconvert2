#ifndef MRUBY_SAMPLE_BUFFER_H
#define MRUBY_SAMPLE_BUFFER_H

#include <mruby.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Use double for sample values. */
#define MRB_SAMPLE_DOUBLE 1
typedef double mrb_sample;

typedef struct {
  int refcnt;
  mrb_value channel;
  mrb_int len;
  mrb_int offset;
  mrb_sample buffer[1];
} mrb_shared_sample_buffer;

typedef struct {
  mrb_int len;
  mrb_int offset;
  mrb_shared_sample_buffer *shared;
} mrb_sample_buffer;

MRB_API mrb_sample_buffer *mrb_sample_buffer_check(mrb_state *mrb, mrb_value b);

#define MRB_SAMPLE_BUFFER_LEN(a) ((a)->len)
#define MRB_SAMPLE_BUFFER_OFFSET(a) ((a)->offset + (a)->shared->offset)
#define MRB_SAMPLE_BUFFER_DATA(a) ((a)->shared->buffer + (a)->offset)

MRB_API mrb_value mrb_sample_buffer_new(mrb_state *mrb, mrb_int len, mrb_int offset, const mrb_sample *data);
MRB_API mrb_value mrb_sample_buffer_new_zero(mrb_state *mrb, mrb_int len, mrb_int offset);
MRB_API mrb_value mrb_sample_buffer_new_uninitialized(mrb_state *mrb, mrb_int len, mrb_int offset);
MRB_API mrb_value mrb_sample_buffer_slice(mrb_state *mrb, mrb_value sample_buffer, mrb_int offset, mrb_int len);

static inline mrb_int
mrb_sample_buffer_len(mrb_state *mrb, mrb_value sample_buffer)
{
  mrb_sample_buffer *b = mrb_sample_buffer_check(mrb, sample_buffer);
  return MRB_SAMPLE_BUFFER_LEN(b);
}

static inline mrb_int
mrb_sample_buffer_offset(mrb_state *mrb, mrb_value sample_buffer)
{
  mrb_sample_buffer *b = mrb_sample_buffer_check(mrb, sample_buffer);
  return MRB_SAMPLE_BUFFER_OFFSET(b);
}

#if defined(__cplusplus)
}
#endif

#endif
