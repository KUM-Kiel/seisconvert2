#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

typedef struct {
  mrb_int length;
  mrb_int capacity;
  double data[1];
} sample_buffer;

static samble_buffer *mrb_samble_buffer_alloc(mrb_state *mrb, mrb_int capacity)
{
  samble_buffer *b = mrb_malloc(mrb, sizeof(samble_buffer) + sizeof(double) * (capacity - 1));
  b->length = 0;
  b->capacity = capacity;
  return b;
}

static const mrb_data_type sample_buffer_type = {"SampleBuffer", mrb_free};

static mrb_value mrb_samble_buffer_initialize(mrb_state *mrb, mrb_value self)
{
  samble_buffer *b = 0;
  mrb_int capacity;
  mrb_get_args(mrb, "i", &capacity);

  if ((b = DATA_PTR(self))) {
    mrb_free(mrb, b);
  }
  mrb_data_init(self, 0, &sample_buffer_type);

  b = mrb_samble_buffer_alloc(mrb, capacity);
  mrb_data_init(self, b, &sample_buffer_type);
  return self;
}

void mrb_mruby_sample_buffer_gem_init(mrb_state* mrb)
{
  struct RClass *samble_buffer_class;
  samble_buffer_class = mrb_define_class(mrb, "SampleBuffer", mrb->object_class);
  MRB_SET_INSTANCE_TT(samble_buffer_class, MRB_TT_DATA);
  mrb_define_method(mrb, kumsdfile, "initialize", mrb_samble_buffer_initialize, MRB_ARGS_REQ(1));
}

void mrb_mruby_sample_buffer_gem_final(mrb_state* mrb)
{
  /* finalizer */
}
