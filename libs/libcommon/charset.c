/*
 * Copyright (C) 1999-2001  Haavard Kvaalen <havardk@xmms.org>
 *
 * Licensed under GNU LGPL version 2.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>

#ifdef HAVE_CODESET
#include <langinfo.h>
#endif

#include "charset.h"
#include "logging.h"

char* charset_get_current(void)
{
	char *charset = getenv("CHARSET");

#ifdef HAVE_CODESET
	if (!charset)
		charset = nl_langinfo(CODESET);
#endif
	if (!charset)
		charset = "ISO-8859-1";

	return charset;
}

//  Extended version of 'charset_convert' which has added an 'outsizebytes_ptr' parameter
//  Converts a string with insize bytes from  'from' encodings to 'to' encondings
//      instring is input string to be converted
//      insizebytes is size of string in bytes without null-terminator !!!!
//      outsizebytes_ptr contains the size in bytes of returned converted string (without null-terminator char)
//      returns a pointer to the null-terminated converted string or, in case of an error, a clone of the same input instring
//      returns NULL only if instring is NULL or calloc fails.
//  User must free the returned buffer !!!!
char* charset_convert_ext(const char *instring, size_t insizebytes,size_t *outsizebytes_ptr, const char *from, const char *to)
{
	size_t outbytesleft, outbytes_initial, inbytesleft, outsize, nconv;
	iconv_t cd;
	char *out, *outptr;
	const char *input = instring;
	size_t used = 0;
	size_t null_char_width = 4U;
	int retry_no = 0;

	// sanity checks
	if (instring == NULL)
		return NULL;
	if(insizebytes > (size_t)32768U)
		insizebytes =32768U;

	if (!from)
		from = charset_get_current();
	if (!to)
		to = charset_get_current();


	if ((cd = iconv_open(to, from)) == (iconv_t)-1)
	{
		LOG(lm_main, LOG_ERROR, ("charset_convert(): iconv_open() = -1 => Conversion not supported. "
			  "Charsets: %s -> %s", from, to));
		goto Error_case_ret;
	}


	/* + 4 for nul in case len == 1 */
	/* For macOS Sonoma 14.4/15: makes more room because iconv behaves different than GNU and did not have errno == E2BIG !!! */

	outsize = (insizebytes + 1) * null_char_width;	

	out = calloc(1,outsize);
	if(out == NULL)
	{
		LOG(lm_main, LOG_ERROR, ("Error in charset_convert(): calloc() ret NULL!"));		
      	goto Error_case_ret;
	}
	

	outbytesleft = outsize - null_char_width;
	outbytes_initial = outbytesleft;
	outptr = out;
	inbytesleft = insizebytes;

 retry:
#ifdef __lv2ppu__ 
 	if ((nconv=iconv(cd, (const char **) &input, &inbytesleft, &outptr, &outbytesleft)) == (size_t) -1)
#else
 	if ((nconv=iconv(cd,(char **) &input, &inbytesleft, &outptr, &outbytesleft)) == (size_t) -1)
#endif
	{

		switch (errno)
		{
			case E2BIG: /*(need more space - on macOS Sonoma 14.4/14.5 did not land here !!)*/

				used = 0;
				if(outptr - out > 0)
					used = (size_t)(outptr - out);
				else
				{
					if(outbytes_initial >= outbytesleft)
						used = (outbytes_initial - outbytesleft);
					else
						goto Error_case;
				}

				outsize = outsize * 2 + null_char_width;
				
				out = realloc(out, outsize);
				if(out == NULL)
				  goto Error_case;
				outptr = out + used;
				outbytesleft = outsize - used - null_char_width;
				outbytes_initial = outbytesleft;

				if(retry_no < 10)
				{
					retry_no ++;
					goto retry;
				}
				else
					break;
			case EINVAL:
				LOG(lm_main, LOG_ERROR, ("charset_convert() -> case EINVAL; (Incomplete input sequence); "
			  		"input=%p; outsize=: %zu", input,outsize));
				goto Error_case;
				break;
			case EILSEQ: /*ill-formed input sequence*/
				/* Invalid sequence, try to get the rest of the string */
				LOG(lm_main, LOG_ERROR, ("charset_convert() -> case EILSEQ; (Invalid input sequence); "
			  		"input=%p; outptr=%p; outsize=%zu", input,outptr,outsize));								   
				if(input + 1 < instring + insizebytes)
				{
					input++;
					inbytesleft--;
					goto retry;
				}
				break;

			default:
				LOG(lm_main, LOG_ERROR, ("charset_convert() ->  case Default. Conversion failed. Charsets: %s -> %s; Inputstring: %s; "
					  "errno=%X;  Error: %s",
					  from,to,instring,errno, strerror(errno)));
				goto Error_case;
		}
	}

	iconv(cd,NULL,NULL,&outptr,&outbytesleft);
	
    memset(outptr, 0, null_char_width);

	iconv_close(cd);

	if(outsizebytes_ptr != NULL)
		*outsizebytes_ptr = outbytes_initial - outbytesleft;

	return out;

Error_case:
	
	iconv_close(cd);
	free(out);

Error_case_ret:
	LOG(lm_main, LOG_ERROR, ("charset_convert() ->  Error_case_ret"));
	// just return the clone of the input instring if calloc did not fail 
	out = calloc(1,insizebytes + null_char_width); // add null terminator
	if(out != NULL)
	{
		memcpy(out,instring,insizebytes);
		if(outsizebytes_ptr != NULL) *outsizebytes_ptr = insizebytes;
	}
	else
		if(outsizebytes_ptr != NULL) *outsizebytes_ptr = 0;

	return out;
}

// converts a string with insize bytes from  'from' encodings to 'to' encondings
//      instring is input string to be converted
//      insize is size of string in bytes !!!!
//      returns a pointer to the converted string or, 
//                         in case of an error, a clone of the same input instring
//      returns NULL only if instring is NULL or calloc fails.
//  User must free the returned buffer !!!!
char* charset_convert(const char *instring, size_t insizebytes, const char *from, const char *to)
{

	return charset_convert_ext(instring, insizebytes, NULL, from, to);
}


wchar_t* utf8char2wchar(const char *instring)
{
	wchar_t *out_wide_ptr = NULL;
	size_t outsizebytes;
	char *out_char_tmp;

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
	out_char_tmp = charset_convert_ext(instring, strlen(instring), &outsizebytes, "UTF-8", "UCS-2-INTERNAL");
#else
	out_char_tmp = charset_convert_ext(instring, strlen(instring), &outsizebytes, "UTF-8", "WCHAR_T");
#endif

    if(out_char_tmp != NULL && outsizebytes > 0)
	{
		size_t nbr_of_wchar = outsizebytes / sizeof(wchar_t);
		out_wide_ptr = calloc(nbr_of_wchar + 1, sizeof(wchar_t)); // +1 for null-terminator
		memcpy(out_wide_ptr, out_char_tmp, outsizebytes);
		free(out_char_tmp);

		return out_wide_ptr;
	}
	else
		return NULL;
}

char* charset_from_utf8(const char *string)
{
	if (!string)
		return NULL;
	return charset_convert_ext(string, strlen(string), NULL, "UTF-8", NULL);
}

char* charset_to_utf8(const char *string)
{
	if (!string)
		return NULL;
	return charset_convert_ext(string, strlen(string), NULL, NULL, "UTF-8");
}
