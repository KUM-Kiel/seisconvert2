#include <mruby.h>
#include <mruby/value.h>

int main(int argc, char **argv)
{
  mrb_state *mrb = mrb_open();
  mrb_value file = (argc > 1 ? mrb_str_new_cstr(mrb, argv[1]) : mrb_nil_value());
  struct RClass *makeseed = mrb_class_get(mrb, "MakeZft");
  mrb_value o = mrb_obj_new(mrb, makeseed, 0, 0);
  mrb_funcall(mrb, o, "run", 1, file);
  mrb_close(mrb);
  return 0;
}
