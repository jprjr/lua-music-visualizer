#ifndef JPR_CHAR_H
#define JPR_CHAR_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifdef __cplusplus
extern "C" {
#endif

int char_isdigit(char c);
int char_isupper(char c);
int char_islower(char c);
char char_upper(char c);
char char_lower(char c);

#ifdef __cplusplus
}
#endif

#endif
