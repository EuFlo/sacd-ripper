/**
 * SACD Ripper - https://github.com/sacd-ripper/
 *
 * Copyright (c) 2010-2015 by respective authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <stdint.h>
#include <ctype.h>
#include <wchar.h>

#include "utils.h"
#include "charset.h"
#include "logging.h"

#define MAX_BUF_SUBSTR 1024
char *substr(const char *pstr, int start, int insizebytes)
{
    static char buf_out[MAX_BUF_SUBSTR];
    char *p_char_buf_tmp,*p_char_buf_tmp2;
    char *wchar_type;
    size_t outsizebytes;

    memset(buf_out, 0, MAX_BUF_SUBSTR);
    if (insizebytes < MAX_BUF_SUBSTR - 4)  // -4 makes room for null-terminator
    {
#ifdef _WIN32
        wchar_type = (sizeof(wchar_t) == 2) ? "UCS-2-INTERNAL" : "UCS-4-INTERNAL";
#else
        wchar_type = "WCHAR_T";
#endif
        p_char_buf_tmp = charset_convert_ext((char *) (pstr + start), insizebytes, &outsizebytes, "UTF-8", wchar_type);
        if(p_char_buf_tmp != NULL && outsizebytes > 0)
        {
                size_t outsizebytes2;
                size_t insizebytes2 = outsizebytes;

                p_char_buf_tmp2 = charset_convert_ext(p_char_buf_tmp, insizebytes2, &outsizebytes2, wchar_type, "UTF-8");
                free(p_char_buf_tmp);

                if(p_char_buf_tmp2 != NULL && outsizebytes2 > 0 && outsizebytes2 < MAX_BUF_SUBSTR - 4)
                { 
                        memcpy(buf_out, p_char_buf_tmp2, outsizebytes2);
                        free(p_char_buf_tmp2);
                }
        }
    }

    return buf_out;
}

char *str_replace(const char *src, const char *from, const char *to)
{
    size_t size    = strlen(src) + 1;
    size_t fromlen = strlen(from);
    size_t tolen   = strlen(to);
    char *value = calloc(size,sizeof(char));
    char *dst = value;
    if (value != NULL)
    {
        for ( ;; )
        {
            const char *match = strstr(src, from);
            if ( match != NULL )
            {
                size_t count = match - src;
                size_t dst_pos_prev;
                char *temp;
                size += tolen - fromlen;
                dst_pos_prev = dst - value;
                temp = realloc(value, size);
                if ( temp == NULL )
                {
                    free(value);
                    return NULL;
                }
                dst = temp + dst_pos_prev;
                value = temp;
                memmove(dst, src, count);
                src += count;
                dst += count;
                memmove(dst, to, tolen);
                src += fromlen;
                dst += tolen;
            }
            else /* No match found. */
            {
                strcpy(dst, src);
                break;
            }
        }
    }
    return value;
}

void replace_double_space_with_single(char *str)
{
    const char *match;
    char *ret;
    do 
    {
        ret = str_replace(str, "  ", " ");
        if (ret)
        {
            strcpy(str, ret);
            free(ret);
        }
        match = strstr(str, "  ");
    } 
    while (match != 0);
}

// removes all instances of bad characters from the string
//
// str - the string to trim
// bad - the sting containing all the characters to remove
void trim_chars(char * str, const char * bad)
{
    int      i;
    int      pos;
    int      len = strlen(str);
    unsigned b;

    if(str == NULL || bad == NULL)return;
    if(len > 16384)len=16384;

    for (b = 0; b < strlen(bad); b++)
    {
        pos = 0;
        for (i = 0; i < len + 1; i++)
        {
            if (str[i] != bad[b])
            {
                str[pos] = str[i];
                pos++;
            }
        }
    }
}

// removes leading and trailing whitespace as defined by isspace()
//
// str - the string to trim
void trim_whitespace(char * s) 
{
    uint8_t * p;
    //int l = strlen((char *) p);
    int l;
    // do some checks
    if(s == NULL) return;

    p = (uint8_t *) s;
    l = strlen(s);
    if(l == 0) return;
    if(l > 16384) l = 16384;

    while(isspace((int) p[l - 1])) p[--l] = 0x00;
    while(* p && isspace((int) *p)) ++p, --l;

    memmove(s, p, l + 1);
}

const char hex_asc[] = "0123456789abcdef";

#define hex_asc_lo(x)   hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x)   hex_asc[((x) & 0xf0) >> 4]

/**
 * hex_dump_to_buffer - convert a blob of data to "hex ASCII" in memory
 * @buf: data blob to dump
 * @len: number of bytes in the @buf
 * @rowsize: number of bytes to print per line; must be 16 or 32
 * @groupsize: number of bytes to print at a time (1, 2, 4, 8; default = 1)
 * @linebuf: where to put the converted data
 * @linebuflen: total size of @linebuf, including space for terminating NUL
 * @ascii: include ASCII after the hex output
 *
 * hex_dump_to_buffer() works on one "line" of output at a time, i.e.,
 * 16 or 32 bytes of input data converted to hex + ASCII output.
 *
 * Given a buffer of uint8_t data, hex_dump_to_buffer() converts the input data
 * to a hex + ASCII dump at the supplied memory location.
 * The converted output is always NUL-terminated.
 *
 * E.g.:
 *   hex_dump_to_buffer(frame->data, frame->len, 16, 1,
 *                      linebuf, sizeof(linebuf), true);
 *
 * example output buffer:
 * 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f  @ABCDEFGHIJKLMNO
 */
void hex_dump_to_buffer(const void *buf, int len, int rowsize,
                        int groupsize, char *linebuf, int linebuflen,
                        int ascii)
{
        const uint8_t *ptr = buf;
        uint8_t ch;
        int j, lx = 0;
        int ascii_column;

        if (rowsize != 16 && rowsize != 32)
                rowsize = 16;

        if (!len)
                goto nil;
        if (len > rowsize)              /* limit to one line at a time */
                len = rowsize;
        if ((len % groupsize) != 0)     /* no mixed size output */
                groupsize = 1;

        switch (groupsize) {
        case 8: {
                const uint64_t *ptr8 = buf;
                int ngroups = len / groupsize;

                for (j = 0; j < ngroups; j++)
#if defined(WIN32) || defined(_WIN32)
                        lx += snprintf(linebuf + lx, linebuflen - lx, "%s%16.16llx", j ? " " : "", *(ptr8 + j));  // used to be "%s%16.16I64x"
#else
                        lx += snprintf(linebuf + lx, linebuflen - lx, "%s%16.16jx", j ? " " : "", *(ptr8 + j)); //(unsigned long long) "%s%16.16llx"
#endif                        

                ascii_column = 17 * ngroups + 2;
                break;
        }

        case 4: {
                const uint32_t *ptr4 = buf;
                int ngroups = len / groupsize;

                for (j = 0; j < ngroups; j++)
                        lx += snprintf(linebuf + lx, linebuflen - lx,
                                        "%s%8.8x", j ? " " : "", *(ptr4 + j));
                ascii_column = 9 * ngroups + 2;
                break;
        }

        case 2: {
                const uint16_t *ptr2 = buf;
                int ngroups = len / groupsize;

                for (j = 0; j < ngroups; j++)
                        lx += snprintf(linebuf + lx, linebuflen - lx,
                                        "%s%4.4x", j ? " " : "", *(ptr2 + j));
                ascii_column = 5 * ngroups + 2;
                break;
        }

        default:
                for (j = 0; (j < len) && (lx + 3) <= linebuflen; j++) {
                        ch = ptr[j];
                        linebuf[lx++] = hex_asc_hi(ch);
                        linebuf[lx++] = hex_asc_lo(ch);
                        linebuf[lx++] = ' ';
                }
                if (j)
                        lx--;

                ascii_column = 3 * rowsize + 2;
                break;
        }
        if (!ascii)
                goto nil;

        while (lx < (linebuflen - 1) && lx < (ascii_column - 1))
                linebuf[lx++] = ' ';
        for (j = 0; (j < len) && (lx + 2) < linebuflen; j++) {
                ch = ptr[j];
                linebuf[lx++] = (isascii(ch) && isprint(ch)) ? ch : '.';
        }
nil:
        linebuf[lx++] = '\0';
}

/**
 * print_hex_dump - print a text hex dump to syslog for a binary blob of data
 * @level: kernel log level (e.g. KERN_DEBUG)
 * @prefix_str: string to prefix each line with;
 *  caller supplies trailing spaces for alignment if desired
 * @rowsize: number of bytes to print per line; must be 16 or 32
 * @groupsize: number of bytes to print at a time (1, 2, 4, 8; default = 1)
 * @buf: data blob to dump
 * @len: number of bytes in the @buf
 * @ascii: include ASCII after the hex output
 *
 * Given a buffer of u8 data, print_hex_dump() prints a hex + ASCII dump
 * to the kernel log at the specified kernel log level, with an optional
 * leading prefix.
 *
 * print_hex_dump() works on one "line" of output at a time, i.e.,
 * 16 or 32 bytes of input data converted to hex + ASCII output.
 * print_hex_dump() iterates over the entire input @buf, breaking it into
 * "line size" chunks to format and print.
 *
 * E.g.:
 *   print_hex_dump(LOG_NOTICE, "data: ", 16, 1, frame->data, frame->len, 0);
 */
void print_hex_dump(log_module_level_t level, const char *prefix_str,
                    int rowsize, int groupsize,
                    const void *buf, int len, int ascii)
{
        const uint8_t *ptr = buf;
        int i, linelen, remaining = len;
        char linebuf[32 * 3 + 2 + 32 + 1];

        if (rowsize != 16 && rowsize != 32)
                rowsize = 16;

        for (i = 0; i < len; i += rowsize)
        {
                linelen = min(remaining, rowsize);
                remaining -= rowsize;

                hex_dump_to_buffer(ptr + i, linelen, rowsize, groupsize,
                                   linebuf, sizeof(linebuf), ascii);

		LOG(lm_main, level, ("%s%s\n", prefix_str, linebuf));
        }
}
