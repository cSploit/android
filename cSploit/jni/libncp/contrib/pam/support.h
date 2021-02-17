#ifndef __SUPPORT_H__
#define __SUPPORT_H__

#ifndef LINUX

#include <security/pam_appl.h>

#endif  /* LINUX */

#define _PAM_EXTERN_FUNCTIONS
#include <security/pam_modules.h>

int _set_auth_tok(pam_handle_t *pamh, int flags);
int _set_oldauth_tok(pam_handle_t *pamh, int flags);
int _read_new_pwd(pam_handle_t *pamh, int flags);

#define UNUSED(x)	x __attribute__((unused))

static inline int my_pam_get_item(const pam_handle_t *pamh,
				  int item_type,
				  void *item) {
	return pam_get_item(pamh, item_type, (const void **)item);
}

static inline int my_pam_get_data(const pam_handle_t *pamh,
				  const char *module_data_name,
				  void *data) {
	return pam_get_data(pamh, module_data_name, (const void **)data);
}

 
#endif /* __SUPPORT_H__ */

