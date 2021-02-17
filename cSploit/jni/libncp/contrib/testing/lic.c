#include <ncp/nwcalls.h>

int main(int argc, char* argv[]) {
	NWCONN_HANDLE cn;
	NWCCODE err;
	int ret = 99;

	err = ncp_open_mount(argv[1], &cn);
	if (err) {
		fprintf(stderr, "Cannot open connection: %s\n", strnwerror(err));
		return ret;
	}
	err = ncp_change_conn_state(cn, argc > 2 ? 1 : 0);
	if (err) {
		fprintf(stderr, "Cannot [un]license connection: %s\n", strnwerror(err));
	} else {
		ret = 0;
	}
	NWCCCloseConn(cn);
	return ret;
}
	
