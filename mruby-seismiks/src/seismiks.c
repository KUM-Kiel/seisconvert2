#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/value.h>
#include <sample_buffer.h>
#include "lz4frame.h"
#include "sqlite3.h"

static void seismiks_destroy(mrb_state *mrb, void *p)
{
  sqlite3 *file = p;
  if (file) {
    sqlite3_close_v2(file);
  }
}

static const mrb_data_type seismiks_type[1] = {{"Seismiks", seismiks_destroy}};

sqlite3 *seismiks_check(mrb_state *mrb, mrb_value v)
{
  sqlite3 *file = mrb_data_check_get_ptr(mrb, v, seismiks_type);
  if (file) {
    return file;
  } else {
    mrb_raise(mrb, E_RUNTIME_ERROR, "Seismiks file already closed");
  }
}

#include "seismiks_format_1.h"

static mrb_value seismiks_initialize(mrb_state *mrb, mrb_value self)
{
  char *filename = 0;
  sqlite3 *file;
  sqlite3_stmt *stmt;
  sqlite3_int64 i;
  int r;

  mrb_get_args(mrb, "z", &filename);

  r = sqlite3_open_v2(filename, &file, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI, 0);
  if (r != SQLITE_OK) {
    sqlite3_close(file);
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not open Seismiks file");
  }

  /* Initialize the file. */
  r = sqlite3_prepare_v2(file, "PRAGMA application_id;", -1, &stmt, 0);
  if (r != SQLITE_OK) mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid file");
  r = sqlite3_step(stmt);
  if (r != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid file");
  }
  i = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  if (i == 0) {
    /* Create tables. */
    r = sqlite3_exec(file, seismiks_format_1_sql, 0, 0, 0);
    if (r != SQLITE_OK) {
      sqlite3_exec(file, "ROLLBACK;", 0, 0, 0);
      mrb_raise(mrb, E_RUNTIME_ERROR, "Could not create file");
    }
  } else if (i == 1936542579) {
    r = sqlite3_prepare_v2(file, "PRAGMA user_version;", -1, &stmt, 0);
    if (r != SQLITE_OK) mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid file");
    r = sqlite3_step(stmt);
    if (r != SQLITE_ROW) {
      sqlite3_finalize(stmt);
      mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid file");
    }
    i = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    if (i != 1) mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid file");
  } else {
    mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid file");
  }

  mrb_data_init(self, file, seismiks_type);

  return self;
}

static mrb_value seismiks_close(mrb_state *mrb, mrb_value self)
{
  sqlite3 *file = seismiks_check(mrb, self);
  sqlite3_close_v2(file);
  mrb_data_init(self, 0, seismiks_type);
  return self;
}

static mrb_value seismiks_create_station(mrb_state *mrb, mrb_value self)
{
  char *name, *comment = 0;
  mrb_int namelen, commentlen = 0;
  sqlite3 *file = seismiks_check(mrb, self);
  sqlite3_stmt *stmt;
  int r;
  mrb_int id;

  mrb_get_args(mrb, "s|s", &name, &namelen, &comment, &commentlen);

  r = sqlite3_prepare_v2(file, "INSERT INTO stations (name, comment) VALUES (?, ?);", -1, &stmt, 0);
  if (r != SQLITE_OK) mrb_raise(mrb, E_RUNTIME_ERROR, "Could not create station");

  sqlite3_bind_text(stmt, 1, name, namelen, SQLITE_TRANSIENT);
  if (comment) {
    sqlite3_bind_text(stmt, 2, comment, commentlen, SQLITE_TRANSIENT);
  }

  r = sqlite3_step(stmt);
  if (r != SQLITE_DONE) mrb_raise(mrb, E_RUNTIME_ERROR, "Could not create station");

  id = sqlite3_last_insert_rowid(file);

  sqlite3_finalize(stmt);

  return mrb_fixnum_value(id);
}

static mrb_value seismiks_get_station(mrb_state *mrb, mrb_value self)
{
  sqlite3 *file = seismiks_check(mrb, self);
  sqlite3_stmt *stmt;
  int r;
  mrb_int id;
  mrb_value station;

  mrb_get_args(mrb, "i", &id);

  r = sqlite3_prepare_v2(file, "SELECT name, comment FROM stations WHERE id = ?;", -1, &stmt, 0);
  if (r != SQLITE_OK) mrb_raise(mrb, E_RUNTIME_ERROR, "");

  sqlite3_bind_int64(stmt, 1, id);

  r = sqlite3_step(stmt);
  if (r == SQLITE_ROW) {
    station = mrb_ary_new(mrb);
    mrb_ary_push(mrb, station, mrb_str_new(mrb, (char *) sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0)));
    if (sqlite3_column_type(stmt, 1) == SQLITE_NULL) {
      mrb_ary_push(mrb, station, mrb_nil_value());
    } else {
      mrb_ary_push(mrb, station, mrb_str_new(mrb, (char *) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1)));
    }
    sqlite3_finalize(stmt);
    return station;
  } else {
    sqlite3_finalize(stmt);
    return mrb_nil_value();
  }
}

void mrb_mruby_seismiks_gem_init(mrb_state *mrb)
{
  struct RClass *seismiks;
  seismiks = mrb_define_class(mrb, "Seismiks", mrb->object_class);
  MRB_SET_INSTANCE_TT(seismiks, MRB_TT_DATA);
  mrb_define_method(mrb, seismiks, "initialize", seismiks_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, seismiks, "close", seismiks_close, MRB_ARGS_NONE());
  mrb_define_method(mrb, seismiks, "create_station", seismiks_create_station, MRB_ARGS_ARG(1, 1));
  mrb_define_method(mrb, seismiks, "get_station", seismiks_get_station, MRB_ARGS_ARG(1, 1));
}

void mrb_mruby_seismiks_gem_final(mrb_state *mrb)
{

}
