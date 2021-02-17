#include "common.h"

static char software_version[] = "$Id: common.c,v 1.33 2012-03-11 15:52:22 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

char USER[512];
char SERVER[512];
char PASSWORD[512];
char DATABASE[512];
/* TODO use another default ?? */
char CHARSET[512] = "ISO-8859-1";

int read_login_info(void);

int
read_login_info(void)
{
	FILE *in = NULL;
	char line[512];
	char *s1, *s2;

	s1 = getenv("TDSPWDFILE");
	if (s1 && s1[0])
		in = fopen(s1, "r");
	if (!in)
		in = fopen("../../../PWD", "r");
	if (!in) {
		fprintf(stderr, "Can not open PWD file\n\n");
		return TDS_FAIL;
	}

	while (fgets(line, 512, in)) {
		s1 = strtok(line, "=");
		s2 = strtok(NULL, "\n");
		if (!s1 || !s2) {
			continue;
		}
		if (!strcmp(s1, "UID")) {
			strcpy(USER, s2);
		} else if (!strcmp(s1, "SRV")) {
			strcpy(SERVER, s2);
		} else if (!strcmp(s1, "PWD")) {
			strcpy(PASSWORD, s2);
		} else if (!strcmp(s1, "DB")) {
			strcpy(DATABASE, s2);
		}
	}
	fclose(in);
	return TDS_SUCCESS;
}

TDSCONTEXT *test_context = NULL;

int
try_tds_login(TDSLOGIN ** login, TDSSOCKET ** tds, const char *appname, int verbose)
{
	TDSLOGIN *connection;

	if (verbose) {
		fprintf(stdout, "Entered tds_try_login()\n");
	}
	if (!login) {
		fprintf(stderr, "Invalid TDSLOGIN**\n");
		return TDS_FAIL;
	}
	if (!tds) {
		fprintf(stderr, "Invalid TDSSOCKET**\n");
		return TDS_FAIL;
	}

	if (verbose) {
		fprintf(stdout, "Trying read_login_info()\n");
	}
	read_login_info();

	if (verbose) {
		fprintf(stdout, "Setting login parameters\n");
	}
	*login = tds_alloc_login(1);
	if (!*login) {
		fprintf(stderr, "tds_alloc_login() failed.\n");
		return TDS_FAIL;
	}
	tds_set_passwd(*login, PASSWORD);
	tds_set_user(*login, USER);
	tds_set_app(*login, appname);
	tds_set_host(*login, "myhost");
	tds_set_library(*login, "TDS-Library");
	tds_set_server(*login, SERVER);
	tds_set_client_charset(*login, CHARSET);
	tds_set_language(*login, "us_english");

	if (verbose) {
		fprintf(stdout, "Connecting to database\n");
	}
	test_context = tds_alloc_context(NULL);
	*tds = tds_alloc_socket(test_context, 512);
	tds_set_parent(*tds, NULL);
	connection = tds_read_config_info(*tds, *login, test_context->locale);
	if (!connection || tds_connect_and_login(*tds, connection) != TDS_SUCCESS) {
		if (connection) {
			tds_free_socket(*tds);
			*tds = NULL;
			tds_free_login(connection);
		}
		fprintf(stderr, "tds_connect_and_login() failed\n");
		return TDS_FAIL;
	}
	tds_free_login(connection);

	return TDS_SUCCESS;
}


/* Note that this always suceeds */
int
try_tds_logout(TDSLOGIN * login, TDSSOCKET * tds, int verbose)
{
	if (verbose) {
		fprintf(stdout, "Entered tds_try_logout()\n");
	}
	tds_free_socket(tds);
	tds_free_login(login);
	tds_free_context(test_context);
	test_context = NULL;
	return TDS_SUCCESS;
}

/* Run query for which there should be no return results */
int
run_query(TDSSOCKET * tds, const char *query)
{
	int rc;
	int result_type;

	rc = tds_submit_query(tds, query);
	if (rc != TDS_SUCCESS) {
		fprintf(stderr, "tds_submit_query() failed for query '%s'\n", query);
		return TDS_FAIL;
	}

	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_RESULTS)) == TDS_SUCCESS) {

		switch (result_type) {
		case TDS_DONE_RESULT:
		case TDS_DONEPROC_RESULT:
		case TDS_DONEINPROC_RESULT:
			/* ignore possible spurious result (TDS7+ send it) */
		case TDS_STATUS_RESULT:
			break;
		default:
			fprintf(stderr, "Error:  query should not return results\n");
			return TDS_FAIL;
		}
	}
	if (rc == TDS_FAIL) {
		fprintf(stderr, "tds_process_tokens() returned TDS_FAIL for '%s'\n", query);
		return TDS_FAIL;
	} else if (rc != TDS_NO_MORE_RESULTS) {
		fprintf(stderr, "tds_process_tokens() unexpected return\n");
		return TDS_FAIL;
	}

	return TDS_SUCCESS;
}
