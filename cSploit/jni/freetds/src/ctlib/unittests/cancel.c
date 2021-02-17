#include <config.h>

#include <stdio.h>

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <sys/types.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <signal.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: cancel.c,v 1.15 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#if defined(HAVE_ALARM) && defined(HAVE_SETITIMER)

/* protos */
int do_fetch(CS_COMMAND * cmd, int *cnt);
void catch_alrm(int);

/* Globals */
static volatile CS_COMMAND *g_cmd = NULL;

void
catch_alrm(int sig_num)
{
	signal(SIGALRM, catch_alrm);

	fprintf(stdout, "- SIGALRM\n");

	/* Cancel current command */
	if (g_cmd)
		ct_cancel(NULL, (CS_COMMAND *) g_cmd, CS_CANCEL_ATTN);

	fflush(stdout);
}

/* Testing: Test asynchronous ct_cancel() */
int
main(int argc, char **argv)
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	int i, verbose = 0, cnt = 0;

	CS_RETCODE ret;
	CS_INT result_type;

	struct itimerval timer;
	char query[1024];

	unsigned clock = 200000;

	fprintf(stdout, "%s: Check asynchronous called ct_cancel()\n", __FILE__);
	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	/* Create needed tables */
	ret = run_command(cmd, "CREATE TABLE #t0010 (id int, col1 varchar(255))");
	if (ret != CS_SUCCEED)
		return 1;

	for (i = 0; i < 10; i++) {
		sprintf(query, "INSERT #t0010 (id, col1) values (%d, 'This is field no %d')", i, i);

		ret = run_command(cmd, query);
		if (ret != CS_SUCCEED)
			return 1;
	}

	/* Set SIGALRM signal handler */
	signal(SIGALRM, catch_alrm);

	for (;;) {
		/* TODO better to use alarm AFTER ct_send ?? */
		/* Set timer */
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = clock;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = clock;
		if (0 != setitimer(ITIMER_REAL, &timer, NULL)) {
			fprintf(stderr, "Could not set realtime timer.\n");
			return 1;
		}

		/* Issue a command returning many rows */
		ret = ct_command(cmd, CS_LANG_CMD, "SELECT * FROM #t0010 t1, #t0010 t2, #t0010 t3, #t0010 t4", CS_NULLTERM, CS_UNUSED);
		if (ret != CS_SUCCEED) {
			fprintf(stderr, "ct_command() failed.\n");
			return 1;
		}

		ret = ct_send(cmd);
		if (ret != CS_SUCCEED) {
			fprintf(stderr, "first ct_send() failed.\n");
			return 1;
		}

		/* Save a global reference for the interrupt handler */
		g_cmd = cmd;

		while ((ret = ct_results(cmd, &result_type)) == CS_SUCCEED) {
			printf("More results?...\n");
			if (result_type == CS_STATUS_RESULT)
				continue;

			switch ((int) result_type) {
			case CS_ROW_RESULT:
				printf("do_fetch() returned: %d\n", do_fetch(cmd, &cnt));
				break;
			}
		}

		/* We should not have received all rows, as the alarm signal cancelled it... */
		if (cnt < 10000)
			break;

		if (clock <= 5000) {
			fprintf(stderr, "All rows read, this may not occur.\n");
			return 1;
		}
		g_cmd = NULL;
		clock /= 2;
	}

	/* Remove timer */
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	if (0 != setitimer(ITIMER_REAL, &timer, NULL)) {
		fprintf(stderr, "Could not remove realtime timer.\n");
		return 1;
	}

	/*
	 * Issue another command, this will be executed after a ct_cancel, 
	 * to test if wire state is consistent 
	 */
	ret = ct_command(cmd, CS_LANG_CMD, "SELECT * FROM #t0010 t1, #t0010 t2, #t0010 t3", CS_NULLTERM, CS_UNUSED);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_command() failed.\n");
		return 1;
	}

	ret = ct_send(cmd);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "second ct_send() failed.\n");
		return 1;
	}

	while ((ret = ct_results(cmd, &result_type)) == CS_SUCCEED) {
		printf("More results?...\n");
		if (result_type == CS_STATUS_RESULT)
			continue;

		switch ((int) result_type) {
		case CS_ROW_RESULT:
			printf("do_fetch() returned: %d\n", do_fetch(cmd, &cnt));
			break;
		}
	}

	if (1000 != cnt) {
		/* This time, all rows must have been received */
		fprintf(stderr, "Incorrect number of rows read.\n");
		return 1;
	}

	if (verbose) {
		fprintf(stdout, "Trying logout\n");
	}
	ret = try_ctlogout(ctx, conn, cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Logout failed\n");
		return 1;
	}

	printf("%s: asynchronous cancel test: PASSED\n", __FILE__);

	return 0;
}

int
do_fetch(CS_COMMAND * cmd, int *cnt)
{
	CS_INT count, row_count = 0;
	CS_RETCODE ret;

	while ((ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count)) == CS_SUCCEED) {
		/* printf ("ct_fetch() == CS_SUCCEED\n"); */
		row_count += count;
	}

	(*cnt) = row_count;
	if (ret == CS_ROW_FAIL) {
		fprintf(stderr, "ct_fetch() CS_ROW_FAIL on row %d.\n", row_count);
		return 1;
	} else if (ret == CS_END_DATA) {
		printf("do_fetch retrieved %d rows\n", row_count);
		return 0;
	} else if (ret == CS_CMD_FAIL) {
		printf("do_fetch(): command aborted after receiving %d rows\n", row_count);
		return 0;
	} else {
		fprintf(stderr, "ct_fetch() unexpected return %d on row %d.\n", ret, row_count);
		return 1;
	}
}

#else

int
main(void)
{
	return 0;
}
#endif

