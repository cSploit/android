#ifndef __NCPM_COMMON_H__
#define __NCPM_COMMON_H__

#define CONFIG_NATIVE_UNIX

#include "config.h"

#include <ncp/kernel/ncp.h>
#include <ncp/kernel/ncp_fs.h>
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <stdarg.h>
#include <sys/utsname.h>
#include <sys/stat.h>

#include "ncpmount.h"       

uid_t myuid;
uid_t myeuid;
char *progname;
char mount_point[MAXPATHLEN + 1];

static inline int suser(void) {
	return myuid == 0;
}

void veprintf(const char* message, va_list va);
void eprintf(const char* message, ...);
void errexit(int ecode, const char* message, ...) __attribute__((noreturn));

struct ncp_mount_data_independent {
	int		ncp_fd;
	int		wdog_fd;
	int		message_fd;
	uid_t		mounted_uid;
	struct sockaddr_ipx serv_addr;
	unsigned char	*server_name;
	unsigned char   *mount_point;
	const unsigned char   *mounted_vol;
	unsigned int	time_out;
	unsigned int	retry_count;
	struct {
	unsigned int	mount_soft:1;
	unsigned int    mount_intr:1;
	unsigned int	mount_strong:1;
	unsigned int	mount_no_os2:1;
	unsigned int	mount_no_nfs:1;
	unsigned int	mount_extras:1;
	unsigned int	mount_symlinks:1;
	unsigned int	mount_nfs_extras:1;
		      } flags;
	uid_t		uid;
	gid_t		gid;
	mode_t		file_mode;
	mode_t		dir_mode;
	enum NET_ADDRESS_TYPE nt_type;
};

void verify_argv(int argc, char* argv[]);
int ncp_mount_specific(struct ncp_conn* conn, int pathNS, const unsigned char* NWpath, int pathlen);
int mount_ok(struct stat *st);
void mycom_err(int, const char*, ...);
void add_mnt_entry(char* mount_name, char* mpoint, unsigned long flags);

struct ncp_mount_info {
	struct ncp_mount_data_independent mdata;
	struct ncp_nls_ioctl nls_cs;
	int		version;
	unsigned int	flags;
	const char*	tree;
	const char*	server;
	const char*	volume;
	int		upcase_password;
       	const char*	user;
	char*		password;
	const char*	server_name;
       	const char*	passwd_file;
	unsigned int	passwd_fd;
	int		allow_multiple_connections;
	int		sig_level;
	int		force_bindery_login;
	const char*	remote_path;
	int		dentry_ttl;
	struct {
		int	valid;
		enum NET_ADDRESS_TYPE value;
		      } nt;
	unsigned int	pathlen;
	unsigned char	NWpath[512];

/*added in ncplogin/ncpmap*/
        char     serverName  [MAX_SCHEMA_NAME_CHARS+1];
        char     resourceName[MAX_SCHEMA_NAME_CHARS+1];
        char     context     [MAX_TREE_NAME_CHARS +1];
        char     defaultUser [MAX_SCHEMA_NAME_CHARS +1];
        const char*     root_path;  /* new argument -R */
        int             autocreate; /* new argument -a in ncpmap ( forced in ncplogin*/
        int             bcastmode; /*  new argument -B mode (default =0)*/
        int             echo_mnt_pnt; /*print out the mounted point if success*/
	const char*	use_local_mnt_dir; /* use /mnt/ncp/$USER instead of $HOME/ncp */
/*end additions*/
	const char*	auth_src;	/* ncpmount -A ... */
};

void init_mount_info(struct ncp_mount_info *info);
int ncp_mount(const char* mount_name, struct ncp_mount_info *info);

struct optinfo {
	char		shortopt;
	const char*	option;
#define FN_NOPARAM	1
#define FN_INT		2
#define FN_STRING	4
	int		flags;
	void		*proc;
	unsigned int	param;
};

void proc_option(const struct optinfo* opts, struct ncp_mount_info* info, const char* opt, const char* param);
int proc_buildconn(struct ncp_mount_info* info);
int proc_aftermount(const struct ncp_mount_info* info, NWCONN_HANDLE* conn);
int proc_ncpm_umount(const char* dir);

void block_sigs(void);
void unblock_sigs(void);

#define UNUSED(x)	x __attribute__((unused))

#endif	/* __NCPM_COMMON_H__ */
