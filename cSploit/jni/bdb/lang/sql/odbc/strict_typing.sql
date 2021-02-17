-- Example for SQLite + TinyCC taken from the book
-- "The Definitive Guide to SQLite" by Mike Owen, Chapter 7, p. 267-278

.echo on
-- Loading sqlite+tcc.dll ...
.load 'sqlite+tcc.dll'

-- Compiling code (SQL not shown) ...
.echo off
select tcc_compile('

#include <sqlite3.h>

/* Installs type validation triggers on column. It first looks the column''s
** declared type in the schema and attempts find the matching validation
** function (validate_xxx()). If on exists, it creates INSERT/UPDATE triggers
** to call validation function.
*/
void add_strict_type_check_udf(sqlite3_context* ctx, int nargs,
    sqlite3_value **values);

/* Helper function. Installs validation trigger on column */
int install_type_trigger(sqlite3 *db, sqlite3_context *ctx, 
    char *table, char *column);

/* Drops validation triggers on column */
void drop_strict_type_check_udf(sqlite3_context *ctx, int nargs,
    sqlite3_value **values);

/* Helper function. Drops validation trigger on column */
int uninstall_type_trigger(sqlite3 *db, sqlite3_context *ctx,
    char *table, char *column);

/* User-defined integer validation function. Use for integers, longs, etc. */
void validate_int_udf(sqlite3_context *ctx, int nargs, sqlite3_value **values);

/* User-defined float validation function. Use for floats, doubles, etc. */
void validate_double_udf(sqlite3_context *ctx, int nargs,
    sqlite3_value **values);

/* User-defined column type function. Given a table name and column name,
** returns a column''s declared type. 
*/
void column_type_udf(sqlite3_context *ctx, int nargs, sqlite3_value **values);

/* C Function: Lookup column''s declared type in sqlite_master. */
char* column_type(sqlite3 *db, char *table, char *column);

/* Initializer for this module */
void init(
  sqlite3 *db
){
  sqlite3_create_function(db, "add_strict_type_check", 2, SQLITE_UTF8, db,
      add_strict_type_check_udf, 0, 0);
  sqlite3_create_function(db, "drop_strict_type_check", 2, SQLITE_UTF8, db,
      drop_strict_type_check_udf, 0, 0);
  sqlite3_create_function(db, "column_type", 2, SQLITE_UTF8, db,
      column_type_udf, 0, 0);
  sqlite3_create_function(db, "validate_int", 1, SQLITE_UTF8, db,
      validate_int_udf, 0, 0);
  sqlite3_create_function(db, "validate_long", 1, SQLITE_UTF8, db,
      validate_int_udf, 0, 0);
  sqlite3_create_function(db, "validate_double", 1, SQLITE_UTF8, db,
      validate_double_udf, 0, 0);
  sqlite3_create_function(db, "validate_float", 1, SQLITE_UTF8, db,
      validate_double_udf, 0, 0);
}

void add_strict_type_check_udf(
  sqlite3_context *ctx,
  int nargs, 
  sqlite3_value **values
){
  sqlite3 *db;
  sqlite3_stmt *stmt;
  int rc;
  char *table, *column, *sql, *tmp;
  db = (sqlite3*) sqlite3_user_data(ctx);
  table = (char*) sqlite3_value_text(values[0]);
  column = (char*) sqlite3_value_text(values[1]);
  if( strncmp(column, "*", 1) == 0 ){
    /* Install on all columns */
    sql = "pragma table_info(%s)";
    tmp = sqlite3_mprintf(sql, table);
    rc = sqlite3_prepare(db, tmp, -1, &stmt, 0);
    sqlite3_free(tmp);
    if( rc != SQLITE_OK ){
      sqlite3_result_error(ctx, sqlite3_errmsg(db), -1);
      return;
    }
    rc = sqlite3_step(stmt);
    while( rc == SQLITE_ROW ){
      /* If not primary key */
      if( sqlite3_column_int(stmt, 5) != 1 ){
        column = (char*) sqlite3_column_text(stmt, 1); 
        install_type_trigger(db, ctx, table, column);
      }
      rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
  }else{
    /* Just installing on a single column */
    if( install_type_trigger(db, ctx, table, column) != 0 ){
      return;
    }
  }
  sqlite3_result_int(ctx, 0);
}

int install_type_trigger(
  sqlite3 *db,
  sqlite3_context *ctx, 
  char *table,
  char *column
){
  int rc;
  char buf[256];
  char *err, *sql, *type, *tmp;
  type = column_type(db, table, column);
  if( type == 0 ){
    sqlite3_result_error(ctx, "column has no declared type", -1);
    sqlite3_free(type);
    return 1;
  }
  /* Check to see if corresponding validation function exists */
  sql = "select validate_%s(null)";
  tmp = sqlite3_mprintf(sql, type);
  rc = sqlite3_exec(db, tmp, 0, 0, &err);
  sqlite3_free(tmp);
  if( rc != SQLITE_OK && err != 0 ){
    sqlite3_result_error(ctx, "no validator exists for column type", -1);
    sqlite3_free(type);
    sqlite3_free(err);
    return 1;
  }
  /* Create INSERT trigger */
  sql = "CREATE TRIGGER %s_insert_%s_typecheck_tr \n"
        "BEFORE INSERT ON %s \n"
        "BEGIN \n"
        "   SELECT CASE \n"
        "     WHEN(SELECT validate_%s(new.%s) != 1) \n"
        "     THEN RAISE(ABORT, ''invalid %s value for %s.%s'') \n"
        "   END; \n"
        "END;";
  tmp = sqlite3_mprintf(sql, table, column, table, type, 
            column, type, table, column);    
  rc = sqlite3_exec(db, tmp, 0, 0, &err);
  sqlite3_free(tmp);
  if( rc != SQLITE_OK && err != 0 ){
    strncpy(&buf[0], err, 255);
    buf[255] = ''\0'';
    sqlite3_result_error(ctx, &buf[0], -1);
    sqlite3_free(type);
    return 1;
  }
  /* Create UPDATE trigger */
  sql = "CREATE TRIGGER %s_update_%s_typecheck_tr \n"
        "BEFORE UPDATE OF %s ON %s \n"
        "FOR EACH ROW BEGIN \n"
        "  SELECT CASE \n"
        "    WHEN(SELECT validate_%s(new.%s) != 1) \n"
        "    THEN RAISE(ABORT, ''invalid %s value for %s.%s'') \n"
        "  END; \n"
        "END;";
  tmp = sqlite3_mprintf(sql, table, column, column, table, 
            type, column, type, table, column);
  rc = sqlite3_exec(db, tmp, 0, 0, &err);
  sqlite3_free(tmp);
  sqlite3_free(type);
  if( rc != SQLITE_OK && err != 0 ) {
    strncpy(&buf[0], err, 255);
    buf[255] = ''\0'';
    sqlite3_result_error(ctx, &buf[0], -1);
    sqlite3_free(err);
    return 1;
  }
  return 0;
}

void drop_strict_type_check_udf(
  sqlite3_context *ctx,
  int nargs,
  sqlite3_value **values
){
  sqlite3 *db;
  sqlite3_stmt *stmt;
  int rc;
  char *table, *column, *sql, *tmp;
  db = (sqlite3*) sqlite3_user_data(ctx);
  table = (char*) sqlite3_value_text(values[0]);
  column = (char*) sqlite3_value_text(values[1]);
  if( strncmp(column,"*",1) == 0 ){
    /* Install on all columns */
    sql = "pragma table_info(%s)";
    tmp = sqlite3_mprintf(sql, table);
    rc = sqlite3_prepare(db, tmp, -1, &stmt, 0);
    sqlite3_free(tmp);
    if( rc != SQLITE_OK ){
      sqlite3_result_error(ctx, sqlite3_errmsg(db), -1);
      return;
    }
    rc = sqlite3_step(stmt);
    while( rc == SQLITE_ROW ){
      /* If not primary key */
      if( sqlite3_column_int(stmt, 5) != 1 ){
        column = (char*) sqlite3_column_text(stmt, 1); 
        uninstall_type_trigger(db, ctx, table, column);
      }
      rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
  }else{
    /* Just installing on a single column */
    if( uninstall_type_trigger(db, ctx, table, column) != 0 ){
      return;
    }
  }
  sqlite3_result_int(ctx, 0);
}

/* Helper function. Drops validation trigger on column */
int uninstall_type_trigger(
  sqlite3 *db,
  sqlite3_context *ctx,
  char *table,
  char *column
){
  int rc;
  char buf[256];
  char *tmp, *err, *sql;
  /* Drop INSERT trigger */
  sql = "DROP TRIGGER %s_insert_%s_typecheck_tr";
  tmp = sqlite3_mprintf(sql, table, column);    
  rc = sqlite3_exec(db, tmp, 0, 0, &err);
  sqlite3_free(tmp);
  if( rc != SQLITE_OK && err != 0 ){
    strncpy(&buf[0], err, 255);
    buf[255] = ''\0'';
    sqlite3_result_error(ctx, &buf[0], -1);
    return 1;
  }
  /* Drop UPDATE trigger */
  sql = "DROP TRIGGER %s_update_%s_typecheck_tr";
  tmp = sqlite3_mprintf(sql, table, column);    
  rc = sqlite3_exec(db, tmp, 0, 0, &err);
  sqlite3_free(tmp);
  if( rc != SQLITE_OK && err != 0 ){
    strncpy(&buf[0], err, 255);
    buf[255] = ''\0'';
    sqlite3_result_error(ctx, &buf[0], -1);
    return 1;
  }
  return 0;
}

void validate_int_udf(
  sqlite3_context *ctx,
  int nargs,
  sqlite3_value **values
){
  sqlite3 *db;
  char *value;
  char *tmp;
  db = (sqlite3*) sqlite3_user_data(ctx);
  value = (char*) sqlite3_value_text(values[0]);
  /* Assuming NULL values for type checked columns not allowed */
  if( value == 0 ){
    sqlite3_result_int(ctx, 0);
    return;
  }
  /* Validate type: */
  tmp = 0;
  strtol(value, &tmp, 0);
  if( *tmp != ''\0'' ){
    /* Value does not conform to type */
    sqlite3_result_int(ctx, 0);
    return;
  }
  /* If we got this far value is valid. */
  sqlite3_result_int(ctx, 1);
}

void validate_double_udf(
  sqlite3_context* ctx,
  int nargs,
  sqlite3_value** values
){
  sqlite3 *db;
  char *value;
  char *tmp;
  db = (sqlite3*) sqlite3_user_data(ctx);
  value = (char*) sqlite3_value_text(values[0]);
  /* Assuming NULL values for type checked columns not allowed */
  if( value == 0 ){
    sqlite3_result_int(ctx, 0);
    return;
  }
  /* Validate type: */
  tmp = 0;
  strtod(value, &tmp);
  if( *tmp != ''\0'' ){
    /* Value does not conform to type */
    sqlite3_result_int(ctx, 0);
    return;
  }
  /* If we got this far value is valid. */
  sqlite3_result_int(ctx, 1);
}

void column_type_udf(
  sqlite3_context *ctx,
  int nargs,
  sqlite3_value **values
){
  sqlite3 *db;
  char *table, *column, *type;
  db = (sqlite3*) sqlite3_user_data(ctx);
  table = (char*) sqlite3_value_text(values[0]);
  column = (char*) sqlite3_value_text(values[1]);
  /* Get declared type from schema */
  type = column_type(db, table, column);
  /* Return type */
  sqlite3_result_text(ctx, type, -1, SQLITE_TRANSIENT);
}

char *column_type(
  sqlite3* db,
  char *table,
  char *column
){
  sqlite3_stmt *stmt;
  int i, len, rc;
  char *sql, *tmp, *type, *p, *sql_type;
  sql = "select %s from %s;";
  tmp = sqlite3_mprintf(sql, column, table);
  rc = sqlite3_prepare(db, tmp, -1, &stmt, 0);
  if( rc != SQLITE_OK ){
    sqlite3_free(tmp);
    return 0;
  }
  sql_type = (char*) sqlite3_column_decltype(stmt, 0);
  /* Convert type to lower case */
  i = 0;
  p = sql_type;
  len = strlen(sql_type);
  type = sqlite3_malloc(len + 1);
  while( i < len ) {
    type[i] = tolower(*p);
    p++;i++;
  }
  type[len] = ''\0'';
  /* Free statement handle and tmp sql string */
  sqlite3_finalize(stmt);
  sqlite3_free(tmp);
  return type;
}

');

.echo on
-- Creating table types.
create table types(
  id integer primary key,
  x int not null default 0,
  y float not null default 0.0
);

-- Populating table types.
insert into types(x,y) values(1,1.1);
insert into types(x,y) values(2,2.1);
insert into types(x,y) values(3,3.1);

-- 1. Add strict typing:
select add_strict_type_check('types', '*');

-- 2. Insert integer value -- should succeed:
insert into types (x) values (1);

-- 3. Update with invalid values -- should fail:
update types set x = 'abc';
update types set y = 'abc';

-- 4. Remove strict typing
select drop_strict_type_check('types', '*');

-- 5. Update with non-integer value -- should succeed:
update types set x = 'not an int';

-- 6. Select records:
.header on
select * from types;

-- 7. Test column_type() UDF
select column_type('types', 'id') as 'id',
       column_type('types', 'x')  as 'x',
       column_type('types', 'y')  as 'y';
