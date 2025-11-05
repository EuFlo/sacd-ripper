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

#include <string.h>
#include <stdlib.h>
#include <fileutils.h>
#include <utils.h>
#include <logging.h>

#include "scarletbook_helpers.h"

//  Copy UTF-8 encoding chars
//   *dst  - destination string
//   *src   - source string
//    n - number of chars
//
int utf8cpy(char *dst, char *src, int n)
{
    // n is the size of dst (including the last byte for null
    int i = 0;

    while (i < n)
    {
        int c;
        if (!(src[i] & 0x80))
        {
            // ASCII code
            if (src[i] == '\0')
            {
                break;
            }
            c = 1;
        }
        else if ((src[i] & 0xe0) == 0xc0)
        {
            // 2-byte code
            c = 2;
        }
        else if ((src[i] & 0xf0) == 0xe0)
        {
            // 3-byte code
            c = 3;
        }
        else if ((src[i] & 0xf8) == 0xf0)
        {
            // 4-byte code
            c = 4;
        }
        else if (src[i] == '\0')
        {
            break;
        }
        else
        {
            break;
        }
        
        if (i + c <= n)
        {
            memcpy(dst + i, src + i, c * sizeof(char));
            i += c;
        }
        else
        {
            break;
        }
    }

    dst[i] = '\0';
    return i;
}

// #define MAX_DISC_ARTIST_LEN 40  ==> moved in scarletbook_helpers.h
// #define MAX_ALBUM_TITLE_LEN 100
// #define MAX_TRACK_TITLE_LEN 120
// #define MAX_TRACK_ARTIST_LEN 40

//  Generates a name for a directory or filename from disc/album artist and album/disc title
//  Useful for iso, cue, xml, DFF edit master files
//    if album is multiset then adds
//         ' (disc number-number_of_total discs)'
//    if artist_flag >0 then adds disc/album artist to the name of directory
//       'artist - title'
//      or
//       'artist - title (disc number-number_of_total discs)'
//   NOTE: caller must free the returned string!

char *get_album_dir(scarletbook_handle_t *handle, int artist_flag)
{
    char disc_artist[MAX_DISC_ARTIST_LEN + 1];
    char disc_album_title[MAX_ALBUM_TITLE_LEN + 1];
    char disc_album_year[20];
    char *albumdir = NULL;
    master_text_t *master_text = &handle->master_text;
    char *p_artist = NULL;
    char *p_album_title = NULL;

    if (master_text->disc_artist)
        p_artist = master_text->disc_artist;
    else if (master_text->disc_artist_phonetic)
        p_artist = master_text->disc_artist_phonetic;
    else if (master_text->album_artist)
        p_artist = master_text->album_artist;
    else if (master_text->album_artist_phonetic)
        p_artist = master_text->album_artist_phonetic;


    if (handle->master_toc->album_set_size > 1) // If there is a set of discs
    {
        // in case of multiset, album_title  must be used first, instead of disc_title
        if (master_text->album_title)
            p_album_title = master_text->album_title;
        else if (master_text->album_title_phonetic)
            p_album_title = master_text->album_title_phonetic;
        else if (master_text->disc_title)
            p_album_title = master_text->disc_title;
        else if (master_text->disc_title)
            p_album_title = master_text->disc_title_phonetic;
    }
    else
    {
        if (master_text->disc_title)
            p_album_title = master_text->disc_title;
        else if (master_text->disc_title_phonetic)
            p_album_title = master_text->disc_title_phonetic;
        else if (master_text->album_title)
            p_album_title = master_text->album_title;
        else if (master_text->album_title_phonetic)
            p_album_title = master_text->album_title_phonetic;
    }


    memset(disc_artist, 0, sizeof(disc_artist));
    if (p_artist)
    {
        char *pos, *pos_end;

        pos_end = p_artist + strlen(p_artist);

        pos = strchr(p_artist, ';');
        if (pos != NULL && pos < pos_end)
            pos_end = pos;       
        pos = strchr(p_artist, '/'); // standard artist separator
        if (pos != NULL && pos < pos_end)
            pos_end = pos;
        pos = strchr(p_artist, ',');
        if (pos != NULL && pos < pos_end)
            pos_end = pos;
        // pos = strchr(p_artist, '.'); 
        //if (pos != NULL && pos < pos_end)
        //    pos_end = pos;
        pos = strstr(p_artist, " -");
        if (pos != NULL && pos < pos_end)
            pos_end = pos;
        
        strncpy(disc_artist, p_artist, min(pos_end - p_artist, MAX_DISC_ARTIST_LEN));
        
        sanitize_filename(disc_artist);
    }

    memset(disc_album_title, 0, sizeof(disc_album_title));
    if (p_album_title)
    {
        char *pos_end = strchr(p_album_title, ';');

        if (pos_end != NULL)
            strncpy(disc_album_title, p_album_title, min(pos_end - p_album_title, MAX_ALBUM_TITLE_LEN));
        else
            strncpy(disc_album_title, p_album_title, MAX_ALBUM_TITLE_LEN);

        sanitize_filename(disc_album_title);
    }

    char multiset_s[32] = "";
    char *disc_album_title_final;

    if (handle->master_toc->album_set_size > 1) // Set of discs
    {       
        snprintf(multiset_s, sizeof(multiset_s), " (disc %d-%d)", handle->master_toc->album_sequence_number, handle->master_toc->album_set_size);
        disc_album_title_final = (char *)calloc( strlen(disc_album_title) + strlen(multiset_s) + 1,sizeof(char));
        sprintf(disc_album_title_final, "%s%s", disc_album_title, multiset_s);
    }
    else
    {
        disc_album_title_final = (char *)calloc(strlen(disc_album_title) + 1,sizeof(char));
        sprintf(disc_album_title_final, "%s", disc_album_title);
    }


    snprintf(disc_album_year, sizeof(disc_album_year), "%04u", handle->master_toc->disc_date_year);

    LOG(lm_main, LOG_NOTICE, ("NOTICE in scarletbook_helpers..get_album_dir(), strlen(disc_album_title)=[%d]; strlen(disc_artist)=[%d]",strlen(disc_album_title), strlen(disc_artist)));

    if (strlen(disc_artist) > 0 && strlen(disc_album_title) > 0 && artist_flag !=0 )
        albumdir = parse_format("%A - %L", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);
    else if (strlen(disc_artist) > 0 && artist_flag != 0)
        albumdir = parse_format("%A", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);
    else if (strlen(disc_album_title) > 0)
        albumdir = parse_format("%L", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);
    else
        albumdir = parse_format("empty album title", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);


    free(disc_album_title_final);
    
    return albumdir;
}

//  Generates a path from disc title/album title.
//  Useful for dsf, dff files.
//  the only difference from get_album_dir is:
//    if album has multiple discs then
//       inserts a new component into path \Disc 1...N
// if artist_flag=1 then adds artist to the path like this:
//    artist - title
//    artist - title\Disc 1...N
//  NOTE: caller must free the returned string!
char *get_path_disc_album(scarletbook_handle_t *handle, int artist_flag)
{
    //char disc_title[MAX_ALBUM_TITLE_LEN + 4];
    char disc_album_title[MAX_ALBUM_TITLE_LEN + 4];
    char disc_artist[MAX_DISC_ARTIST_LEN + 4];

    master_text_t *master_text = &handle->master_text;
     
    //char *p_disc_title = NULL;
    char *p_album_title = NULL;
    char *p_artist = NULL;
    char *disc_album_title_final=NULL;
    

    if (handle->master_toc->album_set_size > 1) // If there is a set of discs
    {
        // in case of multiset, album_title  must be used first, instead of disc_title
        if (master_text->album_title)
            p_album_title = master_text->album_title;
        else if (master_text->album_title_phonetic)
            p_album_title = master_text->album_title_phonetic;
        else if (master_text->disc_title)
            p_album_title = master_text->disc_title;
        else if (master_text->disc_title)
            p_album_title = master_text->disc_title_phonetic;
    }
    else
    {
        if (master_text->disc_title)
            p_album_title = master_text->disc_title;
        else if (master_text->disc_title_phonetic)
            p_album_title = master_text->disc_title_phonetic;
        else if (master_text->album_title)
            p_album_title = master_text->album_title;
        else if (master_text->album_title_phonetic)
            p_album_title = master_text->album_title_phonetic;
    }

    if (master_text->disc_artist)
        p_artist = master_text->disc_artist;
    else if (master_text->disc_artist_phonetic)
        p_artist = master_text->disc_artist_phonetic;
    else if (master_text->album_artist)
        p_artist = master_text->album_artist;
    else if (master_text->album_artist_phonetic)
        p_artist = master_text->album_artist_phonetic;


    memset(disc_album_title, 0,sizeof(disc_album_title));
    if (p_album_title)
    {
        strncpy(disc_album_title, p_album_title, MAX_ALBUM_TITLE_LEN);
        sanitize_filename(disc_album_title);
    }
    if(strlen(disc_album_title)==0)
    {
        strncpy(disc_album_title, "empty album title", MAX_ALBUM_TITLE_LEN);
    }
    LOG(lm_main, LOG_NOTICE, ("NOTICE in scarletbook_helpers..get_path_disc_album(), p_album_title=[%X]; disc_album_title=[%s]",p_album_title, disc_album_title));

    memset(disc_artist, 0, sizeof(disc_artist));
    if (p_artist)
    {
        char *pos, *pos_end;
        
        pos_end = p_artist + strlen(p_artist);

        pos = strchr(p_artist, ';');
        if (pos != NULL && pos < pos_end)
            pos_end = pos;       
        pos = strchr(p_artist, '/'); // standard artist separator
        if (pos != NULL && pos < pos_end)
            pos_end = pos;
        pos = strchr(p_artist, ',');
        if (pos != NULL && pos < pos_end)
            pos_end = pos;
        // pos = strchr(p_artist, '.'); 
        //if (pos != NULL && pos < pos_end)
        //    pos_end = pos;
        pos = strstr(p_artist, " -");
        if (pos != NULL && pos < pos_end)
            pos_end = pos;

        //LOG(lm_main, LOG_NOTICE, ("NOTICE in scarletbook_helpers..get_path_disc_album(), p_artist=[%X]; pos_end=[%X]",p_artist, pos_end));
        strncpy(disc_artist, p_artist, min(pos_end - p_artist, MAX_DISC_ARTIST_LEN));
        sanitize_filename(disc_artist);
        LOG(lm_main, LOG_NOTICE, ("NOTICE in scarletbook_helpers..get_path_disc_album(), disc_artist=[%s]",disc_artist));
    }

    //LOG(lm_main, LOG_NOTICE, ("NOTICE in scarletbook_helpers..get_path_disc_album(), disc_album_title=[%s]; disc_artist=[%s]",disc_album_title, disc_artist));

    if (handle->master_toc->album_set_size > 1) // If there is a set of discs
    {
        char multiset_s[40];

        memset(multiset_s, 0, sizeof(multiset_s));
        //snprintf(multiset_s, sizeof(multiset_s), "(disc %d of %d)", handle->master_toc->album_sequence_number, handle->master_toc->album_set_size);
        snprintf(multiset_s, sizeof(multiset_s), "Disc %d", handle->master_toc->album_sequence_number);

        // if (strcmp(disc_title, disc_album_title) != 0) // if disc title differ from album title. Must add a subfolder with disc title
        // {
        //     disc_album_title_final = (char *)calloc(strlen(disc_album_title) + 1 + strlen(disc_title) + strlen(multiset_s) + 1, sizeof(char));
        //     strcpy(disc_album_title_final, disc_album_title);
        //     strcat(disc_album_title_final, "/");
        //     strcat(disc_album_title_final, disc_title);
        // }
        // else
        // {
        if (artist_flag !=0  && strlen(disc_artist) > 0) // add artist name
        {
            disc_album_title_final = (char *)calloc(strlen(disc_artist) + 3 + strlen(disc_album_title) + 1 + strlen(multiset_s) + 1, sizeof(char));
            strcpy(disc_album_title_final, disc_artist);
            strcat(disc_album_title_final, " - ");
            strcat(disc_album_title_final, disc_album_title);
        }
        else 
        {
            disc_album_title_final = (char *)calloc(strlen(disc_album_title) + 1 + strlen(multiset_s) + 1, sizeof(char));
            strcat(disc_album_title_final, disc_album_title);
        }

#if defined(WIN32) || defined(_WIN32)
            strcat(disc_album_title_final, "\\");
#else
            strcat(disc_album_title_final, "/");
#endif
            //}

            strcat(disc_album_title_final, multiset_s);
    }
    else   // not album set
    {
        // if (strcmp(disc_title, disc_album_title) != 0) // if disc title differ from album title. Must add a subfolder with disc title
        // {
        //     disc_album_title_final = (char *)calloc(strlen(disc_album_title) + 1 + strlen(disc_title)  + 1, sizeof(char));
        //     strcpy(disc_album_title_final, disc_album_title);
        //     strcat(disc_album_title_final, "/");
        //     strcat(disc_album_title_final, disc_title);
        // }
        // else
        // {

 

        size_t total_size = strlen(disc_artist) + 3 + strlen(disc_album_title) + 1;
        LOG(lm_main, LOG_NOTICE, ("NOTICE in scarletbook_helpers..get_path_disc_album(), total size=[%d]",total_size));
        LOG(lm_main, LOG_NOTICE, ("NOTICE in scarletbook_helpers..get_path_disc_album(), strlen(disc_artist)=[%d]; strlen(disc_album_title)=[%d]",strlen(disc_artist),strlen(disc_album_title)));


        if (artist_flag !=0 && strlen(disc_artist) > 0) // add artist name
        {
            disc_album_title_final = (char *)calloc(strlen(disc_artist) + 3 + strlen(disc_album_title) + 1, sizeof(char));
            strcpy(disc_album_title_final, disc_artist);
            strcat(disc_album_title_final, " - ");
            strcat(disc_album_title_final, disc_album_title);
        }
        else{
            disc_album_title_final = (char *)calloc(strlen(disc_album_title) + 1, sizeof(char));
            strcat(disc_album_title_final, disc_album_title);
        }
        
                    
        //}
        
    }

    //sanitize_filepath(disc_album_title_final);
    
    return disc_album_title_final;
}

char *get_music_filename(scarletbook_handle_t *handle, int area, int track, const char *override_title, int performer_flag)
{
    char *c =NULL;
    char track_artist[MAX_TRACK_ARTIST_LEN + 1];
    char track_title[MAX_TRACK_TITLE_LEN + 1];
    char disc_album_title[MAX_ALBUM_TITLE_LEN + 1];
    char disc_album_year[20];
    master_text_t *master_text = &handle->master_text;
    char * p_album_title = NULL;
    int performer_flag_local=performer_flag;

    memset(track_artist, 0, sizeof(track_artist));
    if( handle->area[area].area_track_text[track].track_type_performer)
    {
        c = handle->area[area].area_track_text[track].track_type_performer;
    }
    else
    {
        if (master_text->disc_artist)
        {
            c = master_text->disc_artist;           
        }
        else if (master_text->album_artist)
        {
            c =  master_text->album_artist;           
        }
        else
        {
            // nothing found to put in performer. Do not insert it all
            //strncpy(track_artist, "unknown performer", min(strlen("unknown performer"), MAX_TRACK_ARTIST_LEN));
            performer_flag_local = 0;
        }       
        
    }
    

    if (c)
    {
        char *pos, *pos_end;

        pos_end = c + strlen(c);

        pos = strchr(c, ';');
        if (pos != NULL && pos < pos_end)
            pos_end = pos;
        pos = strchr(c, '/'); // standard artist separator
        if (pos != NULL && pos < pos_end)
            pos_end = pos;
        pos = strchr(c, ',');
        if (pos != NULL && pos < pos_end)
            pos_end = pos;
        // pos = strchr(c, '.'); 
        //if (pos != NULL && pos < pos_end)
        //    pos_end = pos;
        pos = strstr(c, " -");
        if (pos != NULL && pos < pos_end)
            pos_end = pos;
		
        strncpy(track_artist, c, min(pos_end - c, MAX_TRACK_ARTIST_LEN));
        sanitize_filename(track_artist);
    }
    

    memset(track_title, 0, sizeof(track_title));
    c = handle->area[area].area_track_text[track].track_type_title;
    if (c)
    {
        strncpy(track_title, c, MAX_TRACK_TITLE_LEN);
        sanitize_filename(track_title);
    }
    
    if (master_text->disc_title)
        p_album_title = master_text->disc_title;
    else if (master_text->disc_title_phonetic)
        p_album_title = master_text->disc_title_phonetic;
    else if (master_text->album_title)
        p_album_title = master_text->album_title;
    else if (master_text->album_title_phonetic)
        p_album_title = master_text->album_title_phonetic;

    memset(disc_album_title, 0, sizeof(disc_album_title));
    if (p_album_title)
    {
        char *pos_end = strchr(p_album_title, ';');
        if (pos_end != NULL)   
            strncpy(disc_album_title, p_album_title, min(pos_end - p_album_title, MAX_ALBUM_TITLE_LEN));
        else
            strncpy(disc_album_title, p_album_title, MAX_ALBUM_TITLE_LEN);

        sanitize_filename(disc_album_title);
    }

    if(strlen(disc_album_title)==0)
    {
         strncpy(disc_album_title, "empty album title",  MAX_ALBUM_TITLE_LEN);
    }
    
    snprintf(disc_album_year, sizeof(disc_album_year), "%04u", handle->master_toc->disc_date_year);

    if (override_title && strlen(override_title) > 0)
        {
            if(performer_flag_local ==0)
                return parse_format("%N - %L -%T", track + 1, disc_album_year, track_artist, disc_album_title, override_title);
            else
                return parse_format("%N - %L - %A -%T", track + 1, disc_album_year, track_artist, disc_album_title, override_title);
        }
    else if (strlen(track_artist) > 0 && strlen(track_title) > 0 && performer_flag_local != 0) 
        return parse_format("%N - %A - %T", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(track_artist) > 0 && performer_flag_local != 0)  
        return parse_format("%N - %A", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(track_title) > 0)
        return parse_format("%N - %T", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else  // as a last resort, using album/disc title
        return parse_format("%N - %L", track + 1, disc_album_year, track_artist, disc_album_title, track_title);

}


char *get_speaker_config_string(area_toc_t *area) 
{
    if (area->channel_count == 2 && area->extra_settings == 0)
    {
        return "Stereo";
    }
    else if (area->channel_count == 5 && area->extra_settings == 3)
    {
        return "5ch";
    }
    else if (area->channel_count == 6 && area->extra_settings == 4)
    {
        return "6ch"; //"5.1ch";
    }
    else
    {
        return "Unkn-ch";
    }
}

char *get_frame_format_string(area_toc_t *area) 
{
    if (area->frame_format == FRAME_FORMAT_DSD_3_IN_14)
    {
        return "DSD 3 in 14";
    }
    else if (area->frame_format == FRAME_FORMAT_DSD_3_IN_16)
    {
        return "DSD 3 in 16";
    }
    else if (area->frame_format == FRAME_FORMAT_DST)
    {
        return "Lossless DST";
    }
    else
    {
        return "Unknown";
    }
}
