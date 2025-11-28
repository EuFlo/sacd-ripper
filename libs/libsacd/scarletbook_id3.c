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
//#include "genre.dat"

#include "id3.h"
#include "utils.h"
#include "lang_code_table.dat"


int scarletbook_id3_tag_render(scarletbook_handle_t *handle, uint8_t *buffer, int area, int track)
{
    // const int sacd_id3_genres[] = {
    //     12,     /* Not used => Other */
    //     12,     /* Not defined => Other */
    //     60,     /* Adult Contemporary => Top 40 */
    //     40,     /* Alternative Rock => AlternRock */
    //     12,     /* Children's Music => Other */
    //     32,     /* Classical => Classical */
    //     140,    /* Contemporary Christian => Contemporary Christian */
    //     2,      /* Country => Country */
    //     3,      /* Dance => Dance */
    //     98,     /* Easy Listening => Easy Listening */
    //     109,    /* Erotic => Porn Groove */
    //     80,     /* Folk => Folk */
    //     38,     /* Gospel => Gospel */
    //     7,      /* Hip Hop => Hip-Hop */
    //     8,      /* Jazz => Jazz */
    //     86,     /* Latin => Latin */
    //     77,     /* Musical => Musical */
    //     10,     /* New Age => New Age */
    //     103,    /* Opera => Opera */
    //     104,    /* Operetta => Chamber Music */
    //     13,     /* Pop Music => Pop */
    //     15,     /* RAP => Rap */
    //     16,     /* Reggae => Reggae */
    //     17,     /* Rock Music => Rock */
    //     14,     /* Rhythm & Blues => R&B */
    //     37,     /* Sound Effects => Sound Clip */
    //     24,     /* Sound Track => Soundtrack */
    //     101,    /* Spoken Word => Speech */
    //     48,     /* World Music => Ethnic */
    //     0,      /* Blues => Blues */
    //     12,     /* Not used => Other */
    // }; 
    struct id3_tag *tag;
    struct id3_frame *frame;
    char tmp[200];
    int len = 0;
    master_text_t *master_text = &handle->master_text;
    char *album_title = NULL;
    char *disc_title = NULL;
    char cat_str_d[20];

    if (handle->id3_tag_mode == 0)
    {
        return len;
    }

    master_toc_t *mtoc = handle->master_toc;

    tag = id3_open_mem(0, ID3_OPENF_CREATE);

    if (handle->id3_tag_mode == 4 || handle->id3_tag_mode == 5)
        tag->id3_version = 4;

    // TIT2 track title
    frame = id3_add_frame(tag, ID3_TIT2); 
    if (handle->area[area].area_track_text[track].track_type_title != NULL)
    {         
        id3_set_text_wraper(frame, handle->area[area].area_track_text[track].track_type_title, handle->id3_tag_mode);
    }
    else if (handle->area[area].area_track_text[track].track_type_title_phonetic != NULL)
    {           
        id3_set_text_wraper(frame, handle->area[area].area_track_text[track].track_type_title_phonetic, handle->id3_tag_mode);
    }
    else
        id3_set_text_wraper(frame, "no track title", handle->id3_tag_mode);
    
    // Title of album   
    if (master_text->album_title != NULL)
        album_title = master_text->album_title;
    else if (master_text->album_title_phonetic != NULL)
        album_title = master_text->album_title_phonetic;

    if (album_title != NULL)
    {      
        frame = id3_add_frame(tag, ID3_TALB);           
        id3_set_text_wraper(frame, album_title, handle->id3_tag_mode); 
    }

    // Disc Title
    //  if disc_title exists and differ from album title or it is a  disc in a set
    if (master_text->disc_title != NULL)
        disc_title = master_text->disc_title;
    else if (master_text->disc_title_phonetic != NULL)
        disc_title = master_text->disc_title_phonetic;

    if(disc_title != NULL && album_title != NULL)
    {
        if((strcmp(disc_title, album_title) != 0) ||   // Titles are not identical
                            mtoc->album_set_size > 1 )                 // or a set discs
        {
            if(tag->id3_version == 4) // ID3 v2.4
            {
                
                frame = id3_add_frame(tag, ID3_TSST);
                id3_set_text_wraper(frame, disc_title, handle->id3_tag_mode); 
                
            }
            else  // // ID3v2.3 has no official disc title frame. Use TXXX (user-defined text) frame with description "SETSUBTITLE" or "DISCSUBTITLE."
            {
                    frame = id3_add_frame(tag, ID3_TXXX);
                id3_set_text_txxx_wraper(frame, "DISCSUBTITLE", disc_title, handle->id3_tag_mode); 
                
            }
        }
    }
    else if(disc_title != NULL)
    {      
        frame = id3_add_frame(tag, ID3_TALB);           
        id3_set_text_wraper(frame, disc_title, handle->id3_tag_mode); 
    }  


    // Track Artists (Artist name /performer)
    if (handle->area[area].area_track_text[track].track_type_performer != NULL ||
        handle->area[area].area_track_text[track].track_type_performer_phonetic != NULL)
    {
        char *performer;

        if (handle->area[area].area_track_text[track].track_type_performer != NULL)
            performer = handle->area[area].area_track_text[track].track_type_performer;
        else
            performer = handle->area[area].area_track_text[track].track_type_performer_phonetic;

        frame = id3_add_frame(tag, ID3_TPE1); // Artist, soloist
                                              //id3_set_text(frame, performer);       //TPE1 = The 'Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group' is used for the main artist(s). They are seperated with the "/" character
        id3_set_text_wraper(frame, performer, handle->id3_tag_mode);

        //frame = id3_add_frame(tag, ID3_TPE3);  // TPE3=Conductor/performer refinement;  TOPE='Original artist(s)/performer(s)' IPLS -Involved people(performer?)
       
    }
    else
    {
        master_text_t *master_text = &handle->master_text;
        char *artist = 0;
       
        if (master_text->disc_artist != NULL)
            artist = master_text->disc_artist;
        else if (master_text->disc_artist_phonetic != NULL)
            artist = master_text->disc_artist_phonetic;
        else if (master_text->album_artist != NULL)
            artist = master_text->album_artist;
        else if (master_text->album_artist_phonetic != NULL)
            artist = master_text->album_artist_phonetic;
       
        if (artist != NULL)
        {
            frame = id3_add_frame(tag, ID3_TPE1);           
            id3_set_text_wraper(frame, artist, handle->id3_tag_mode);
        }
    }

    if (handle->id3_tag_mode != 2 && handle->id3_tag_mode != 5) // not minimal
    {

        // TPE2 is widely used as album artist
        if (handle->master_text.album_artist != NULL)
        {
            
            frame = id3_add_frame(tag, ID3_TPE2); // TPE2: The 'Band/Orchestra/Accompaniment' frame is used for additional information about the performers in the recording

            id3_set_text_wraper(frame, handle->master_text.album_artist, handle->id3_tag_mode);
        }

        // ID3_TXXX:Performer
        if (handle->area[area].area_track_text[track].track_type_performer != NULL ||
            handle->area[area].area_track_text[track].track_type_performer_phonetic != NULL)
        {
            char *performer;

            if (handle->area[area].area_track_text[track].track_type_performer != NULL)
                performer = handle->area[area].area_track_text[track].track_type_performer;
            else
                performer = handle->area[area].area_track_text[track].track_type_performer_phonetic ;


            frame = id3_add_frame(tag, ID3_TXXX); // ID3_TXXX, Performer

            id3_set_text_txxx_wraper(frame,"PERFORMER", performer, handle->id3_tag_mode);

        }

        // Songwriter
        if (handle->area[area].area_track_text[track].track_type_songwriter != NULL ||
            handle->area[area].area_track_text[track].track_type_songwriter_phonetic != NULL  )
        {
            char *songwriter;

            if (handle->area[area].area_track_text[track].track_type_songwriter != NULL)
                songwriter = handle->area[area].area_track_text[track].track_type_songwriter;
            else
                songwriter = handle->area[area].area_track_text[track].track_type_songwriter_phonetic;

            frame = id3_add_frame(tag, ID3_TEXT);

            id3_set_text_wraper(frame, songwriter, handle->id3_tag_mode);           
        }

        // Composer
        if (handle->area[area].area_track_text[track].track_type_composer != NULL ||
            handle->area[area].area_track_text[track].track_type_composer_phonetic != NULL )
        {
            char *composer;

            if (handle->area[area].area_track_text[track].track_type_composer != NULL)
                composer = handle->area[area].area_track_text[track].track_type_composer;
            else
                composer = handle->area[area].area_track_text[track].track_type_composer_phonetic;


            frame = id3_add_frame(tag, ID3_TCOM);
            id3_set_text_wraper(frame, composer, handle->id3_tag_mode);           
        }

        // Arranger
        if (handle->area[area].area_track_text[track].track_type_arranger != NULL ||
            handle->area[area].area_track_text[track].track_type_arranger_phonetic != NULL)
        {
            char *arranger;

            if(handle->area[area].area_track_text[track].track_type_arranger != NULL )
                arranger = handle->area[area].area_track_text[track].track_type_arranger;
            else
                arranger = handle->area[area].area_track_text[track].track_type_arranger_phonetic;

            if(tag->id3_version == 4) // ID3 v2.4
                frame = id3_add_frame(tag, ID3_TIPL);
            else
               frame = id3_add_frame(tag, ID3_IPLS);

            id3_set_text_wraper(frame, arranger, handle->id3_tag_mode);           
        }

        // Message --> Comment
        if (handle->area[area].area_track_text[track].track_type_message != NULL ||
            handle->area[area].area_track_text[track].track_type_message_phonetic != NULL )
        {
            char *comment_text;
            char lang3[4] = {'e','n','g','\0'}; // default
            char lang2[3] ={'e','n','\0'};  // default

            if(handle->area[area].area_track_text[track].track_type_message != NULL)
                comment_text = handle->area[area].area_track_text[track].track_type_message;
            else
                comment_text = handle->area[area].area_track_text[track].track_type_message_phonetic;

            if(handle->area[area].area_toc->channel_count > 0)
                memcpy(lang2,handle->area[area].area_toc->languages[0].language_code, 2);


                // look up into lang_table to find the corresponding 3 letttes ISO-639-2
            int n=0;
            while( lang_table[n].lang2[0] != '\0' && lang_table[n].lang3[0] != '\0')
            {
                if(strncmp(lang2, lang_table[n].lang2, 2) != 0)
                {
                    memcpy(lang3,lang_table[n].lang3, 3);
                    break;
                }
                n++;
                if(n > 500) break; // avoid infinite loop
            }

            frame = id3_add_frame(tag, ID3_COMM);
            id3_set_comment_wraper(frame,"MESSAGE",comment_text, handle->id3_tag_mode, lang3);          
        }    

        // Extra Message --> TXXX
        if (handle->area[area].area_track_text[track].track_type_extra_message != NULL ||
            handle->area[area].area_track_text[track].track_type_extra_message_phonetic != NULL )
        {
            char *extramesage_text;
            
            if (handle->area[area].area_track_text[track].track_type_extra_message != NULL)
                extramesage_text = handle->area[area].area_track_text[track].track_type_extra_message;
            else
                extramesage_text =  handle->area[area].area_track_text[track].track_type_extra_message_phonetic;

            frame = id3_add_frame(tag, ID3_TXXX); 
            id3_set_text_txxx_wraper(frame,"Extra message", extramesage_text, handle->id3_tag_mode);            
        }  
        
        
        // ID3_TXXX:Catalog Number
        if (mtoc->disc_catalog_number[0] != '\0')
        {
            memset(cat_str_d,0,sizeof(cat_str_d));
            memcpy(cat_str_d, mtoc->disc_catalog_number, 16);
            cat_str_d[16] = '\0';

            trim_whitespace(cat_str_d);
   
            frame = id3_add_frame(tag, ID3_TXXX); 
            id3_set_text_txxx_wraper(frame,"Catalog Number", cat_str_d, handle->id3_tag_mode);

        }

        if(mtoc->album_catalog_number[0] != '\0')
        {
            char cat_str_a[20];
            memset(cat_str_a,0,sizeof(cat_str_a));
            memcpy(cat_str_a, mtoc->album_catalog_number, 16);
            cat_str_a[16] = '\0';

            trim_whitespace(cat_str_a);

            if(mtoc->disc_catalog_number[0] == '\0')
            {
                frame = id3_add_frame(tag, ID3_TXXX);
                id3_set_text_txxx_wraper(frame,"Catalog Number", cat_str_a, handle->id3_tag_mode);
            }
            else if(strcmp(cat_str_a,cat_str_d) != 0)  // not identical
            {
                frame = id3_add_frame(tag, ID3_TXXX);
                id3_set_text_txxx_wraper(frame,"Album Catalog Number", cat_str_a, handle->id3_tag_mode);
            }

        }
        

        // ISRC
        isrc_t isrc_st = handle->area[area].area_isrc_genre->isrc[track];
        if ((isrc_st.country_code[0] != 0x0) || (isrc_st.owner_code[0] != 0x0) || (isrc_st.designation_code[0] !=0x0) || (isrc_st.recording_year[0] != 0x0) )
        {
            char isrc[16];
            
            memset(isrc,0,sizeof(isrc));
            memcpy(isrc, handle->area[area].area_isrc_genre->isrc[track].country_code, 2);
            memcpy(isrc + 2, handle->area[area].area_isrc_genre->isrc[track].owner_code, 3);
            memcpy(isrc + 5, handle->area[area].area_isrc_genre->isrc[track].recording_year, 2);
            memcpy(isrc + 7, handle->area[area].area_isrc_genre->isrc[track].designation_code, 5);

            frame = id3_add_frame(tag, ID3_TSRC);

            id3_set_text_wraper(frame, isrc, handle->id3_tag_mode);
               
        }

        // Copyright
        if (handle->area[area].area_track_text[track].track_type_copyright != NULL || 
            handle->area[area].area_track_text[track].track_type_copyright_phonetic != NULL ||
            handle->master_text.album_copyright != NULL)
        {
            char *copyright;
            
            if(handle->area[area].area_track_text[track].track_type_copyright != NULL)
                copyright = handle->area[area].area_track_text[track].track_type_copyright;
            else if (handle->area[area].area_track_text[track].track_type_copyright_phonetic != NULL)
                copyright = handle->area[area].area_track_text[track].track_type_copyright_phonetic;
            else
                copyright = handle->master_text.album_copyright;

            frame = id3_add_frame(tag, ID3_TCOP);         
           
            id3_set_text_wraper(frame, copyright,handle->id3_tag_mode);            
        }

        // Publisher
        if (handle->master_text.album_publisher != NULL)
        {
            char *publisher = handle->master_text.album_publisher;
            frame = id3_add_frame(tag, ID3_TPUB);
            id3_set_text_wraper(frame, publisher, handle->id3_tag_mode);            
        }

        // Part of set. Disc sequence/set size
        char str[64];
        snprintf(str, 64, "%d/%d", mtoc->album_sequence_number, mtoc->album_set_size);
        frame = id3_add_frame(tag, ID3_TPOS);                    
        id3_set_text_wraper(frame, str, handle->id3_tag_mode);
       

        // Genre ; only if it uses General Genre Table == 0x01
        // TO DO: to implement Japanese Genre Table
        genre_table_t *genre_t = &handle->area[area].area_isrc_genre->track_genre[track];
        if(genre_t->category == 0x01)  // uses General Genre Table
        {
            if(genre_t->genre > 0 && genre_t->genre < MAX_GENRE_COUNT)
            {
                frame = id3_add_frame(tag, ID3_TCON);       
                //id3_set_text_wraper(frame, (char *)genre_table[sacd_id3_genres[genre_t->genre]], handle->id3_tag_mode);
                id3_set_text_wraper(frame, album_genre[genre_t->genre], handle->id3_tag_mode);
            }     
        }
        // else if (genre_t->category == 0x02)  //to implement Japanese Genre Table
        // {}

        //  Version 2.3 –> version 2.4
        //TYER, TDAT, TIME –> TDRC
        if(tag->id3_version == 4) // ID3 v2.4
        {
            snprintf(tmp, 200, "%04d-%02d-%02d", handle->master_toc->disc_date_year,handle->master_toc->disc_date_month, handle->master_toc->disc_date_day);
            frame = id3_add_frame(tag, ID3_TDRC);
            id3_set_text_wraper(frame, tmp, handle->id3_tag_mode);
        }
        else  // ID3 v2.3
        {  
            // YEAR
            //ID3 v2.3 Date Frames
            // TYER year (recording year of form YYYY, always 4 bytes)
            snprintf(tmp, 200, "%04d", handle->master_toc->disc_date_year);
            frame = id3_add_frame(tag, ID3_TYER);
            id3_set_text_wraper(frame, tmp, handle->id3_tag_mode);

            // Month, day
            //ID3 v2.3 Date Frames
            //TDAT date (recording date of form DDMM, always 4 bytes)
            snprintf(tmp, 200, "%02d%02d", handle->master_toc->disc_date_day,handle->master_toc->disc_date_month);
            frame = id3_add_frame(tag, ID3_TDAT);
            id3_set_text_wraper(frame, tmp, handle->id3_tag_mode);
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
