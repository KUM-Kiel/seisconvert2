#include <errno.h>
#include <mruby.h>
#include <mruby/value.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>

static int mkdir_p(const char *path)
{
  char p[1024] = {0}, *x;
  int o = 0;
  while ((x = index(path + o, '/'))) {
    o = x - path;
    if (o > sizeof(p)) return -1;
    memcpy(p, path, o);
    p[o] = 0;
    if (mkdir(p, 0755) < 0) {
      if (errno != EEXIST)
        return -1;
    }
    o += 1;
  }
  if (mkdir(path, 0755) < 0) {
    if (errno != EEXIST)
      return -1;
  }
  return 0;
}

static mrb_value mrb_mkdir_p(mrb_state *mrb, mrb_value self)
{
  char *s;
  mrb_get_args(mrb, "z", &s);
  if (mkdir_p(s) < 0)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not create folder");
  return mrb_nil_value();
}

static mrb_value mrb_mkdir(mrb_state *mrb, mrb_value self)
{
  char *s;
  mrb_get_args(mrb, "z", &s);
  if (mkdir(s, 0755) < 0)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not create folder");
  return mrb_nil_value();
}

void mrb_mruby_mkdir_gem_init(mrb_state *mrb)
{
  struct RClass *kernel = mrb_module_get(mrb, "Kernel");
  mrb_define_module_function(mrb, kernel, "mkdir", mrb_mkdir, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, kernel, "mkdir_p", mrb_mkdir_p, MRB_ARGS_REQ(1));
}

void mrb_mruby_mkdir_gem_final(mrb_state *mrb)
{

}
