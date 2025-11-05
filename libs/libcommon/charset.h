/*
 * Copyright (C) 1999-2001  Haavard Kvaalen <havardk@xmms.org>
 *
 * Licensed under GNU LGPL version 2.
 *
 */

#ifndef CHARSET_H_INCLUDED
#define CHARSET_H_INCLUDED

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)

#define CHAR2WCHAR(dst, src) dst = (wchar_t *)charset_convert(src, strlen(src), "UTF-8", "UCS-2-INTERNAL")
#else

#define CHAR2WCHAR(dst, src) dst = (wchar_t *)charset_convert(src, strlen(src), "UTF-8", "WCHAR_T") 
#endif

char* charset_get_current(void);
char* charset_convert(const char *string, size_t insizebytes, const char *from, const char *to);
char* charset_to_utf8(const char *string);
char* charset_from_utf8(const char *string);

#endif  /* CHARSET_H_INCLUDED */
