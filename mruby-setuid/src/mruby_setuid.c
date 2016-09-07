#include <mruby.h>
#include <mruby/value.h>
#include <unistd.h>

static mrb_value mrb_setuid(mrb_state *mrb, mrb_value self)
{
  mrb_int i;
  mrb_get_args(mrb, "i", &i);
  return mrb_fixnum_value(setuid(i));
}

static mrb_value mrb_getuid(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(getuid());
}

static mrb_value mrb_seteuid(mrb_state *mrb, mrb_value self)
{
  mrb_int i;
  mrb_get_args(mrb, "i", &i);
  return mrb_fixnum_value(seteuid(i));
}

static mrb_value mrb_geteuid(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(geteuid());
}

void mrb_mruby_setuid_gem_init(mrb_state *mrb)
{
  struct RClass *kernel = mrb_module_get(mrb, "Kernel");
  mrb_define_module_function(mrb, kernel, "setuid", mrb_setuid, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, kernel, "getuid", mrb_getuid, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, kernel, "seteuid", mrb_seteuid, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, kernel, "geteuid", mrb_geteuid, MRB_ARGS_NONE());
}

void mrb_mruby_setuid_gem_final(mrb_state *mrb)
{

}
