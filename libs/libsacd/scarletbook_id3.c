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

#include <charset.h>

#include "scarletbook.h"
#include <logging.h>

#include "id3.h"
#include <genre.dat>

int scarletbook_id3_tag_render(scarletbook_handle_t *handle, uint8_t *buffer, int area, int track)
{
    const int sacd_id3_genres[] = {
        12,     /* Not used => Other */
        12,     /* Not defined => Other */
        60,     /* Adult Contemporary => Top 40 */
        40,     /* Alternative Rock => AlternRock */
        12,     /* Children's Music => Other */
        32,     /* Classical => Classical */
        140,    /* Contemporary Christian => Contemporary Christian */
        2,      /* Country => Country */
        3,      /* Dance => Dance */
        98,     /* Easy Listening => Easy Listening */
        109,    /* Erotic => Porn Groove */
        80,     /* Folk => Folk */
        38,     /* Gospel => Gospel */
        7,      /* Hip Hop => Hip-Hop */
        8,      /* Jazz => Jazz */
        86,     /* Latin => Latin */
        77,     /* Musical => Musical */
        10,     /* New Age => New Age */
        103,    /* Opera => Opera */
        104,    /* Operetta => Chamber Music */
        13,     /* Pop Music => Pop */
        15,     /* RAP => Rap */
        16,     /* Reggae => Reggae */
        17,     /* Rock Music => Rock */
        14,     /* Rhythm & Blues => R&B */
        37,     /* Sound Effects => Sound Clip */
        24,     /* Sound Track => Soundtrack */
        101,    /* Spoken Word => Speech */
        48,     /* World Music => Ethnic */
        0,      /* Blues => Blues */
        12,     /* Not used => Other */
    }; 
    struct id3_tag *tag;
    struct id3_frame *frame;
    char tmp[200];
    int len;

    tag = id3_open_mem(0, ID3_OPENF_CREATE);

    if (handle->id3_tag_mode == 4 || handle->id3_tag_mode == 5)
        tag->id3_version = 4;

    memset(tmp, 0, sizeof(tmp));

    // TIT2 track title
    if (handle->area[area].area_track_text[track].track_type_title)
    {
        frame = id3_add_frame(tag, ID3_TIT2);       
        id3_set_text_wraper(frame, handle->area[area].area_track_text[track].track_type_title, handle->id3_tag_mode);
    }
    else
    {
        if(handle->id3_tag_mode !=2) // not minimal
        {
            master_text_t *master_text = &handle->master_text;
            char *album_title = 0;

            if (master_text->album_title)
                album_title = master_text->album_title;
            else if (master_text->album_title_phonetic)
                album_title = master_text->album_title_phonetic;
            else if (master_text->disc_title)
                album_title = master_text->disc_title;
            else if (master_text->disc_title_phonetic)
                album_title = master_text->disc_title_phonetic;

            if (album_title)
            {
                frame = id3_add_frame(tag, ID3_TIT2);             
                id3_set_text_wraper(frame, album_title, handle->id3_tag_mode);
            }
        }
    }
    // Title of album
    {
        master_text_t *master_text = &handle->master_text;
        char *album_title = 0;

        if (master_text->album_title)
            album_title = master_text->album_title;
        else if (master_text->album_title_phonetic)
            album_title = master_text->album_title_phonetic;
        else if (master_text->disc_title)
            album_title = master_text->disc_title;
        else if (master_text->disc_title_phonetic)
            album_title = master_text->disc_title_phonetic;

        if (album_title)
        {
            frame = id3_add_frame(tag, ID3_TALB);           
            id3_set_text_wraper(frame, album_title, handle->id3_tag_mode); 
        }
    }
    // Track Artists (Artist name /performer)
    if (handle->area[area].area_track_text[track].track_type_performer)
    {
        char *performer = handle->area[area].area_track_text[track].track_type_performer;
        

        frame = id3_add_frame(tag, ID3_TPE1); // Artist, soloist
                                              //id3_set_text(frame, performer);       //TPE1 = The 'Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group' is used for the main artist(s). They are seperated with the "/" character
       
        id3_set_text_wraper(frame, performer, handle->id3_tag_mode);


        //frame = id3_add_frame(tag, ID3_TPE3);  // TPE3=Conductor/performer refinement;  TOPE='Original artist(s)/performer(s)' IPLS -Involved people(performer?)
        //id3_set_text(frame, performer);
        //frame = id3_add_frame(tag, ID3_IPLS);  // IPLS -Involved people(performer?)
        //id3_set_text(frame, performer);
       
    }
    else
    {
        master_text_t *master_text = &handle->master_text;
        char *artist = 0;
       
        if (master_text->disc_artist)
            artist = master_text->disc_artist;
        else if (master_text->disc_artist_phonetic)
            artist = master_text->disc_artist_phonetic;
        else if (master_text->album_artist)
            artist = master_text->album_artist;
        else if (master_text->album_artist_phonetic)
            artist = master_text->album_artist_phonetic;
       
        if (artist)
        {
            frame = id3_add_frame(tag, ID3_TPE1);           
            id3_set_text_wraper(frame, artist, handle->id3_tag_mode);
        }
    }

    if (handle->id3_tag_mode != 2 && handle->id3_tag_mode != 5) // not minimal
    {

        // TPE2 is widely used as album artist
        if (handle->master_text.album_artist)
        {
            char *album_artist = handle->master_text.album_artist;
            frame = id3_add_frame(tag, ID3_TPE2); // TPE2: The 'Band/Orchestra/Accompaniment' frame is used for additional information about the performers in the recording

            id3_set_text_wraper(frame, album_artist, handle->id3_tag_mode);
        }

        // ID3_TXXX:Performer
        if (handle->area[area].area_track_text[track].track_type_performer)
        {
            char *performer = handle->area[area].area_track_text[track].track_type_performer;
            frame = id3_add_frame(tag, ID3_TXXX); // ID3_TXXX, Performer
            
            id3_set_text__performer_wraper(frame, performer, handle->id3_tag_mode);

        }

        // Composer
        if (handle->area[area].area_track_text[track].track_type_composer)
        {
            char *composer = handle->area[area].area_track_text[track].track_type_composer;
            frame = id3_add_frame(tag, ID3_TCOM);

            id3_set_text_wraper(frame, composer, handle->id3_tag_mode);           
        }

        // ISCR
        isrc_t isrc_st = handle->area[area].area_isrc_genre->isrc[track];
        if ((isrc_st.country_code[0] != 0x0) || (isrc_st.owner_code[0] != 0x0) || (isrc_st.designation_code[0] !=0x0) || (isrc_st.recording_year[0] != 0x0) )
        {
            char isrc[16];
            
            memcpy(isrc, handle->area[area].area_isrc_genre->isrc[track].country_code, 2);
            memcpy(isrc + 2, handle->area[area].area_isrc_genre->isrc[track].owner_code, 3);
            memcpy(isrc + 5, handle->area[area].area_isrc_genre->isrc[track].recording_year, 2);
            memcpy(isrc + 7, handle->area[area].area_isrc_genre->isrc[track].designation_code, 5);
            isrc[12] = 0x00;

            frame = id3_add_frame(tag, ID3_TSRC);

            id3_set_text_wraper(frame, isrc, handle->id3_tag_mode);
            
            // DEBUG
            // char isrc_str[100];
           
            // LOG(lm_main, LOG_NOTICE, ("ISRC Track=%d; id3_add_frame(.., ID3_TSRC), id3_set_text_wraper()",track));
            // LOG(lm_main, LOG_NOTICE, ("Country: %c%c",isrc_st.country_code[0],isrc_st.country_code[1] ));
            // LOG(lm_main, LOG_NOTICE, ("Owner: %c%c%c ",isrc_st.owner_code[0],isrc_st.owner_code[1],isrc_st.owner_code[2] ));
            // LOG(lm_main, LOG_NOTICE, ("Year: %c%c",isrc_st.recording_year[0],isrc_st.recording_year[1] ));
            // snprintf(isrc_str,100, "Designation: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X \n", isrc_st.designation_code[0],isrc_st.designation_code[1],isrc_st.designation_code[2],isrc_st.designation_code[3],isrc_st.designation_code[4]);
            // LOG(lm_main, LOG_NOTICE, ("%s",isrc_str ));
            
        }
        // Publisher
        if (handle->master_text.album_publisher)
        {
            char *publisher = handle->master_text.album_publisher;
            frame = id3_add_frame(tag, ID3_TPUB);
                       
            id3_set_text_wraper(frame, publisher, handle->id3_tag_mode);            
        }

        // Copyright
        if (handle->master_text.album_copyright)
        {
            char *copyright = handle->master_text.album_copyright;
            frame = id3_add_frame(tag, ID3_TCOP);         
           
            id3_set_text_wraper(frame, copyright,handle->id3_tag_mode);            
        }

        // Part of set. Disc sequence/set size
        if (handle->master_toc)
        {
            master_toc_t *mtoc = handle->master_toc;
            char str[64];
            
            sprintf(str, "%d/%d", mtoc->album_sequence_number, mtoc->album_set_size);
            frame = id3_add_frame(tag, ID3_TPOS);                    
            id3_set_text_wraper(frame, str, handle->id3_tag_mode);
        }

        // Genre
        frame = id3_add_frame(tag, ID3_TCON);       
        id3_set_text_wraper(frame, (char *)genre_table[sacd_id3_genres[handle->area[area].area_isrc_genre->track_genre[track].genre & 0x1f]], handle->id3_tag_mode);
        
        //  Version 2.3 –> version 2.4
        //TYER, TDAT, TIME –> TDRC

        if(tag->id3_version == 4) // ID3 v2.4
        {
            snprintf(tmp, 200, "%04d-%02d-%02d", handle->master_toc->disc_date_year,handle->master_toc->disc_date_month, handle->master_toc->disc_date_day);
            frame = id3_add_frame(tag, ID3_TDRC);
            id3_set_text(frame, tmp);
        }
        else  // ID3 v2.3
        {  
            // YEAR
            //ID3 v2.3 Date Frames
            // TYER year (recording year of form YYYY, always 4 bytes)
            snprintf(tmp, 200, "%04d", handle->master_toc->disc_date_year);
            frame = id3_add_frame(tag, ID3_TYER);
            id3_set_text(frame, tmp);

            // Month, day
            //ID3 v2.3 Date Frames
            //TDAT date (recording date of form DDMM, always 4 bytes)
            snprintf(tmp, 200, "%02d%02d", handle->master_toc->disc_date_day,handle->master_toc->disc_date_month);
            frame = id3_add_frame(tag, ID3_TDAT);
            id3_set_text(frame, tmp);
        }

    } // end if not minimal

    // Track number/total tracks
    snprintf(tmp, 200, "%d/%d", track + 1, handle->area[area].area_toc->track_count); // internally tracks start from 0
    frame = id3_add_frame(tag, ID3_TRCK);
    id3_set_text_wraper(frame, tmp, handle->id3_tag_mode);

    len = id3_write_tag(tag, buffer);
    id3_close(tag);

    return len;
}
