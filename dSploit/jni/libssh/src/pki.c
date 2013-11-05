/*
 * known_hosts.c
 * This file is part of the SSH Library
 *
 * Copyright (c) 2010 by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

/**
 * @defgroup libssh_pki The SSH Public Key Infrastructure
 * @ingroup libssh
 *
 * Functions for the creation, importation and manipulation of public and
 * private keys in the context of the SSH protocol
 *
 * @{
 */

#include "libssh/priv.h"
#include "libssh/pki.h"
#include "libssh/keys.h"

/**
 * @brief creates a new empty SSH key
 * @returns an empty ssh_key handle, or NULL on error.
 */
ssh_key ssh_key_new (void) {
  ssh_key ptr = malloc (sizeof (struct ssh_key_struct));
  if (ptr == NULL) {
      return NULL;
  }
  ZERO_STRUCTP(ptr);
  return ptr;
}

/**
 * @brief clean up the key and deallocate all existing keys
 * @param[in] key ssh_key to clean
 */
void ssh_key_clean (ssh_key key){
  if(key==NULL)
    return;
#ifdef HAVE_LIBGCRYPT
  gcry_sexp_release(key->dsa);
  gcry_sexp_release(key->rsa);
#elif defined HAVE_LIBCRYPTO
  DSA_free(key->dsa);
  RSA_free(key->rsa);
#endif
  key->flags=SSH_KEY_FLAG_EMPTY;
  key->type=SSH_KEYTYPE_UNKNOWN;
  key->type_c=NULL;
}

/**
 * @brief deallocate a SSH key
 * @param[in] key ssh_key handle to free
 */
void ssh_key_free (ssh_key key){
  if(key){
    ssh_key_clean(key);
    SAFE_FREE(key);
  }
}

/**
 * @brief returns the type of a ssh key
 * @param[in] key the ssh_key handle
 * @returns one of SSH_KEYTYPE_RSA,SSH_KEYTYPE_DSS,SSH_KEYTYPE_RSA1
 * @returns SSH_KEYTYPE_UNKNOWN if the type is unknown
 */
enum ssh_keytypes_e ssh_key_type(ssh_key key){
  if (key==NULL)
    return SSH_KEYTYPE_UNKNOWN;
  return key->type;
}

/**
 * @brief import a key from a file
 * @param[out]  key      the ssh_key to update
 * @param[in]  session  The SSH Session to use. If a key decryption callback is set, it will
 *                      be used to ask for the passphrase.
 * @param[in]  filename The filename of the the private key.
 * @param[in]  passphrase The passphrase to decrypt the private key. Set to null
 *                        if none is needed or it is unknown.
 * @returns SSH_OK on success, SSH_ERROR otherwise.
 **/
int ssh_key_import_private(ssh_key key, ssh_session session, const char *filename, const char *passphrase){
  ssh_private_key priv=privatekey_from_file(session,filename,0,passphrase);
  if(priv==NULL)
    return SSH_ERROR;
  ssh_key_clean(key);
  key->dsa=priv->dsa_priv;
  key->rsa=priv->rsa_priv;
  key->type=priv->type;
  key->flags=SSH_KEY_FLAG_PRIVATE | SSH_KEY_FLAG_PUBLIC;
  key->type_c=ssh_type_to_char(key->type);
  SAFE_FREE(priv);
  return SSH_OK;
}

/**
 * @}
 */
