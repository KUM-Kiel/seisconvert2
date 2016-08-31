#include <mruby.h>
#include <mruby/hash.h>
#include <mruby/value.h>
#include <stdio.h>
#include <stdlib.h>
#include "options.h"

static const char* program = "makeseed";
static void help(const char *x)
{
  fprintf(stderr,
    "\n"
    "Makeseed reads a KUM SD card and creates a MiniSEED file for each channel.\n"
    "\n"
    "# Usage\n"
    "\n"
    "  %s [options] /dev/sdX\n"
    "\n"
    "  /dev/sdX should be replaced with the SD card device file.\n"
    "\n"
    "# Options\n"
    "\n"
#if 0
    "  -f, --fast\n"
    "  Use a much faster conversion. Time needed is greatly reduced, but this mode\n"
    "  is experimental and no engineeringdata file will be created.\n"
    "  This mode is disabled by default.\n"
    "\n"
#endif
    "  -e, --engineeringdata\n"
    "  Create an engineeringdata file. This turns off fast mode.\n"
    "  This is on by default.\n"
    "\n"
    "  -h, --help\n"
    "  Display this message and exit.\n"
    "\n",
    program);
  exit(0);
}

int main(int argc, char **argv)
{
  int fast = 0;
  mrb_state *mrb;
  mrb_value file, o, options;
  struct RClass *makeseed;

  program = argv[0];
  parse_options(&argc, &argv, OPTIONS(
    FLAG('f', "fast", fast, 1),
    FLAG('e', "engineeringdata", fast, 0),
    FLAG_CALLBACK('h', "help", help),
    FLAG_CALLBACK('?', "?", help)
  ));
  if (argc < 2) help(0);

  mrb = mrb_open();
  file = mrb_str_new_cstr(mrb, argv[1]);
  makeseed = mrb_class_get(mrb, "Makeseed");
  o = mrb_obj_new(mrb, makeseed, 0, 0);
  options = mrb_hash_new(mrb);
  mrb_hash_set(mrb, options, mrb_symbol_value(mrb_intern_lit(mrb, "fast")), mrb_bool_value(fast));
  mrb_funcall(mrb, o, "run", 2, file, options);
  mrb_close(mrb);
  return 0;
}
