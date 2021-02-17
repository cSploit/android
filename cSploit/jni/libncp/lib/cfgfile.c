/*
    cfgfile.c - Configuration file handling
    Copyright (C) 2000  Petr Vandrovec

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

    Revision history:

	1.00  2000, July 8		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.01  2001, March 9		Petr Vandrovec <vandrove@vc.cvut.cz>
		Add getCfgItemW. Config file is UTF-8...

	1.02  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Add include strings.h.

 */

#include "config.h"

#include "nwnet_i.h"

#include "cfgfile.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>

#ifndef GLOBALCFGFILE
#error "GLOBALCFGFILE must be defined"
#endif

struct cfgFile {
		ncpt_mutex_t	mutex;
		FILE*		file;
};

static struct cfgFile* cfgOpenFile(
		const char* path,
		int writeRequired) {
	struct cfgFile* cfg;

	cfg = (struct cfgFile*)malloc(sizeof(*cfg));
	if (cfg) {
		cfg->file = fopen(path, writeRequired ? "r+" : "r");
		if (cfg->file) {
			ncpt_mutex_init(&cfg->mutex);
		} else {
			free(cfg);
			cfg = NULL;
		}
	}
	return cfg;
}

static void cfgClose(
		struct cfgFile* cfg) {
	ncpt_mutex_lock(&cfg->mutex);
	fclose(cfg->file);
	ncpt_mutex_destroy(&cfg->mutex);
	free(cfg);
}

static char* cfgFindKey(struct cfgFile* cfg, 
		const char* section, const char* key) {
	char cfgline[16384];
	size_t seclen = strlen(section);
	size_t keylen = strlen(key);

	ncpt_mutex_lock(&cfg->mutex);
	rewind(cfg->file);
	while (fgets(cfgline, sizeof(cfgline)-1, cfg->file)) {
		char* cptr = cfgline;

		while (*cptr && isspace(*cptr))
			cptr++;
		if (*cptr != '[')
			continue;
sstart:;
		if (strncasecmp(++cptr, section, seclen))
			continue;
		if (cptr[seclen] != ']')
			continue;
		while (fgets(cfgline, sizeof(cfgline) - 1, cfg->file)) {
			char* sptr;
			char ec;
			char cc;

			cptr = cfgline;

			while (*cptr && isspace(*cptr))
				cptr++;
			if (*cptr == '[')
				goto sstart;
			if (strncasecmp(cptr, key, keylen))
				continue;
			cptr += keylen;
			while (*cptr && isspace(*cptr))
				cptr++;
			if (*cptr != '=' && *cptr != ':')
				continue;
			cptr++;
			while (*cptr && isspace(*cptr))
				cptr++;
			sptr = cfgline;
			if (*cptr == '"' || *cptr == '\'')
				ec = *cptr++;
			else
				ec = 0;
			while ((cc = *cptr++) != 0) {
				if (cc == '\n')
					break;
				if (!ec && isspace(cc)) {
					char* cp = sptr;
					
					do {
						*cp++ = cc;
						cc = *cptr++;
					} while (cc && cc != '\n' && isspace(cc));
					if (!cc || cc == '\n' || cc == '#' || cc == ';') {
						break;
					}
					sptr = cp;
				}
				if (cc == ec)
					break;
				if (cc == '\\') {
					cc = *cptr++;
					if (!cc)
						break;
					if (cc == '\n')
						break;
					switch (cc) {
						case '#':
						case ';':
						case ' ':
							/* List these explicitly so it is clear that they are used/supported */
							break;
						case 't':
							cc = '\t'; break;
						case 'n':
							cc = '\n'; break;
						case 'r':
							cc = '\r'; break;
					}
				}
				*sptr++ = cc;
			}
			*sptr = 0;

			ncpt_mutex_unlock(&cfg->mutex);
			return strdup(cfgline);
		}
		break;
	}
	ncpt_mutex_unlock(&cfg->mutex);
	return NULL;
}

static struct {
	ncpt_mutex_t	lock;
	int		valid;
	char*		path;
} localCfg = { NCPT_MUTEX_INITIALIZER, 0, NULL };

static struct cfgFile* cfgOpenLocalCfg(int writeRequired) {
	struct cfgFile* f = NULL;

	ncpt_mutex_lock(&localCfg.lock);
	if (!localCfg.valid) {
		char* home;

		home = getenv("HOME");
		if (home) {
			size_t ln;
			char* pathenv;

			ln = strlen(home);
			pathenv = (char*)malloc(ln + 20);
			if (pathenv) {
				struct stat stb;

				memcpy(pathenv, home, ln);
				strcpy(pathenv + ln, "/.nwclient");
				if (!stat(pathenv, &stb)) {
					if (S_ISREG(stb.st_mode)) {
						localCfg.valid = 1;
						localCfg.path = pathenv;
						pathenv = NULL;
					}
				}
				if (pathenv)
					free(pathenv);
			}
		}
	}
	if (localCfg.valid) {
		f = cfgOpenFile(localCfg.path, writeRequired);
	}
	ncpt_mutex_unlock(&localCfg.lock);
	return f;
}

char* cfgGetItem(const char* section, const char* key) {
	struct cfgFile *cfg;
	char* reply;

	cfg = cfgOpenLocalCfg(0);
	if (cfg) {
		reply = cfgFindKey(cfg, section, key);
		cfgClose(cfg);
		if (reply)
			return reply;
	}
	cfg = cfgOpenFile(GLOBALCFGFILE, 0);
	if (cfg) {
		reply = cfgFindKey(cfg, section, key);
		cfgClose(cfg);
		if (reply) {
#if 0
			fprintf(stderr, "Match: [%s]:%s => %s\n",
				section, key, reply);
#endif
			return reply;
		}
	}
	return NULL;
}

#ifdef NDS_SUPPORT
wchar_t* cfgGetItemW(const char* section, const char* key) {
	char* val;
	wchar_t* wval;
	size_t len;
	
	val = cfgGetItem(section, key);
	if (!val)
		return NULL;
	len = (strlen(val) + 1) * sizeof(wchar_t);
	wval = (wchar_t*)malloc(len);
	if (wval) {
		if (iconv_external_to_wchar_t(val, wval, len)) {
			free(wval);
			wval = NULL;
		}
	}
	free(val);
	return wval;
}
#endif
