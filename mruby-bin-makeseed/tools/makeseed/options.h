/* liboptions by Lukas Joeressen.
 *
 * The author disclaims copyright to this source code.  In place of
 * a legal notice, here is a blessing:
 *
 * - May you do good and not evil.
 * - May you find forgiveness for yourself and forgive others.
 * - May you share freely, never taking more than you give. */

#pragma once

typedef struct {
  int type;
  char shortopt;
  const char *longopt;
  int *flag_adr;
  int flag_value;
  char **arg_adr;
  char *arg_default;
  void (*callback)(const char *arg);
} option_t;

#define OPTION_FLAG 0
#define OPTION_PARAMETER 1
#define OPTION_DEFAULT 2
#define OPTION_CALLBACK 4

/* Create a list of options. This list can be passed directly to parse_options. */
#define OPTIONS(...) ((option_t *[]){__VA_ARGS__, 0})

/* Create a Flag. A Flag has no arguments but sets the variable f to the
 * value v, if it is encountered on the command line. f must be of type int.
 * s is the short option name as char and l is the long option name as c string.
 * If short or long option name are not wanted, they can be set to zero. */
#define FLAG(s, l, f, v) (&((option_t){OPTION_FLAG, s, l, &(f), v, 0, 0, 0}))

/* A Flag with a callback. Instead of setting a variable, the callback function c
 * is called, if the flag is encountered on the command line.
 * c must be of type void c(char *). It is always called with a null pointer. */
#define FLAG_CALLBACK(s, l, c) (&((option_t){OPTION_FLAG | OPTION_CALLBACK, s, l, 0, 0, 0, 0, c}))

/* A parameter. Parameters expect a value. If the parameter is encountered on the
 * command line, the value is stored into p. p must be of type char*. */
#define PARAMETER(s, l, p) (&((option_t){OPTION_PARAMETER, s, l, 0, 0, &(p), 0, 0}))

/* A parameter with a callback. Instead of storing the value into a variable,
 * the callback function is called with the encountered value. */
#define PARAMETER_CALLBACK(s, l, c) (&((option_t){OPTION_PARAMETER | OPTION_CALLBACK, s, l, 0, 0, 0, 0, c}))

/* Like PARAMETER, but if the value is ommitted on the command line, the variable
 * will be set to v. */
#define PARAMETER_DEFAULT(s, l, p, v) (&((option_t){OPTION_PARAMETER | OPTION_DEFAULT, s, l, 0, 0, &(p), v, 0}))

/* Like PARAMETER_CALLBACK, but if the value is ommitted on the command line, the
 * callback function is called with argument v. */
#define PARAMETER_DEFAULT_CALLBACK(s, l, c, v) (&((option_t){OPTION_PARAMETER | OPTION_DEFAULT | OPTION_CALLBACK, s, l, 0, 0, 0, v, c}))

/* Do the option parsing. Expects pointers to argc and argv, which will be
 * modified to hold only the positional arguments after the parsing process.
 * The third parameter is a list created with OPTIONS(...). */
extern int parse_options(int *argc, char ***argv, option_t **options);
