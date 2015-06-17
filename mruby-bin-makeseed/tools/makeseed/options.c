/* liboptions by Lukas Joeressen.
 *
 * The author disclaims copyright to this source code.  In place of
 * a legal notice, here is a blessing:
 *
 * - May you do good and not evil.
 * - May you find forgiveness for yourself and forgive others.
 * - May you share freely, never taking more than you give. */

#include "options.h"

#include <stdio.h>
#include <stdlib.h>

#define NEED_ARGUMENT_LONG  "Option '--\0' needs an argument."
#define NEED_ARGUMENT_SHORT "Option '-\0' needs an argument."
#define UNRECOGNISED_LONG   "Unrecognised option '--\0'."
#define UNRECOGNISED_SHORT  "Unrecognised option '-\0'."
#define UNEXPECTED_ARGUMENT "Option '--\0' expects no argument."

static int fprintfs(FILE* f, const char *format, const char *str)
{
  while (*format) putc(*(format++), f); format++;
  while (*str)    putc(*(str++), f);
  while (*format) putc(*(format++), f);
  return 0;
}

static int fprintfc(FILE* f, const char *format, char c)
{
  while (*format) putc(*(format++), f); format++;
  putc(c, f);
  while (*format) putc(*(format++), f);
  return 0;
}


static int str_equal(const char *a, const char *b)
{
  if (!a || !b) return 0;
  while (*a && *b && *a == *b) {++a; ++b;}
  return *a == *b;
}

static option_t *lookup_short(option_t **o, char n)
{
  while (*o) {
    if ((*o)->shortopt == n) {
      return *o;
    }
    ++o;
  }
  fprintfc(stderr, UNRECOGNISED_SHORT "\n", n);
  exit(1);
  return 0;
}

static option_t *lookup_long(option_t **o, char *n)
{
  while (*o) {
    if ((*o)->longopt && str_equal((*o)->longopt, n)) {
      return *o;
    }
    ++o;
  }
  fprintfs(stderr, UNRECOGNISED_LONG "\n", n);
  exit(1);
  return 0;
}

static char *find_and_null_eq(char *s)
{
  while (*s) {
    if (*s == '=') {
      *s = 0;
      return s + 1;
    }
    ++s;
  }
  return 0;
}

#define HAS_PARAMETER(o) ((o)->type & OPTION_PARAMETER)
#define HAS_DEFAULT(o) ((o)->type & OPTION_DEFAULT)
#define HAS_CALLBACK(o) ((o)->type & OPTION_CALLBACK)

static void apply_option(option_t *o, char *arg, int l)
{
  if (!arg && HAS_DEFAULT(o)) {
    arg = o->arg_default;
  }
  if (!arg && HAS_PARAMETER(o) && !HAS_DEFAULT(o)) {
    if (l) {
      fprintfs(stderr, NEED_ARGUMENT_LONG "\n", o->longopt);
    } else {
      fprintfc(stderr, NEED_ARGUMENT_SHORT "\n", o->shortopt);
    }
    exit(1);
  }
  if (HAS_CALLBACK(o)) {
    o->callback(arg);
  } else if (HAS_PARAMETER(o)) {
    if (o->arg_adr) {
      *(o->arg_adr) = arg;
    }
  } else {
    if (o->flag_adr) {
      *(o->flag_adr) = o->flag_value;
    }
  }
}

int parse_options(int *argc, char ***argv, option_t **options)
{
  int c = 1, i = 1, j;
  char x;
  char *a, *v;
  char **_argv = *argv;
  option_t *o = 0;
  int l = 0;

  while (i < *argc) {
    if (_argv[i][0] == '-' && _argv[i][1]) {
      if (o) {
        apply_option(o, 0, l);
        o = 0;
      }
      if (_argv[i][1] == '-' && !_argv[i][2]) {
        /* End of options */
        ++i;
        while (i < *argc) {
          _argv[c] = _argv[i];
          ++c;
          ++i;
        }
      } else if (_argv[i][1] == '-') {
        /* Longopt */
        l = 1;
        a = _argv[i] + 2;
        v = find_and_null_eq(a);
        o = lookup_long(options, a);
        if (v || !HAS_PARAMETER(o)) {
          if (v && !HAS_PARAMETER(o)) {
            fprintfs(stderr, UNEXPECTED_ARGUMENT "\n", o->longopt);
            exit(1);
          }
          apply_option(o, v, l);
          o = 0;
        }
      } else {
        /* Shortopt */
        l = 0;
        j = 1;
        while ((x = _argv[i][j])) {
          o = lookup_short(options, x);
          if (!HAS_PARAMETER(o)) {
            apply_option(o, 0, l);
            o = 0;
          } else {
            if (_argv[i][j + 1]) {
              apply_option(o, _argv[i] + j + 1, l);
              o = 0;
            }
            break;
          }
          ++j;
        }
      }
    } else {
      if (o) {
        apply_option(o, _argv[i], l);
        o = 0;
      } else {
        _argv[c] = _argv[i];
        ++c;
      }
    }
    ++i;
  }
  if (o) apply_option(o, 0, l);

  /* Clean argument vector. */
  for (i = c; i < *argc; ++i)
    _argv[i] = 0;
  *argc = c;

  return 0;
}
