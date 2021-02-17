/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"
#include <stdio.h>
#include "slap.h"

#ifdef SLAPD_MODULES

#include <ltdl.h>

typedef int (*MODULE_INIT_FN)(
	int argc,
	char *argv[]);
typedef int (*MODULE_LOAD_FN)(
	const void *module,
	const char *filename);
typedef int (*MODULE_TERM_FN)(void);


struct module_regtable_t {
	char *type;
	MODULE_LOAD_FN proc;
} module_regtable[] = {
		{ "null", load_null_module },
#ifdef SLAPD_EXTERNAL_EXTENSIONS
		{ "extension", load_extop_module },
#endif
		{ NULL, NULL }
};

typedef struct module_loaded_t {
	struct module_loaded_t *next;
	lt_dlhandle lib;
	char name[1];
} module_loaded_t;

module_loaded_t *module_list = NULL;

static int module_int_unload (module_loaded_t *module);

#ifdef HAVE_EBCDIC
static char ebuf[BUFSIZ];
#endif

int module_init (void)
{
	if (lt_dlinit()) {
		const char *error = lt_dlerror();
#ifdef HAVE_EBCDIC
		strcpy( ebuf, error );
		__etoa( ebuf );
		error = ebuf;
#endif
		Debug(LDAP_DEBUG_ANY, "lt_dlinit failed: %s\n", error, 0, 0);

		return -1;
	}

	return module_path( LDAP_MODULEDIR );
}

int module_kill (void)
{
	/* unload all modules before shutdown */
	while (module_list != NULL) {
		module_int_unload(module_list);
	}

	if (lt_dlexit()) {
		const char *error = lt_dlerror();
#ifdef HAVE_EBCDIC
		strcpy( ebuf, error );
		__etoa( ebuf );
		error = ebuf;
#endif
		Debug(LDAP_DEBUG_ANY, "lt_dlexit failed: %s\n", error, 0, 0);

		return -1;
	}
	return 0;
}

void * module_handle( const char *file_name )
{
	module_loaded_t *module;

	for ( module = module_list; module; module= module->next ) {
		if ( !strcmp( module->name, file_name )) {
			return module;
		}
	}
	return NULL;
}

int module_unload( const char *file_name )
{
	module_loaded_t *module;

	module = module_handle( file_name );
	if ( module ) {
		module_int_unload( module );
		return 0;
	}
	return -1;	/* not found */
}

int module_load(const char* file_name, int argc, char *argv[])
{
	module_loaded_t *module;
	const char *error;
	int rc;
	MODULE_INIT_FN initialize;
#ifdef HAVE_EBCDIC
#define	file	ebuf
#else
#define	file	file_name
#endif

	module = module_handle( file_name );
	if ( module ) {
		Debug( LDAP_DEBUG_ANY, "module_load: (%s) already loaded\n",
			file_name, 0, 0 );
		return -1;
	}

	/* If loading a backend, see if we already have it */
	if ( !strncasecmp( file_name, "back_", 5 )) {
		char *name = (char *)file_name + 5;
		char *dot = strchr( name, '.');
		if (dot) *dot = '\0';
		rc = backend_info( name ) != NULL;
		if (dot) *dot = '.';
		if ( rc ) {
			Debug( LDAP_DEBUG_CONFIG, "module_load: (%s) already present (static)\n",
				file_name, 0, 0 );
			return 0;
		}
	} else {
		/* check for overlays too */
		char *dot = strchr( file_name, '.' );
		if ( dot ) *dot = '\0';
		rc = overlay_find( file_name ) != NULL;
		if ( dot ) *dot = '.';
		if ( rc ) {
			Debug( LDAP_DEBUG_CONFIG, "module_load: (%s) already present (static)\n",
				file_name, 0, 0 );
			return 0;
		}
	}

	module = (module_loaded_t *)ch_calloc(1, sizeof(module_loaded_t) +
		strlen(file_name));
	if (module == NULL) {
		Debug(LDAP_DEBUG_ANY, "module_load failed: (%s) out of memory\n", file_name,
			0, 0);

		return -1;
	}
	strcpy( module->name, file_name );

#ifdef HAVE_EBCDIC
	strcpy( file, file_name );
	__atoe( file );
#endif
	/*
	 * The result of lt_dlerror(), when called, must be cached prior
	 * to calling Debug. This is because Debug is a macro that expands
	 * into multiple function calls.
	 */
	if ((module->lib = lt_dlopenext(file)) == NULL) {
		error = lt_dlerror();
#ifdef HAVE_EBCDIC
		strcpy( ebuf, error );
		__etoa( ebuf );
		error = ebuf;
#endif
		Debug(LDAP_DEBUG_ANY, "lt_dlopenext failed: (%s) %s\n", file_name,
			error, 0);

		ch_free(module);
		return -1;
	}

	Debug(LDAP_DEBUG_CONFIG, "loaded module %s\n", file_name, 0, 0);

   
#ifdef HAVE_EBCDIC
#pragma convlit(suspend)
#endif
	if ((initialize = lt_dlsym(module->lib, "init_module")) == NULL) {
#ifdef HAVE_EBCDIC
#pragma convlit(resume)
#endif
		Debug(LDAP_DEBUG_CONFIG, "module %s: no init_module() function found\n",
			file_name, 0, 0);

		lt_dlclose(module->lib);
		ch_free(module);
		return -1;
	}

	/* The imported init_module() routine passes back the type of
	 * module (i.e., which part of slapd it should be hooked into)
	 * or -1 for error.  If it passes back 0, then you get the 
	 * old behavior (i.e., the library is loaded and not hooked
	 * into anything).
	 *
	 * It might be better if the conf file could specify the type
	 * of module.  That way, a single module could support multiple
	 * type of hooks. This could be done by using something like:
	 *
	 *    moduleload extension /usr/local/openldap/whatever.so
	 *
	 * then we'd search through module_regtable for a matching
	 * module type, and hook in there.
	 */
	rc = initialize(argc, argv);
	if (rc == -1) {
		Debug(LDAP_DEBUG_CONFIG, "module %s: init_module() failed\n",
			file_name, 0, 0);

		lt_dlclose(module->lib);
		ch_free(module);
		return rc;
	}

	if (rc >= (int)(sizeof(module_regtable) / sizeof(struct module_regtable_t))
		|| module_regtable[rc].proc == NULL)
	{
		Debug(LDAP_DEBUG_CONFIG, "module %s: unknown registration type (%d)\n",
			file_name, rc, 0);

		module_int_unload(module);
		return -1;
	}

	rc = (module_regtable[rc].proc)(module, file_name);
	if (rc != 0) {
		Debug(LDAP_DEBUG_CONFIG, "module %s: %s module could not be registered\n",
			file_name, module_regtable[rc].type, 0);

		module_int_unload(module);
		return rc;
	}

	module->next = module_list;
	module_list = module;

	Debug(LDAP_DEBUG_CONFIG, "module %s: %s module registered\n",
		file_name, module_regtable[rc].type, 0);

	return 0;
}

int module_path(const char *path)
{
#ifdef HAVE_EBCDIC
	strcpy(ebuf, path);
	__atoe(ebuf);
	path = ebuf;
#endif
	return lt_dlsetsearchpath( path );
}

void *module_resolve (const void *module, const char *name)
{
#ifdef HAVE_EBCDIC
	strcpy(ebuf, name);
	__atoe(ebuf);
	name = ebuf;
#endif
	if (module == NULL || name == NULL)
		return(NULL);
	return(lt_dlsym(((module_loaded_t *)module)->lib, name));
}

static int module_int_unload (module_loaded_t *module)
{
	module_loaded_t *mod;
	MODULE_TERM_FN terminate;

	if (module != NULL) {
		/* remove module from tracking list */
		if (module_list == module) {
			module_list = module->next;
		} else {
			for (mod = module_list; mod; mod = mod->next) {
				if (mod->next == module) {
					mod->next = module->next;
					break;
				}
			}
		}

		/* call module's terminate routine, if present */
#ifdef HAVE_EBCDIC
#pragma convlit(suspend)
#endif
		if ((terminate = lt_dlsym(module->lib, "term_module"))) {
#ifdef HAVE_EBCDIC
#pragma convlit(resume)
#endif
			terminate();
		}

		/* close the library and free the memory */
		lt_dlclose(module->lib);
		ch_free(module);
	}
	return 0;
}

int load_null_module (const void *module, const char *file_name)
{
	return 0;
}

#ifdef SLAPD_EXTERNAL_EXTENSIONS
int
load_extop_module (
	const void *module,
	const char *file_name
)
{
	SLAP_EXTOP_MAIN_FN *ext_main;
	SLAP_EXTOP_GETOID_FN *ext_getoid;
	struct berval oid;
	int rc;

	ext_main = (SLAP_EXTOP_MAIN_FN *)module_resolve(module, "ext_main");
	if (ext_main == NULL) {
		return(-1);
	}

	ext_getoid = module_resolve(module, "ext_getoid");
	if (ext_getoid == NULL) {
		return(-1);
	}

	rc = (ext_getoid)(0, &oid, 256);
	if (rc != 0) {
		return(rc);
	}
	if (oid.bv_val == NULL || oid.bv_len == 0) {
		return(-1);
	}

	/* FIXME: this is broken, and no longer needed, 
	 * as a module can call load_extop() itself... */
	rc = load_extop( &oid, ext_main );
	return rc;
}
#endif /* SLAPD_EXTERNAL_EXTENSIONS */
#endif /* SLAPD_MODULES */

