
#ifndef _FS_CLIB_H_
#define _FS_CLIB_H_
#include "def.h"





U8                  *FS__CLIB_strchr(const char *s, int c);
U32                 FS__CLIB_strlen(const char *s);
int                 FS__CLIB_strncmp(const char *s1, const char *s2, U32 n);
int                 FS__CLIB_strcmp(const char *s1, const char *s2);
int                 FS__CLIB_atoi(const char *s);
void                *FS__CLIB_memset(void *s, int c, U32 n);
void                *FS__CLIB_memcpy(void *s1, const void *s2, U32 n);
char                *FS__CLIB_strncpy(char *s1, const char *s2,U32 n);
int                 FS__CLIB_toupper(int c);




#endif