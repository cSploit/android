/*
 *	pg_test_fsync.c
 *		tests all supported fsync() methods
 */

#include "postgres_fe.h"

#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "getopt_long.h"
#include "access/xlogdefs.h"


/*
 * put the temp files in the local directory
 * unless the user specifies otherwise
 */
#define FSYNC_FILENAME	"./pg_test_fsync.out"

#define XLOG_BLCKSZ_K	(XLOG_BLCKSZ / 1024)

#define LABEL_FORMAT		"        %-32s"
#define NA_FORMAT			"%18s"
#define OPS_FORMAT			"%9.3f ops/sec"

static const char *progname;

static int	ops_per_test = 2000;
static char full_buf[XLOG_SEG_SIZE],
		   *buf,
		   *filename = FSYNC_FILENAME;
static struct timeval start_t,
			stop_t;


static void handle_args(int argc, char *argv[]);
static void prepare_buf(void);
static void test_open(void);
static void test_non_sync(void);
static void test_sync(int writes_per_op);
static void test_open_syncs(void);
static void test_open_sync(const char *msg, int writes_size);
static void test_file_descriptor_sync(void);

#ifdef HAVE_FSYNC_WRITETHROUGH
static int	pg_fsync_writethrough(int fd);
#endif
static void print_elapse(struct timeval start_t, struct timeval stop_t);
static void die(const char *str);


int
main(int argc, char *argv[])
{
	progname = get_progname(argv[0]);

	handle_args(argc, argv);

	prepare_buf();

	test_open();

	/* Test using 1 XLOG_BLCKSZ write */
	test_sync(1);

	/* Test using 2 XLOG_BLCKSZ writes */
	test_sync(2);

	test_open_syncs();

	test_file_descriptor_sync();

	test_non_sync();

	unlink(filename);

	return 0;
}

static void
handle_args(int argc, char *argv[])
{
	static struct option long_options[] = {
		{"filename", required_argument, NULL, 'f'},
		{"ops-per-test", required_argument, NULL, 'o'},
		{NULL, 0, NULL, 0}
	};
	int			option;			/* Command line option */
	int			optindex = 0;	/* used by getopt_long */

	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ||
			strcmp(argv[1], "-?") == 0)
		{
			printf("Usage: %s [-f FILENAME] [-o OPS-PER-TEST]\n", progname);
			exit(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			puts("pg_test_fsync (PostgreSQL) " PG_VERSION);
			exit(0);
		}
	}

	while ((option = getopt_long(argc, argv, "f:o:",
								 long_options, &optindex)) != -1)
	{
		switch (option)
		{
			case 'f':
				filename = strdup(optarg);
				break;

			case 'o':
				ops_per_test = atoi(optarg);
				break;

			default:
				fprintf(stderr, "Try \"%s --help\" for more information.\n",
						progname);
				exit(1);
				break;
		}
	}

	if (argc > optind)
	{
		fprintf(stderr,
				"%s: too many command-line arguments (first is \"%s\")\n",
				progname, argv[optind]);
		fprintf(stderr, "Try \"%s --help\" for more information.\n",
				progname);
		exit(1);
	}

	printf("%d operations per test\n", ops_per_test);
#if PG_O_DIRECT != 0
	printf("O_DIRECT supported on this platform for open_datasync and open_sync.\n");
#else
	printf("Direct I/O is not supported on this platform.\n");
#endif
}

static void
prepare_buf(void)
{
	int			ops;

	/* write random data into buffer */
	for (ops = 0; ops < XLOG_SEG_SIZE; ops++)
		full_buf[ops] = random();

	buf = (char *) TYPEALIGN(ALIGNOF_XLOG_BUFFER, full_buf);
}

static void
test_open(void)
{
	int			tmpfile;

	/*
	 * test if we can open the target file
	 */
	if ((tmpfile = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
		die("could not open output file");
	if (write(tmpfile, full_buf, XLOG_SEG_SIZE) != XLOG_SEG_SIZE)
		die("write failed");

	/* fsync now so that dirty buffers don't skew later tests */
	if (fsync(tmpfile) != 0)
		die("fsync failed");

	close(tmpfile);
}

static void
test_sync(int writes_per_op)
{
	int			tmpfile,
				ops,
				writes;
	bool		fs_warning = false;

	if (writes_per_op == 1)
		printf("\nCompare file sync methods using one %dkB write:\n", XLOG_BLCKSZ_K);
	else
		printf("\nCompare file sync methods using two %dkB writes:\n", XLOG_BLCKSZ_K);
	printf("(in wal_sync_method preference order, except fdatasync\n");
	printf("is Linux's default)\n");

	/*
	 * Test open_datasync if available
	 */
	printf(LABEL_FORMAT, "open_datasync");
	fflush(stdout);

#ifdef OPEN_DATASYNC_FLAG
	if ((tmpfile = open(filename, O_RDWR | O_DSYNC | PG_O_DIRECT, 0)) == -1)
	{
		printf(NA_FORMAT, "n/a*\n");
		fs_warning = true;
	}
	else
	{
		if ((tmpfile = open(filename, O_RDWR | O_DSYNC | PG_O_DIRECT, 0)) == -1)
			die("could not open output file");
		gettimeofday(&start_t, NULL);
		for (ops = 0; ops < ops_per_test; ops++)
		{
			for (writes = 0; writes < writes_per_op; writes++)
				if (write(tmpfile, buf, XLOG_BLCKSZ) != XLOG_BLCKSZ)
					die("write failed");
			if (lseek(tmpfile, 0, SEEK_SET) == -1)
				die("seek failed");
		}
		gettimeofday(&stop_t, NULL);
		close(tmpfile);
		print_elapse(start_t, stop_t);
	}
#else
	printf(NA_FORMAT, "n/a\n");
#endif

/*
 * Test fdatasync if available
 */
	printf(LABEL_FORMAT, "fdatasync");
	fflush(stdout);

#ifdef HAVE_FDATASYNC
	if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
		die("could not open output file");
	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		for (writes = 0; writes < writes_per_op; writes++)
			if (write(tmpfile, buf, XLOG_BLCKSZ) != XLOG_BLCKSZ)
				die("write failed");
		fdatasync(tmpfile);
		if (lseek(tmpfile, 0, SEEK_SET) == -1)
			die("seek failed");
	}
	gettimeofday(&stop_t, NULL);
	close(tmpfile);
	print_elapse(start_t, stop_t);
#else
	printf(NA_FORMAT, "n/a\n");
#endif

/*
 * Test fsync
 */
	printf(LABEL_FORMAT, "fsync");
	fflush(stdout);

	if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
		die("could not open output file");
	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		for (writes = 0; writes < writes_per_op; writes++)
			if (write(tmpfile, buf, XLOG_BLCKSZ) != XLOG_BLCKSZ)
				die("write failed");
		if (fsync(tmpfile) != 0)
			die("fsync failed");
		if (lseek(tmpfile, 0, SEEK_SET) == -1)
			die("seek failed");
	}
	gettimeofday(&stop_t, NULL);
	close(tmpfile);
	print_elapse(start_t, stop_t);

/*
 * If fsync_writethrough is available, test as well
 */
	printf(LABEL_FORMAT, "fsync_writethrough");
	fflush(stdout);

#ifdef HAVE_FSYNC_WRITETHROUGH
	if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
		die("could not open output file");
	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		for (writes = 0; writes < writes_per_op; writes++)
			if (write(tmpfile, buf, XLOG_BLCKSZ) != XLOG_BLCKSZ)
				die("write failed");
		if (pg_fsync_writethrough(tmpfile) != 0)
			die("fsync failed");
		if (lseek(tmpfile, 0, SEEK_SET) == -1)
			die("seek failed");
	}
	gettimeofday(&stop_t, NULL);
	close(tmpfile);
	print_elapse(start_t, stop_t);
#else
	printf(NA_FORMAT, "n/a\n");
#endif

/*
 * Test open_sync if available
 */
	printf(LABEL_FORMAT, "open_sync");
	fflush(stdout);

#ifdef OPEN_SYNC_FLAG
	if ((tmpfile = open(filename, O_RDWR | OPEN_SYNC_FLAG | PG_O_DIRECT, 0)) == -1)
	{
		printf(NA_FORMAT, "n/a*\n");
		fs_warning = true;
	}
	else
	{
		gettimeofday(&start_t, NULL);
		for (ops = 0; ops < ops_per_test; ops++)
		{
			for (writes = 0; writes < writes_per_op; writes++)
				if (write(tmpfile, buf, XLOG_BLCKSZ) != XLOG_BLCKSZ)
					die("write failed");
			if (lseek(tmpfile, 0, SEEK_SET) == -1)
				die("seek failed");
		}
		gettimeofday(&stop_t, NULL);
		close(tmpfile);
		print_elapse(start_t, stop_t);
	}
#else
	printf(NA_FORMAT, "n/a\n");
#endif

	if (fs_warning)
	{
		printf("* This file system and its mount options do not support direct\n");
		printf("I/O, e.g. ext4 in journaled mode.\n");
	}
}

static void
test_open_syncs(void)
{
	printf("\nCompare open_sync with different write sizes:\n");
	printf("(This is designed to compare the cost of writing 16kB\n");
	printf("in different write open_sync sizes.)\n");

	test_open_sync("16kB open_sync write", 16);
	test_open_sync(" 8kB open_sync writes", 8);
	test_open_sync(" 4kB open_sync writes", 4);
	test_open_sync(" 2kB open_sync writes", 2);
	test_open_sync(" 1kB open_sync writes", 1);
}

/*
 * Test open_sync with different size files
 */
static void
test_open_sync(const char *msg, int writes_size)
{
#ifdef OPEN_SYNC_FLAG
	int			tmpfile,
				ops,
				writes;
#endif

	printf(LABEL_FORMAT, msg);
	fflush(stdout);

#ifdef OPEN_SYNC_FLAG
	if ((tmpfile = open(filename, O_RDWR | OPEN_SYNC_FLAG | PG_O_DIRECT, 0)) == -1)
		printf(NA_FORMAT, "n/a*\n");
	else
	{
		gettimeofday(&start_t, NULL);
		for (ops = 0; ops < ops_per_test; ops++)
		{
			for (writes = 0; writes < 16 / writes_size; writes++)
				if (write(tmpfile, buf, writes_size * 1024) !=
					writes_size * 1024)
					die("write failed");
			if (lseek(tmpfile, 0, SEEK_SET) == -1)
				die("seek failed");
		}
		gettimeofday(&stop_t, NULL);
		close(tmpfile);
		print_elapse(start_t, stop_t);
	}
#else
	printf(NA_FORMAT, "n/a\n");
#endif
}

static void
test_file_descriptor_sync(void)
{
	int			tmpfile,
				ops;

	/*
	 * Test whether fsync can sync data written on a different descriptor for
	 * the same file.  This checks the efficiency of multi-process fsyncs
	 * against the same file. Possibly this should be done with writethrough
	 * on platforms which support it.
	 */
	printf("\nTest if fsync on non-write file descriptor is honored:\n");
	printf("(If the times are similar, fsync() can sync data written\n");
	printf("on a different descriptor.)\n");

	/*
	 * first write, fsync and close, which is the normal behavior without
	 * multiple descriptors
	 */
	printf(LABEL_FORMAT, "write, fsync, close");
	fflush(stdout);

	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
			die("could not open output file");
		if (write(tmpfile, buf, XLOG_BLCKSZ) != XLOG_BLCKSZ)
			die("write failed");
		if (fsync(tmpfile) != 0)
			die("fsync failed");
		close(tmpfile);

		/*
		 * open and close the file again to be consistent with the following
		 * test
		 */
		if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
			die("could not open output file");
		close(tmpfile);
	}
	gettimeofday(&stop_t, NULL);
	print_elapse(start_t, stop_t);

	/*
	 * Now open, write, close, open again and fsync This simulates processes
	 * fsyncing each other's writes.
	 */
	printf(LABEL_FORMAT, "write, close, fsync");
	fflush(stdout);

	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
			die("could not open output file");
		if (write(tmpfile, buf, XLOG_BLCKSZ) != XLOG_BLCKSZ)
			die("write failed");
		close(tmpfile);
		/* reopen file */
		if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
			die("could not open output file");
		if (fsync(tmpfile) != 0)
			die("fsync failed");
		close(tmpfile);
	}
	gettimeofday(&stop_t, NULL);
	print_elapse(start_t, stop_t);

}

static void
test_non_sync(void)
{
	int			tmpfile,
				ops;

	/*
	 * Test a simple write without fsync
	 */
	printf("\nNon-Sync'ed %dkB writes:\n", XLOG_BLCKSZ_K);
	printf(LABEL_FORMAT, "write");
	fflush(stdout);

	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
			die("could not open output file");
		if (write(tmpfile, buf, XLOG_BLCKSZ) != XLOG_BLCKSZ)
			die("write failed");
		close(tmpfile);
	}
	gettimeofday(&stop_t, NULL);
	print_elapse(start_t, stop_t);
}

#ifdef HAVE_FSYNC_WRITETHROUGH

static int
pg_fsync_writethrough(int fd)
{
#ifdef WIN32
	return _commit(fd);
#elif defined(F_FULLFSYNC)
	return (fcntl(fd, F_FULLFSYNC, 0) == -1) ? -1 : 0;
#else
	errno = ENOSYS;
	return -1;
#endif
}
#endif

/*
 * print out the writes per second for tests
 */
static void
print_elapse(struct timeval start_t, struct timeval stop_t)
{
	double		total_time = (stop_t.tv_sec - start_t.tv_sec) +
	(stop_t.tv_usec - start_t.tv_usec) * 0.000001;
	double		per_second = ops_per_test / total_time;

	printf(OPS_FORMAT "\n", per_second);
}

static void
die(const char *str)
{
	fprintf(stderr, "%s: %s\n", str, strerror(errno));
	exit(1);
}
