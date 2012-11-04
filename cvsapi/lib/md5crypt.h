#ifndef MD5CRYPT__H
#define MD5CRYPT__H

#ifdef __cplusplus
extern "C" {
#endif

char *ufc_crypt(const char *key, const char *salt); // Linked externally
char *md5_crypt(const char *pw, const char *salt);
int compare_crypt(const char *text, const char *crypt_pw);

#ifdef __cplusplus
}
#endif

#endif
