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
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <wchar.h>
#include <locale.h>

#include <charset.h>
#include <utils.h>
#include <logging.h>

#include "scarletbook.h"
#include "scarletbook_helpers.h"
#include "scarletbook_print.h"

// Print to 'wide stdout' a char *vartext (which is converted inside this func before)
// Conversion char *vartext to wide is made inside of this function.
// input text is a simple char * vartext (not wide) !!! 
// format must contains %ls !!!
//
void fwprintf_vartext(const wchar_t *__restrict__ format, const char *text)
{
    wchar_t * wide_text;

    CHAR2WCHAR(wide_text,text);
    fwprintf(stdout, format, wide_text);
  
    free(wide_text);
}
// Print to 'wide stdout' an int and a char *vartext (which is converted inside this func before)
// Conversion char *vartext to wide is made inside of this function.
// input text is a simple char * vartext (not wide) !!! 
// format must contains %d ...%ls !!!
void fwprintf_int_vartext(const wchar_t *__restrict__ format, int i, const char *text)
{
    wchar_t * wide_text;
    
    CHAR2WCHAR(wide_text,text);
    fwprintf(stdout, format, i, wide_text);
      
    free(wide_text);
}


static void scarletbook_print_album_text(scarletbook_handle_t *handle)
{

    master_text_t *master_text = &handle->master_text;

    if (master_text->album_title)
        fwprintf_vartext(L"\tTitle: %ls\n", master_text->album_title);
    if (master_text->album_title_phonetic)
        fwprintf_vartext(L"\tTitle Phonetic: %ls\n", master_text->album_title_phonetic);
    if (master_text->album_artist)
        fwprintf_vartext(L"\tArtist: %ls\n", master_text->album_artist);
    if (master_text->album_artist_phonetic)
        fwprintf_vartext(L"\tArtist Phonetic: %ls\n", master_text->album_artist_phonetic);
    if (master_text->album_publisher)
        fwprintf_vartext(L"\tPublisher: %ls\n", master_text->album_publisher);
    if (master_text->album_publisher_phonetic)
        fwprintf_vartext(L"\tPublisher Phonetic: %ls\n", master_text->album_publisher_phonetic);
    if (master_text->album_copyright)
        fwprintf_vartext(L"\tCopyright: %ls\n", master_text->album_copyright);
    if (master_text->album_copyright_phonetic)
        fwprintf_vartext(L"\tCopyright Phonetic: %ls\n", master_text->album_copyright_phonetic);
}

static void scarletbook_print_disc_text(scarletbook_handle_t *handle)
{

    master_text_t *master_text = &handle->master_text;

    if (master_text->disc_title)
        fwprintf_vartext(L"\tTitle: %ls\n", master_text->disc_title);
    if (master_text->disc_title_phonetic)
        fwprintf_vartext(L"\tTitle Phonetic: %ls\n", master_text->disc_title_phonetic);
    if (master_text->disc_artist)
        fwprintf_vartext(L"\tArtist: %ls\n", master_text->disc_artist);
    if (master_text->disc_artist_phonetic)
        fwprintf_vartext(L"\tArtist Phonetic: %ls\n", master_text->disc_artist_phonetic);
    if (master_text->disc_publisher)
        fwprintf_vartext(L"\tPublisher: %ls\n", master_text->disc_publisher);
    if (master_text->disc_publisher_phonetic)
        fwprintf_vartext(L"\tPublisher Phonetic: %ls\n", master_text->disc_publisher_phonetic);
    if (master_text->disc_copyright)
        fwprintf_vartext(L"\tCopyright: %ls\n", master_text->disc_copyright);
    if (master_text->disc_copyright_phonetic)
        fwprintf_vartext(L"\tCopyright Phonetic: %ls\n", master_text->disc_copyright_phonetic);
}

static void scarletbook_print_master_toc(scarletbook_handle_t *handle)
{
    int          i;
    char         tmp_str[20];
    master_toc_t *mtoc = handle->master_toc;

    fwprintf(stdout, L"\nAlbum Information:\n");
    if (mtoc->album_catalog_number[0] != '\0')
    {
        memset(tmp_str,0,sizeof(tmp_str));
        memcpy(tmp_str, mtoc->album_catalog_number, 16);
        tmp_str[16] = '\0';
        fwprintf_vartext(L"\tAlbum Catalog Number: %ls\n", tmp_str);
    }
    fwprintf(stdout, L"\tSequence Number: %i; Set Size: %i\n", mtoc->album_sequence_number,mtoc->album_set_size);

    for (i = 0; i < 4; i++)
    {
        genre_table_t *t = &mtoc->album_genre[i];
        if (t->category >0 && t->category < MAX_CATEGORY_COUNT)
        {
            fwprintf_vartext(L"\tAlbum Category: %ls\n", album_category[t->category]);
        }
        if (t->genre  >0 && t->genre < MAX_GENRE_COUNT)
        {
            fwprintf_vartext(L"\tAlbum Genre: %ls\n", album_genre[t->genre]);
        }
    }

    scarletbook_print_album_text(handle);

    fwprintf(stdout, L"\nDisc Information:\n");
    fwprintf(stdout, L"\tVersion: %2i.%02i\n", mtoc->version.major, mtoc->version.minor);
    fwprintf(stdout, L"\tDisc type hybrid: %ls\n",mtoc->disc_type_hybrid != 0x0 ? L"yes": L"no");

    if (mtoc->disc_catalog_number[0] != '\0')
    {
        memset(tmp_str,0,sizeof(tmp_str));
        memcpy(tmp_str, mtoc->disc_catalog_number, 16);
        tmp_str[16] = '\0';
        fwprintf_vartext(L"\tDisc Catalog Number: %ls\n", tmp_str);
    }

    for (i = 0; i < 4; i++)
    {
        genre_table_t *t = &mtoc->disc_genre[i];
        if (t->category > 0 && t->category < MAX_CATEGORY_COUNT)
        {
            fwprintf_vartext(L"\tDisc Category: %ls\n", album_category[t->category]);
        }
        if (t->genre >0 && t->genre < MAX_GENRE_COUNT)
        {
            fwprintf_vartext(L"\tDisc Genre: %ls\n", album_genre[t->genre]);
        }
    }

    fwprintf(stdout, L"\tCreation date: %4i-%02i-%02i\n", mtoc->disc_date_year, mtoc->disc_date_month, mtoc->disc_date_day);

    for (i=0;i < MAX_LANGUAGE_COUNT;i++)
    {
        if (mtoc->locales[i].language_code[0] != '\0' && mtoc->locales[i].language_code[1] != '\0')
        {
            memset(tmp_str,0,sizeof(tmp_str));
            memcpy(tmp_str,&mtoc->locales[i].language_code[0], 2);
            fwprintf_vartext(L"\tLocale: %ls; ",tmp_str);

            uint8_t current_charset_nr;
            char *current_charset_name;
            current_charset_nr = mtoc->locales[i].character_set & 0x07;

            if(current_charset_nr < MAX_LANGUAGE_COUNT)
            {
                current_charset_name = (char *)character_set[current_charset_nr];
                fwprintf_int_vartext(L"Code character set:[%d], %ls\n", mtoc->locales[i].character_set, current_charset_name);
            }
            else
            {
                fwprintf_int_vartext(L"Code character set:[%d], %ls\n", mtoc->locales[i].character_set, "unknown");
            }

        }
    }

    scarletbook_print_disc_text(handle);


}

static void scarletbook_print_area_text(scarletbook_handle_t *handle, int area_idx)
{
    int i;

    fwprintf(stdout, L"\tTrack list of area[%d]:\n", area_idx);
    for (i = 0; i < handle->area[area_idx].area_toc->track_count; i++)
    {
        area_track_text_t *track_text = &handle->area[area_idx].area_track_text[i];
        if (track_text->track_type_title)
            fwprintf_int_vartext(L"\t\tTitle[%d]: %ls\n", i+1, track_text->track_type_title);
        if (track_text->track_type_title_phonetic)
            fwprintf_int_vartext(L"\t\tTitle Phonetic[%d]: %ls\n", i+1, track_text->track_type_title_phonetic);
        if (track_text->track_type_performer)
            fwprintf_int_vartext(L"\t\tPerformer[%d]: %ls\n", i+1, track_text->track_type_performer);
        if (track_text->track_type_performer_phonetic)
            fwprintf_int_vartext(L"\t\tPerformer Phonetic[%d]: %ls\n", i+1, track_text->track_type_performer_phonetic);
        if (track_text->track_type_songwriter)
            fwprintf_int_vartext(L"\t\tSongwriter[%d]: %ls\n", i+1, track_text->track_type_songwriter);
        if (track_text->track_type_songwriter_phonetic)
            fwprintf_int_vartext(L"\t\tSongwriter Phonetic[%d]: %ls\n", i+1, track_text->track_type_songwriter_phonetic);
        if (track_text->track_type_composer)
            fwprintf_int_vartext(L"\t\tComposer[%d]: %ls\n", i+1, track_text->track_type_composer);
        if (track_text->track_type_composer_phonetic)
            fwprintf_int_vartext(L"\t\tComposer Phonetic[%d]: %ls\n", i+1, track_text->track_type_composer_phonetic);
        if (track_text->track_type_arranger)
            fwprintf_int_vartext(L"\t\tArranger[%d]: %ls\n", i+1, track_text->track_type_arranger);
        if (track_text->track_type_arranger_phonetic)
            fwprintf_int_vartext(L"\t\tArranger Phonetic[%d]: %ls\n", i+1, track_text->track_type_arranger_phonetic);
        if (track_text->track_type_message)
            fwprintf_int_vartext(L"\t\tMessage[%d]: %ls\n", i+1, track_text->track_type_message);
        if (track_text->track_type_message_phonetic)
            fwprintf_int_vartext(L"\t\tMessage Phonetic[%d]: %ls\n", i+1, track_text->track_type_message_phonetic);
        if (track_text->track_type_extra_message)
            fwprintf_int_vartext(L"\t\tExtra Message[%d]: %ls\n", i+1, track_text->track_type_extra_message);
        if (track_text->track_type_extra_message_phonetic)
            fwprintf_int_vartext(L"\t\tExtra Message Phonetic[%d]: %ls\n", i+1, track_text->track_type_extra_message_phonetic);



        isrc_t *isrc = &handle->area[area_idx].area_isrc_genre->isrc[i];
        char isrc_buf[16];
        if (*isrc->country_code)
        {
            memset(isrc_buf,0,sizeof(isrc_buf));
            memcpy(isrc_buf,isrc,12);
            fwprintf_int_vartext(L"\t\tISRC[%d]: %ls ",i+1, isrc_buf);

            memset(isrc_buf,0,sizeof(isrc_buf));
            memcpy(isrc_buf,isrc->country_code,2);
            fwprintf_vartext(L"(country:%ls, ", isrc_buf);
            memset(isrc_buf,0,sizeof(isrc_buf));
            memcpy(isrc_buf,isrc->owner_code,3);            
            fwprintf_vartext(L"owner:%ls, ", isrc_buf);
            memset(isrc_buf,0,sizeof(isrc_buf));
            memcpy(isrc_buf,isrc->recording_year,2);           
            fwprintf_vartext(L"year:%ls, ", isrc_buf);
            memset(isrc_buf,0,sizeof(isrc_buf));
            memcpy(isrc_buf,isrc->designation_code,5);            
            fwprintf_vartext(L"designation:%ls)\n", isrc_buf);
        }

        area_tracklist_time_t time_start = handle->area[area_idx].area_tracklist_time->start[i];
        area_tracklist_time_t time_duration = handle->area[area_idx].area_tracklist_time->duration[i];
        fwprintf(stdout, L"\t\tTrack_Start_Time_Code: %02d:%02d:%02d [mins:secs:frames]\n", time_start.minutes, time_start.seconds, time_start.frames);
        fwprintf(stdout, L"\t\tDuration: %02d:%02d:%02d [mins:secs:frames]\n", time_duration.minutes, time_duration.seconds, time_duration.frames);
        fwprintf(stdout, L"\n");
    }   
}

static void scarletbook_print_area_toc(scarletbook_handle_t *handle, int area_idx)
{

    scarletbook_area_t      *area = &handle->area[area_idx];
    area_toc_t              *area_toc = area->area_toc;


    fwprintf(stdout, L"\tArea Information [%i]:\n", area_idx);
    fwprintf(stdout, L"\tVersion: %2i.%02i\n", area_toc->version.major, area_toc->version.minor);

    if (area->copyright)
        fwprintf_vartext(L"\tCopyright: %ls\n", area->copyright);
    if (area->copyright_phonetic)
        fwprintf_vartext(L"\tCopyright Phonetic: %ls\n", area->copyright_phonetic);
    if (area->description)
        fwprintf_vartext(L"\tArea Description: %ls\n", area->description);
    if (area->description_phonetic)
        fwprintf_vartext(L"\tArea Description Phonetic: %ls\n", area->description_phonetic);

    fwprintf(stdout, L"\tTrack Count: %i\n", area_toc->track_count);
    fwprintf(stdout, L"\tTotal play time: %02d:%02d:%02d [mins:secs:frames]\n", area_toc->total_playtime.minutes, area_toc->total_playtime.seconds, area_toc->total_playtime.frames);

    fwprintf_int_vartext(L"\tFrame format encoding: [%02d] [%ls]\n",area_toc->frame_format,get_frame_format_string(area_toc));

    fwprintf(stdout, L"\tSpeaker config: ");
    if (area_toc->channel_count == 2 && area_toc->extra_settings == 0)
    {
        fwprintf(stdout, L"2 Channel\n");
    }
    else if (area_toc->channel_count == 5 && area_toc->extra_settings == 3)
    {
        fwprintf(stdout, L"5 Channel\n");
    }
    else if (area_toc->channel_count == 6 && area_toc->extra_settings == 4)
    {
        fwprintf(stdout, L"6 Channel\n");
    }
    else
    {
        fwprintf(stdout, L"Unknown\n");
    }

    int i;
    char tmp_str[20];
    for (i=0;i < MAX_LANGUAGE_COUNT;i++)
    {
        if (area_toc->languages[i].language_code[0] != '\0' && area_toc->languages[i].language_code[1] != '\0')
        {
            memset(tmp_str,0,sizeof(tmp_str));
            memcpy(tmp_str,&area_toc->languages[i].language_code[0], 2);
            fwprintf_vartext(L"\tLocale: %ls; ",tmp_str);

            uint8_t current_charset_nr;
            char *current_charset_name;
            current_charset_nr = area_toc->languages[i].character_set & 0x07;

            if(current_charset_nr < MAX_LANGUAGE_COUNT)
            {
                current_charset_name = (char *)character_set[current_charset_nr];
                fwprintf_int_vartext(L"Code character set:[%d], %ls\n", area_toc->languages[i].character_set, current_charset_name);
            }
            else
            {
                fwprintf_int_vartext(L"Code character set:[%d], %ls\n", area_toc->languages[i].character_set, "unknown");
            }

        }
    }

    scarletbook_print_area_text(handle, area_idx);

}

void scarletbook_print(scarletbook_handle_t *handle)
{
    int i;

    if (!handle)
    {
        fprintf(stderr, "No valid ScarletBook handle available\n");
        LOG(lm_main, LOG_ERROR, ("ERROR in scarletbook_print...No valid ScarletBook handle available!"));
        return;
    }

    LOG(lm_main, LOG_NOTICE, ("NOTICE in scarletbook_print...start scarltebook_print_master_toc()"));
    if (handle->master_toc)
    {
        scarletbook_print_master_toc(handle);
    }

    LOG(lm_main, LOG_NOTICE, ("NOTICE in scarletbook_print...start scarletbook_print_area_toc()"));
    fwprintf(stdout, L"\nArea count: %i\n", handle->area_count);
    if (handle->area_count > 0)
    {
        for (i = 0; i < handle->area_count; i++)
        {
            scarletbook_print_area_toc(handle, i);
        }
    }
}
