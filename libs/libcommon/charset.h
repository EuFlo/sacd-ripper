/*
 * Copyright (C) 1999-2001  Haavard Kvaalen <havardk@xmms.org>
 *
 * Licensed under GNU LGPL version 2.
 *
 */

#ifndef CHARSET_H_INCLUDED
#define CHARSET_H_INCLUDED


char* charset_get_current(void);
char* charset_convert_ext(const char *instring, size_t insizebytes,size_t *outsizebytes_ptr, const char *from, const char *to);
char* charset_convert(const char *string, size_t insizebytes, const char *from, const char *to);
wchar_t* utf8char2wchar(const char *instring);
char* charset_to_utf8(const char *string);
char* charset_from_utf8(const char *string);

#define CHAR2WCHAR(dst, src) dst = utf8char2wchar(src)

#endif  /* CHARSET_H_INCLUDED */
