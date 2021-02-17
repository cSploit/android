/*
    mount_login.c
    Copyright (C) 1995, 1997 by Volker Lendecke

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

#include "mount_login.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <errno.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char* get_passwd_file(const char* file, const char* server, const char* user) {
	FILE *f;
	char b[2048];

	if (!file) return NULL;		/* you have not password file */
	if (!server) return NULL;	/* something is wrong */
	if (!user) return NULL;
	f = fopen(file, "r");
	if (!f) return NULL;	/* NOENT */
	while (fgets(b, sizeof(b), f)) {
		char* s;
		char* u;
		char* p;
		int l;

		b[sizeof(b) - 1] = 0;
		l = strlen(b);
		while ((l > 0) && (b[l-1] == '\n')) l--;
		b[l] = 0;
		s = strtok(b, "/");
		if (!s) continue;	/* malformed line */
		u = strtok(NULL, ":");
		if (!u) continue;	/* malformed line */
		p = strtok(NULL, ":");
		if (!p) continue;
		if (*s && strcmp(server, s)) continue;
		if (strcmp(user, u)) continue;
		fclose(f);
		return strdup(p);
	}
	fclose(f);
	return NULL;
}

static char* get_passwd_fd(unsigned int fd) {
	size_t idx;
	char b[2048];

	if (fd == ~0U)
		return NULL;
	idx = 0;
	while (1) {
		int r;
		char c;
		
		r = read(fd, &c, 1);
		if (r == 0)
			break;
		if (r == -1) {
			if (errno == -EINTR)
				continue;
			if (errno == -EAGAIN || errno == -EWOULDBLOCK) {
				errexit(78, _("Cannot retrieve password from non-blocking file descriptor\n"));
			}
			errexit(79, _("Cannot retrieve password from file descriptor: %s\n"), strerror(errno));
		}
		if (r != 1) {
			errexit(80, _("Cannot retrieve password from file descriptor\n"));
		}
		if (c == 0 || c == '\n')
			break;
		b[idx++] = c;
		if (idx >= sizeof(b)) {
			errexit(81, _("Password too long\n"));
		}
	}
	b[idx] = 0;
	return strdup(b);
}

int get_passwd(struct ncp_mount_info* info) {
	if (info->password)
		return 0;
	info->password = get_passwd_file(info->passwd_file, info->server, info->user);
	if (info->password)
		return 0;
	info->password = get_passwd_fd(info->passwd_fd);
	if (info->password)
		return 0;
	return 1;
}

static void opt_set_mounted_uid(struct ncp_mount_info* info, const char* param) {
	if (isdigit(param[0])) {
		char* end;

		info->mdata.mounted_uid = strtoul(param, &end, 10);
		if (end && *end && !isspace(*end)) {
			errexit(10, _("owner parameter `%s' is not valid decimal number\n"), param);
		}
	} else {
		struct passwd *pwd = getpwnam(param);

		if (!pwd) {
			errexit(11, _("User `%s' does not exist on this machine\n"), param);
		}
		info->mdata.mounted_uid = pwd->pw_uid;
	}
	if (suser() || myuid == info->mdata.mounted_uid) {
		return;
	}
	errexit(71, _("You are not allowed to set owner to `%s'\n"), param);
}

static void opt_set_uid(struct ncp_mount_info* info, const char* param) {
	if (isdigit(param[0])) {
		char* end;

		info->mdata.uid = strtoul(param, &end, 10);
		if (end && *end && !isspace(*end)) {
			errexit(12, _("uid parameter `%s' is not valid decimal number\n"), param);
		}
	} else {
		struct passwd *pwd = getpwnam(param);

		if (!pwd) {
			errexit(13, _("User `%s' does not exist on this machine\n"), param);
		}
		info->mdata.uid = pwd->pw_uid;
	}
	if (suser() || myuid == info->mdata.uid) {
		return;
	}
	errexit(72, _("You are not allowed to set uid to `%s'\n"), param);
}

static void opt_set_gid(struct ncp_mount_info* info, const char* param) {
	int groups;

	if (isdigit(param[0])) {
		char* end;

		info->mdata.gid = strtoul(param, &end, 10);
		if (end && *end && !isspace(*end)) {
			errexit(14, _("gid parameter `%s' is not valid decimal number\n"), param);
		}
	} else {
		struct group* grp = getgrnam(param);

		if (!grp) {
			errexit(15, _("Group `%s' does not exist on this machine\n"), param);
		}
		info->mdata.gid = grp->gr_gid;
	}
	if (suser() || getgid() == info->mdata.gid) {
		return;
	}

	/* Check supplementary group IDs */
	groups = getgroups(0, NULL);
		
	if (groups < 0) {
		errexit(75, _("Cannot retrieve list of groups you are in\n"));
	}
	if (groups) {
		gid_t* buf;
		int groups2;
		
		buf = (gid_t*)malloc(sizeof(*buf) * groups);
		if (!buf) {
			errexit(74, _("Not enough memory for list of groups\n"));
		}
		groups2 = getgroups(groups, buf);
		if (groups2 > 0) {
			while (groups2--) {
				if (buf[groups2] == info->mdata.gid) {
					free(buf);
					return;
				}
			}
		}
		free(buf);
	}
	errexit(73, _("You are not allowed to set gid to `%s'\n"), param);
}

static void opt_set_passwd(struct ncp_mount_info* info, char* param) {
	size_t pwdlen;
	
	if (!param) {
		pwdlen = 0;
	} else {
		pwdlen = strlen(param);
	}
	if (pwdlen >= sizeof(((const struct ncp_conn_spec*)param)->password)) {
		errexit(16, _("Specified password is too long\n"));
	}
	if (info->password) {
		free(info->password);
	}
	info->password = malloc(pwdlen + 1);
	if (!info->password) {
		errexit(77, _("Not enough memory for copy of password\n"));
	}
	memcpy(info->password, param, pwdlen);
	info->password[pwdlen] = 0;
	if (pwdlen) {
		memset(param, 0, pwdlen);
	}
}

static void opt_set_user(struct ncp_mount_info* info, const char* param) {
	if (param) {
		info->user = param;
	} else {
		/* silently ignore user... it is standard fstab option,
		   we always operate like if user is set, and mount does
		   user=>nodev,noexec,nosuid,user expansion for us */
	}
}

static void opt_set_passwd_file(struct ncp_mount_info* info, const char* param) {
	info->passwd_file = param;
}

static void opt_set_passwd_fd(struct ncp_mount_info* info, unsigned int param) {
	info->passwd_fd = param;
}

static void opt_set_upcase_passwd(struct ncp_mount_info* info, unsigned int param) {
	info->upcase_password = param;
}

static void opt_set_nouser(struct ncp_mount_info* info, unsigned int param) {
	/* Ignore nouser. mount does not pass it to us */
	(void)info;
	(void)param;
}

struct optinfo extopts[] = {
	{'u', "uid",		FN_STRING,	opt_set_uid,		0},
	{'g', "gid",		FN_STRING,	opt_set_gid,		0},
	{'c', "owner",		FN_STRING,	opt_set_mounted_uid,	0},
	{0,   "nopasswd",	FN_NOPARAM,	opt_set_passwd,		0}, /* 'n' without mount.ncp */
	{'P', "passwd",		FN_STRING,	opt_set_passwd,		0},
	{0,   "passwdfile",	FN_STRING,	opt_set_passwd_file,	0},
	{0,   "pass-fd",	FN_INT,		opt_set_passwd_fd,	~0},
	{'U', "user",		FN_STRING | FN_NOPARAM,
						opt_set_user,		0},
	{0,   "nouser",		FN_NOPARAM,	opt_set_nouser,		0},
	{'C', "noupcasepasswd",	FN_NOPARAM,	opt_set_upcase_passwd,	0},
	{0,   NULL,		0,		NULL,			0}
};
