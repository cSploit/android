

#ifndef EC_FILE_H
#define EC_FILE_H

EC_API_EXTERN FILE * open_data(char *dir, char *file, char *mode);
EC_API_EXTERN char * get_full_path(const char *dir, const char *file);
EC_API_EXTERN char * get_local_path(const char *file);

#define MAC_FINGERPRINTS   "etter.finger.mac"
#define TCP_FINGERPRINTS   "etter.finger.os"
#define SERVICES_NAMES     "etter.services"
#define ETTER_CONF         "etter.conf"
#define ETTER_DNS          "etter.dns"
#define ETTER_MDNS         "etter.mdns"
#define ETTER_NBNS         "etter.nbns"
#define ETTER_FIELDS       "etter.fields"
#define CERT_FILE          "etter.ssl.crt"

/* fopen modes */
#define FOPEN_READ_TEXT   "r"                                                                   
#define FOPEN_READ_BIN    "rb"                                                                   
#define FOPEN_WRITE_TEXT  "w"                                                                   
#define FOPEN_WRITE_BIN   "wb"

#endif

/* EOF */

// vim:ts=3:expandtab

