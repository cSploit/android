/*
 * Nsock regression test suite
 * Same license as nmap -- see http://nmap.org/book/man-legal.html
 */

#include "test-common.h"


struct log_test_data {
  nsock_pool nsp;
  nsock_loglevel_t current_level;
  unsigned int got_dbgfull: 1;
  unsigned int got_dbg: 1;
  unsigned int got_info: 1;
  unsigned int got_error: 1;
  unsigned int total;
  int errcode;
};


static void log_handler(nsock_pool nsp, const struct nsock_log_rec *rec) {
  struct log_test_data *ltd;

  ltd = (struct log_test_data *)nsp_getud(nsp);
  ltd->total++;

  switch(rec->level) {
    case NSOCK_LOG_DBG_ALL:
      ltd->got_dbgfull = 1;
      break;

    case NSOCK_LOG_DBG:
      ltd->got_dbg = 1;
      break;

    case NSOCK_LOG_INFO:
      ltd->got_info = 1;
      break;

    case NSOCK_LOG_ERROR:
      ltd->got_error = 1;
      break;

    default:
      fprintf(stderr, "UNEXPECTED LOG LEVEL (%d)!\n", (int)rec->level);
      ltd->errcode = -EINVAL;
  }
}

static void nop_handler(nsock_pool nsp, nsock_event nse, void *udata) {
  return;
}

static int check_loglevel(struct log_test_data *ltd, nsock_loglevel_t level) {
  int rc = 0;
  nsock_event_id id;

  nsock_set_loglevel(ltd->nsp, level);

  ltd->current_level = level;

  ltd->got_dbgfull = 0;
  ltd->got_dbg     = 0;
  ltd->got_info    = 0;
  ltd->got_error   = 0;

  ltd->total   = 0;
  ltd->errcode = 0;

  id = nsock_timer_create(ltd->nsp, nop_handler, 200, NULL);
  nsock_event_cancel(ltd->nsp, id, 0);

  if (ltd->errcode)
    return ltd->errcode;

  if (ltd->total < 1)
    return -EINVAL;

  return rc;
}

static int check_errlevel(struct log_test_data *ltd, nsock_loglevel_t level) {
  nsock_event_id id;

  nsock_set_loglevel(ltd->nsp, level);

  ltd->current_level = level;

  ltd->got_dbgfull = 0;
  ltd->got_dbg     = 0;
  ltd->got_info    = 0;
  ltd->got_error   = 0;

  ltd->total   = 0;
  ltd->errcode = 0;

  id = nsock_timer_create(ltd->nsp, nop_handler, 200, NULL);
  nsock_event_cancel(ltd->nsp, id, 0);

  if (ltd->errcode)
    return ltd->errcode;

  if (ltd->total > 0)
    return -EINVAL;

  return 0;
}

static int log_setup(void **tdata) {
  struct log_test_data *ltd;

  ltd = calloc(1, sizeof(struct log_test_data));
  if (ltd == NULL)
    return -ENOMEM;

  ltd->nsp = nsp_new(ltd);
  AssertNonNull(ltd->nsp);

  *tdata = ltd;
  return 0;
}

static int log_teardown(void *tdata) {
  struct log_test_data *ltd = (struct log_test_data *)tdata;

  if (tdata) {
    nsp_delete(ltd->nsp);
    free(tdata);
  }
  return 0;
}

static int log_check_std_levels(void *tdata) {
  struct log_test_data *ltd = (struct log_test_data *)tdata;
  nsock_loglevel_t lvl;
  int rc = 0;

  nsock_set_log_function(ltd->nsp, log_handler);

  for (lvl = NSOCK_LOG_DBG_ALL; lvl < NSOCK_LOG_ERROR; lvl++) {
    rc = check_loglevel(ltd, lvl);
    if (rc)
      return rc;
  }

  return 0;
}

static int log_check_err_levels(void *tdata) {
  struct log_test_data *ltd = (struct log_test_data *)tdata;
  nsock_loglevel_t lvl;
  int rc = 0;

  nsock_set_log_function(ltd->nsp, log_handler);

  for (lvl = NSOCK_LOG_ERROR; lvl <= NSOCK_LOG_NONE; lvl++) {
    rc = check_errlevel(ltd, NSOCK_LOG_ERROR);
    if (rc)
      return rc;
  }

  return 0;
}


const struct test_case TestLogLevels = {
  .t_name     = "set standard log levels",
  .t_setup    = log_setup,
  .t_run      = log_check_std_levels,
  .t_teardown = log_teardown
};

const struct test_case TestErrLevels = {
  .t_name     = "check error log levels",
  .t_setup    = log_setup,
  .t_run      = log_check_err_levels,
  .t_teardown = log_teardown
};
