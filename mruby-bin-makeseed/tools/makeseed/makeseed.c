#include <mruby.h>
#include <mruby/value.h>

int main(int argc, char **argv)
{
  mrb_state *mrb = mrb_open();
  mrb_value file = (argc > 1 ? mrb_str_new_cstr(mrb, argv[1]) : mrb_nil_value());
  struct RClass *makeseed = mrb_module_get(mrb, "Makeseed");
  mrb_funcall(mrb, mrb_obj_value(makeseed), "run", 1, file);
  return 0;
}
