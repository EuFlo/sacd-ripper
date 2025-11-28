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
#include <inttypes.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#endif

#include <charset.h>
#include <logging.h>
#include <wchar.h>

#include "endianess.h"
#include "scarletbook.h"
#include "scarletbook_read.h"
#include "scarletbook_helpers.h"
#include "sacd_reader.h"
#include "sacd_read_internal.h"
#include "utils.h"

#ifndef NDEBUG
#define CHECK_ZERO0(arg)                                                       \
    if (arg != 0) {                                                            \
        fprintf(stderr, "*** Zero check failed in %s:%i\n    for %s = 0x%x\n", \
                __FILE__, __LINE__, # arg, arg);                               \
    }
#define CHECK_ZERO(arg)                                                    \
    if (memcmp(my_friendly_zeros, &arg, sizeof(arg))) {                    \
        unsigned int i_CZ;                                                 \
        fprintf(stderr, "*** Zero check failed in %s:%i\n    for %s = 0x", \
                __FILE__, __LINE__, # arg);                                \
        for (i_CZ = 0; i_CZ < sizeof(arg); i_CZ++)                         \
            fprintf(stderr, "%02x", *((uint8_t *) &arg + i_CZ));           \
        fprintf(stderr, "\n");                                             \
    }
static const uint8_t my_friendly_zeros[2048];
#else
#define CHECK_ZERO0(arg)    (void) (arg)
#define CHECK_ZERO(arg)     (void) (arg)
#endif

/* Prototypes for internal functions */
static int scarletbook_read_master_toc(scarletbook_handle_t *);
static int scarletbook_read_area_toc(scarletbook_handle_t *, int);


scarletbook_handle_t *scarletbook_open(sacd_reader_t *sacd)
{
    scarletbook_handle_t *sb;

    sb = (scarletbook_handle_t *) calloc(sizeof(scarletbook_handle_t), 1);
    if (!sb)
        return NULL;

#ifdef __lv2ppu__
    sb->frame.data = (uint8_t *) memalign(128, MAX_DST_SIZE);  // (1024 * 64)
#else
    sb->frame.data = (uint8_t *) malloc(MAX_DST_SIZE);			//(1024 * 64)
#endif

    if (!sb->frame.data)
        return NULL;

    sb->sacd      = sacd;
    sb->twoch_area_idx = -1;
    sb->mulch_area_idx = -1;

    if (scarletbook_read_master_toc(sb)==0)
    {
        fwprintf(stdout, L"scarletbook_open: Can't read Master TOC !!\n");
        LOG(lm_main, LOG_ERROR, ("Error: scarletbook_open: Can't read Master TOC !!"));
        free(sb->frame.data);
        free(sb);
        return NULL;
    }

    if (sb->master_toc->area_1_toc_1_start > 0) // Area 1 (TWOCHTOC) TOC-1
    {
        int flag_use_toc1 = 0;
        int flag_use_toc2 = 0;

        sb->area[sb->area_count].area_data = malloc(sb->master_toc->area_1_toc_size * SACD_LSN_SIZE);
        if (sb->area[sb->area_count].area_data == NULL)
        {
            fwprintf(stdout, L"Can't alocate memory for Area 1 (TWOCHTOC) TOC-1 !!\n");
            LOG(lm_main, LOG_ERROR, ("Error: Can't alocate memory for Area 1 (TWOCHTOC) TOC-1 !!"));
        }
        else
        {
            if (!sacd_read_block_raw(sacd, sb->master_toc->area_1_toc_1_start,(uint32_t) sb->master_toc->area_1_toc_size, sb->area[sb->area_count].area_data))
            {
                fwprintf(stdout, L"Can't read Area 1 (TWOCHTOC) TOC-1 !! Trying to read and use TOC-2...\n");
                LOG(lm_main, LOG_NOTICE, ("Warning: Can't read Area 1 (TWOCHTOC) TOC-1 !! Trying to read and use TOC-2..."));
                flag_use_toc2 = 1;               
            }
            else
              flag_use_toc1 = 1;

            // check if Area 1 (TWOCHTOC) TOC-1 is identical with backup AREA 1 (TWOCHTOC) TOC-2
            if (sb->master_toc->area_1_toc_2_start > 0) // Area 1 (TWOCHTOC) TOC-2
            {
                sb->area[2].area_data = malloc(sb->master_toc->area_1_toc_size * SACD_LSN_SIZE);
                if (sb->area[2].area_data == NULL)
                {
                    fwprintf(stdout, L"Error: Can't alocate memory for backup Area 1 (TWOCHTOC) TOC-2.\n");
                    LOG(lm_main, LOG_ERROR, ("Error: Can't alocate memory for backup Area 1 (TWOCHTOC) TOC-2."));
                    flag_use_toc2 = 0;
                }
                else
                {
                    if (!sacd_read_block_raw(sacd, sb->master_toc->area_1_toc_2_start, (uint32_t)sb->master_toc->area_1_toc_size, sb->area[2].area_data))
                    {
                        fwprintf(stdout, L"Warning: can't read Area 1 (TWOCHTOC) TOC-2 !! There are some errros on disc !\n");
                        LOG(lm_main, LOG_NOTICE, ("Warning: can't read Area 1 (TWOCHTOC) TOC-2 !! There are some errros on disc !"));
                        flag_use_toc2 = 0;
                    }
                    else  // compare
                    {
                        // if not identical then copy TOC-2 in TOC-1. 
                        int res_cmp = memcmp((const void *)sb->area[sb->area_count].area_data, (const void *)sb->area[2].area_data, (size_t)((size_t)sb->master_toc->area_1_toc_size * SACD_LSN_SIZE));
                        if (res_cmp != 0x00)
                        {
                            fwprintf(stdout, L"Warning: Area 1 (TWOCHTOC) TOC-1 did not match with Area 1 (TWOCHTOC) TOC-2. Disc has some errors !! Using TOC-1... \n");
                            LOG(lm_main, LOG_NOTICE, ("Warning: Area 1 (TWOCHTOC) TOC-1 did not match with Area 1 (TWOCHTOC) TOC-2. Disc has some errors !! Using TOC-1... "));
                            flag_use_toc1 = 1;
                            flag_use_toc2 = 0;
                        }
                    }
                    if (flag_use_toc2 == 1)
                        memcpy((void *)sb->area[sb->area_count].area_data, (void *)sb->area[2].area_data, (size_t)((size_t)sb->master_toc->area_1_toc_size * SACD_LSN_SIZE));

                    free(sb->area[2].area_data);
                }
                
            }

            if (((flag_use_toc1 == 1) || (flag_use_toc2 == 1)) &&
                (scarletbook_read_area_toc(sb, sb->area_count) == 1))
            {
                ++sb->area_count;
            }
            else
                fwprintf(stdout, L"libsacdread: Erors processing Area 1 (TWOCHTOC)!!\n");                        
        }

    }
 
    if (sb->master_toc->area_2_toc_1_start > 0) //  Area 2 (MULCHTOC) TOC-1
    {
        int flag_use_toc1 = 0;
        int flag_use_toc2 = 0;

        sb->area[sb->area_count].area_data = malloc(sb->master_toc->area_2_toc_size * SACD_LSN_SIZE);
        if (!sb->area[sb->area_count].area_data)
        {
            fwprintf(stdout, L"Error: can't alocate memory for Area 2 (MULCHTOC) TOC-1 !!\n");
            LOG(lm_main, LOG_ERROR, ("Error: can't alocate memory for Area 2 (MULCHTOC) TOC-1 !!"));
        }
        else
        {
            if (!sacd_read_block_raw(sacd, sb->master_toc->area_2_toc_1_start, (uint32_t)sb->master_toc->area_2_toc_size, sb->area[sb->area_count].area_data))
            {
                fwprintf(stdout, L"Error: can't read Area 2 (MULCHTOC) TOC-1 !! Trying to read and use TOC-2...\n");
                LOG(lm_main, LOG_ERROR, ("Error: can't read Area 2 (MULCHTOC) TOC-1 !! Trying to read and use TOC-2..."));
                flag_use_toc2 = 1;
            }
            else
                flag_use_toc1 = 1;

            // check if are identical Area 2 (MULCHTOC) TOC-1 with backup Area 2 (MULCHTOC) TOC-2
            if (sb->master_toc->area_2_toc_2_start > 0) // Area 2 (MULCHTOC) TOC-2
            {
                sb->area[3].area_data = malloc(sb->master_toc->area_2_toc_size * SACD_LSN_SIZE);

                if (sb->area[3].area_data == NULL)
                {
                    fwprintf(stdout, L"Error: can't alocate memory for backup Area 2 (MULCHTOC)  TOC-2.\n");
                    flag_use_toc2 = 0;
                }
                else 
                {
                    if (!sacd_read_block_raw(sacd, sb->master_toc->area_2_toc_2_start, (uint32_t)sb->master_toc->area_2_toc_size, sb->area[3].area_data))
                    {
                        fwprintf(stdout, L"Warning: can't read Area 2 (MULCHTOC) TOC-2 !! There are some errros on disc !\n");
                        LOG(lm_main, LOG_NOTICE, ("Warning: can't read Area 2 (MULCHTOC) TOC-2 !! There are some errros on disc !"));
                        flag_use_toc2 = 0;
                    }
                    else // compare
                    {
                        // if not identical then copy TOC-2 in TOC-1.
                        int res_cmp = memcmp((const void *)sb->area[sb->area_count].area_data, (const void *)sb->area[3].area_data, (size_t)((size_t)sb->master_toc->area_2_toc_size * SACD_LSN_SIZE));
                        if (res_cmp != 0x00)
                        {
                            fwprintf(stdout, L"Warning: Area 2 (MULCHTOC) TOC-1 did not match with Area 2 (MULCHTOC) TOC-2. Disc has some errors !! Using TOC-1... \n");
                            flag_use_toc1 = 1;
                            flag_use_toc2 = 0;
                        }
                    }
                    if (flag_use_toc2 == 1)
                        memcpy((void *)sb->area[sb->area_count].area_data, (void *)sb->area[2].area_data, (size_t)((size_t)sb->master_toc->area_1_toc_size * SACD_LSN_SIZE));

                    free(sb->area[3].area_data);
                }
            }

            if (((flag_use_toc1 == 1) || (flag_use_toc2 == 1) ) &&
                ( scarletbook_read_area_toc(sb, sb->area_count) == 1) )
            {
                ++sb->area_count;               
            }
            else
              fwprintf(stdout, L"Error processing Area 2 (MULCHTOC). \n");
        }
    }

    if (sb->area_count == 0)
    {
        free(sb->frame.data);
        free(sb);
        return NULL;
    }

    return sb;
}

static void free_area(scarletbook_area_t *area)
{
    int i;
    
    for (i = 0; i < area->area_toc->track_count; i++)
    {
        free(area->area_track_text[i].track_type_title);
        free(area->area_track_text[i].track_type_performer);
        free(area->area_track_text[i].track_type_songwriter);
        free(area->area_track_text[i].track_type_composer);
        free(area->area_track_text[i].track_type_arranger);
        free(area->area_track_text[i].track_type_message);
        free(area->area_track_text[i].track_type_extra_message);
        free(area->area_track_text[i].track_type_copyright);
        free(area->area_track_text[i].track_type_title_phonetic);
        free(area->area_track_text[i].track_type_performer_phonetic);
        free(area->area_track_text[i].track_type_songwriter_phonetic);
        free(area->area_track_text[i].track_type_composer_phonetic);
        free(area->area_track_text[i].track_type_arranger_phonetic);
        free(area->area_track_text[i].track_type_message_phonetic);
        free(area->area_track_text[i].track_type_extra_message_phonetic);
        free(area->area_track_text[i].track_type_copyright_phonetic);
    }

    free(area->description);
    free(area->copyright);
    free(area->description_phonetic);
    free(area->copyright_phonetic);
}

void scarletbook_close(scarletbook_handle_t *handle)
{
    if (!handle)
        return;

    if (has_two_channel(handle))
    {
        free_area(&handle->area[handle->twoch_area_idx]);
        free(handle->area[handle->twoch_area_idx].area_data);
    }

    if (has_multi_channel(handle))
    {
        free_area(&handle->area[handle->mulch_area_idx]);
        free(handle->area[handle->mulch_area_idx].area_data);
    }

    {
        master_text_t *mt = &handle->master_text;
        free(mt->album_title);
        free(mt->album_title_phonetic);
        free(mt->album_artist);
        free(mt->album_artist_phonetic);
        free(mt->album_publisher);
        free(mt->album_publisher_phonetic);
        free(mt->album_copyright);
        free(mt->album_copyright_phonetic);
        free(mt->disc_title);
        free(mt->disc_title_phonetic);
        free(mt->disc_artist);
        free(mt->disc_artist_phonetic);
        free(mt->disc_publisher);
        free(mt->disc_publisher_phonetic);
        free(mt->disc_copyright);
        free(mt->disc_copyright_phonetic);
    } 

    if (handle->master_data)
        free((void *) handle->master_data);

    if (handle->frame.data)
        free((void *) handle->frame.data);

    memset(handle, 0, sizeof(scarletbook_handle_t));

    free(handle);
    handle = 0;
}
//  Read Master TOC
//   input scarletbook_handle_t *handle
//   User  must  free (master_toc_t *) handle->master_data
//   Returns : 1 on success;   0 if errors
static int scarletbook_read_master_toc(scarletbook_handle_t *handle)
{
    int          i;
    uint8_t      * p;
    master_toc_t *master_toc;

    handle->master_data = malloc(MASTER_TOC_LEN * SACD_LSN_SIZE);
    if (!handle->master_data)
        return 0;

    if (!sacd_read_block_raw(handle->sacd, START_OF_MASTER_TOC, MASTER_TOC_LEN, handle->master_data))
        return 0;

    master_toc = handle->master_toc = (master_toc_t *) handle->master_data;

    if (strncmp("SACDMTOC", master_toc->id, 8) != 0)
    {
        fwprintf(stdout, L"scarletbook_read_master_toc: Not a ScarletBook disc!\n");
        LOG(lm_main, LOG_ERROR, ("scarletbook_read_master_toc(): Not a ScarletBook disc!"));
        return 0;
    }

    SWAP16(master_toc->album_set_size);
    SWAP16(master_toc->album_sequence_number);
    SWAP32(master_toc->area_1_toc_1_start);
    SWAP32(master_toc->area_1_toc_2_start);
    SWAP16(master_toc->area_1_toc_size);
    SWAP32(master_toc->area_2_toc_1_start);
    SWAP32(master_toc->area_2_toc_2_start);
    SWAP16(master_toc->area_2_toc_size);
    SWAP16(master_toc->disc_date_year);

    if (master_toc->version.major > SUPPORTED_VERSION_MAJOR || master_toc->version.minor > SUPPORTED_VERSION_MINOR)
    {
        fwprintf(stdout, L"libsacdread: Unsupported version: %i.%02i\n", master_toc->version.major, master_toc->version.minor);
        LOG(lm_main, LOG_ERROR, ("libsacdread: Unsupported version: %i.%02i", master_toc->version.major, master_toc->version.minor));
        return 0;
    }

    uint32_t total_sectors = sacd_get_total_sectors((sacd_reader_t*)handle->sacd); // get the real full size of disc [number of sectors] or file [number of SACD_LSN_SIZE]
    handle->total_sectors_iso = total_sectors;

    // made some checks on the total size of iso/disc
    uint32_t area1_sectors_max = master_toc->area_1_toc_2_start + master_toc->area_1_toc_size;
    uint32_t area2_sectors_max = master_toc->area_2_toc_2_start + master_toc->area_2_toc_size;
    uint32_t max_sectors = area2_sectors_max > area1_sectors_max ? area2_sectors_max : area1_sectors_max;

    if (max_sectors <= total_sectors)
    {
        fwprintf(stdout, L"The size of sacd is ok (sectors=%d). Size is: %llu bytes, %.3f GB (gigabyte) \n", total_sectors, (uint64_t)total_sectors * SACD_LSN_SIZE, (double)total_sectors * SACD_LSN_SIZE / (1000 * 1000 * 1000));
        LOG(lm_main, LOG_NOTICE, ("Notice in scarletbook_read_master():The size of sacd is ok (sectors=%d). Size is: %llu bytes, %.3f GB (gigabyte)", total_sectors, (uint64_t)total_sectors * SACD_LSN_SIZE, (double)total_sectors * SACD_LSN_SIZE / (1000 * 1000 * 1000)));
    }
    else
    {
        fwprintf(stdout, L"\nError: the reported size (sectors) of sacd is not ok (sectors=%u) < (max_sectors=%u) !\n", total_sectors, max_sectors);
        LOG(lm_main, LOG_ERROR, ("Error in scarletbook_read_master_toc(): the reported size (sectors) of sacd is not ok (sectors=%u) < (max_sectors=%u)", total_sectors, max_sectors));
        //return 0;
    }

    CHECK_ZERO(master_toc->reserved01);
    CHECK_ZERO(master_toc->reserved02);
    CHECK_ZERO(master_toc->reserved03);
    CHECK_ZERO(master_toc->reserved04);
    CHECK_ZERO(master_toc->reserved05);
    CHECK_ZERO(master_toc->reserved06);
    for (i = 0; i < 4; i++)
    {
        CHECK_ZERO(master_toc->album_genre[i].reserved);
        CHECK_ZERO(master_toc->disc_genre[i].reserved);
        CHECK_VALUE(master_toc->album_genre[i].category <= MAX_CATEGORY_COUNT);
        CHECK_VALUE(master_toc->disc_genre[i].category <= MAX_CATEGORY_COUNT);
        SWAP16(master_toc->album_genre[i].genre);
        SWAP16(master_toc->disc_genre[i].genre);
        CHECK_VALUE(master_toc->album_genre[i].genre <= MAX_GENRE_COUNT);
        CHECK_VALUE(master_toc->disc_genre[i].genre <= MAX_GENRE_COUNT);
    }

    CHECK_VALUE(master_toc->text_area_count <= MAX_LANGUAGE_COUNT); //N_Text_Channels

    // point to eof master header
    p = handle->master_data + SACD_LSN_SIZE;

    // set pointers to text content
    for (i = 0; i < MAX_LANGUAGE_COUNT; i++)
    {
        master_sacd_text_t *master_text = (master_sacd_text_t *) p;

        if (strncmp("SACDText", master_text->id, 8) != 0)
        {
            fwprintf(stdout, L"\nError in scarletbook_read_master_toc(): SACDText did not found at LANGUAGE idx=[%d]\n",i);
            LOG(lm_main, LOG_ERROR, ("Error in scarletbook_read_master_toc(): SACDText did not found at LANGUAGE idx=[%d]",i));
            return 0;
        }

        CHECK_ZERO(master_text->reserved);

        SWAP16(master_text->album_title_position);  // If Album_Title is used, the Album_Title field is present and Album_Title_Ptr must have the value 64.
        SWAP16(master_text->album_title_phonetic_position);
        SWAP16(master_text->album_artist_position);
        SWAP16(master_text->album_artist_phonetic_position);
        SWAP16(master_text->album_publisher_position);
        SWAP16(master_text->album_publisher_phonetic_position);
        SWAP16(master_text->album_copyright_position);
        SWAP16(master_text->album_copyright_phonetic_position);
        SWAP16(master_text->disc_title_position);
        SWAP16(master_text->disc_title_phonetic_position);
        SWAP16(master_text->disc_artist_position);
        SWAP16(master_text->disc_artist_phonetic_position);
        SWAP16(master_text->disc_publisher_position);
        SWAP16(master_text->disc_publisher_phonetic_position);
        SWAP16(master_text->disc_copyright_position);
        SWAP16(master_text->disc_copyright_phonetic_position);

        // we only use the first SACDText entry
        if (i == 0)
        {
            char *current_charset;
            uint8_t character_set_idx;

            character_set_idx = handle->master_toc->locales[i].character_set & 0x07;
            if(character_set_idx < MAX_LANGUAGE_COUNT )
                current_charset = (char *)character_set[character_set_idx];
            else
                current_charset = (char *)character_set[2]; // set default  to 'ISO-8859-1'


            if (master_text->album_title_position)
                handle->master_text.album_title = charset_convert((char *) master_text + master_text->album_title_position, strlen((char *) master_text + master_text->album_title_position), current_charset, "UTF-8");
            if (master_text->album_title_phonetic_position)
                handle->master_text.album_title_phonetic = charset_convert((char *) master_text + master_text->album_title_phonetic_position, strlen((char *) master_text + master_text->album_title_phonetic_position), current_charset, "UTF-8");
            if (master_text->album_artist_position)
                handle->master_text.album_artist = charset_convert((char *) master_text + master_text->album_artist_position, strlen((char *) master_text + master_text->album_artist_position), current_charset, "UTF-8");
            if (master_text->album_artist_phonetic_position)
                handle->master_text.album_artist_phonetic = charset_convert((char *) master_text + master_text->album_artist_phonetic_position, strlen((char *) master_text + master_text->album_artist_phonetic_position), current_charset, "UTF-8");
            if (master_text->album_publisher_position)
                handle->master_text.album_publisher = charset_convert((char *) master_text + master_text->album_publisher_position, strlen((char *) master_text + master_text->album_publisher_position), current_charset, "UTF-8");
            if (master_text->album_publisher_phonetic_position)
                handle->master_text.album_publisher_phonetic = charset_convert((char *) master_text + master_text->album_publisher_phonetic_position, strlen((char *) master_text + master_text->album_publisher_phonetic_position), current_charset, "UTF-8");
            if (master_text->album_copyright_position)
                handle->master_text.album_copyright = charset_convert((char *) master_text + master_text->album_copyright_position, strlen((char *) master_text + master_text->album_copyright_position), current_charset, "UTF-8");
            if (master_text->album_copyright_phonetic_position)
                handle->master_text.album_copyright_phonetic = charset_convert((char *) master_text + master_text->album_copyright_phonetic_position, strlen((char *) master_text + master_text->album_copyright_phonetic_position), current_charset, "UTF-8");

            if (master_text->disc_title_position)
                handle->master_text.disc_title = charset_convert((char *) master_text + master_text->disc_title_position, strlen((char *) master_text + master_text->disc_title_position), current_charset, "UTF-8");
            if (master_text->disc_title_phonetic_position)
                handle->master_text.disc_title_phonetic = charset_convert((char *) master_text + master_text->disc_title_phonetic_position, strlen((char *) master_text + master_text->disc_title_phonetic_position), current_charset, "UTF-8");
            if (master_text->disc_artist_position)
                handle->master_text.disc_artist = charset_convert((char *) master_text + master_text->disc_artist_position, strlen((char *) master_text + master_text->disc_artist_position), current_charset, "UTF-8");
            if (master_text->disc_artist_phonetic_position)
                handle->master_text.disc_artist_phonetic = charset_convert((char *) master_text + master_text->disc_artist_phonetic_position, strlen((char *) master_text + master_text->disc_artist_phonetic_position), current_charset, "UTF-8");
            if (master_text->disc_publisher_position)
                handle->master_text.disc_publisher = charset_convert((char *) master_text + master_text->disc_publisher_position, strlen((char *) master_text + master_text->disc_publisher_position), current_charset, "UTF-8");
            if (master_text->disc_publisher_phonetic_position)
                handle->master_text.disc_publisher_phonetic = charset_convert((char *) master_text + master_text->disc_publisher_phonetic_position, strlen((char *) master_text + master_text->disc_publisher_phonetic_position), current_charset, "UTF-8");
            if (master_text->disc_copyright_position)
                handle->master_text.disc_copyright = charset_convert((char *) master_text + master_text->disc_copyright_position, strlen((char *) master_text + master_text->disc_copyright_position), current_charset, "UTF-8");
            if (master_text->disc_copyright_phonetic_position)
                handle->master_text.disc_copyright_phonetic = charset_convert((char *) master_text + master_text->disc_copyright_phonetic_position, strlen((char *) master_text + master_text->disc_copyright_phonetic_position), current_charset, "UTF-8");
        }

        p += SACD_LSN_SIZE;
    }

    handle->master_man = (master_man_t *) p;
    if (strncmp("SACD_Man", handle->master_man->id, 8) != 0)
    {
        fwprintf(stdout, L"\nError in scarletbook_read_master_toc(): SACD_Man did not found!!!\n");
        LOG(lm_main, LOG_ERROR, ("Error in scarletbook_read_master_toc(): SACD_Man did not found!!"));
        return 0;
    }

    return 1;
}

static int scarletbook_read_area_toc(scarletbook_handle_t *handle, int area_idx)
{
    int                 i, j;
    area_toc_t         *area_toc;
    uint8_t            *area_data;
    uint8_t            *p;
    int                 sacd_text_idx = 0;
    scarletbook_area_t *area = &handle->area[area_idx];
    char *current_charset;
    uint8_t character_set_idx;
    int Track_List_1_is_present = 0;
    int Track_List_2_is_present = 0;
    int ISRC_and_Genre_List_is_present = 0;
    size_t total_sectors_read = 0;

    p = area_data = area->area_data;
    area_toc = area->area_toc = (area_toc_t *) area_data;

    if (strncmp("TWOCHTOC", area_toc->id, 8) != 0 && strncmp("MULCHTOC", area_toc->id, 8) != 0)
    {
        fwprintf(stdout, L"libsacdread: Not a valid Area TOC!\n");
        LOG(lm_main, LOG_ERROR, ("Error in scarletbook_read_area_toc(): libsacdread: Not a valid Area TOC!"));
        return 0;
    }

    SWAP16(area_toc->size);
    SWAP32(area_toc->track_start);
    SWAP32(area_toc->track_end);
    SWAP16(area_toc->area_description_offset);
    SWAP16(area_toc->copyright_offset);
    SWAP16(area_toc->area_description_phonetic_offset);
    SWAP16(area_toc->copyright_phonetic_offset);
    SWAP32(area_toc->max_byte_rate);
    SWAP16(area_toc->track_text_offset);// Track_Text_Ptr are 0, 5 and 37
    SWAP16(area_toc->index_list_offset);
    SWAP16(area_toc->access_list_offset);

    CHECK_ZERO(area_toc->reserved01);
    CHECK_ZERO(area_toc->reserved03);
    CHECK_ZERO(area_toc->reserved04);
    CHECK_ZERO(area_toc->reserved06);
    CHECK_ZERO(area_toc->reserved07);
    CHECK_ZERO(area_toc->reserved08);
    CHECK_ZERO(area_toc->reserved09);
    CHECK_ZERO(area_toc->reserved10);

    if(area_toc->track_text_offset != 0 && area_toc->track_text_offset != 5 && area_toc->track_text_offset != 37 )
    {
        fwprintf(stdout, L"Notice in scarletbook_read_area_toc(): Track_Text_Ptr is not 0, 5 or 37 \n");
        LOG(lm_main, LOG_NOTICE, ("Notice in scarletbook_read_area_toc(): Track_Text_Ptr is not 0, 5 or 37 "));
    }

    character_set_idx = area->area_toc->languages[sacd_text_idx].character_set & 0x07;
    if(character_set_idx < MAX_LANGUAGE_COUNT )
        current_charset = (char *)character_set[character_set_idx];
    else
        current_charset = (char *)character_set[2]; // set default  to 'ISO-8859-1'    

    if (area_toc->area_description_offset)
        area->description = charset_convert((char *)area_toc + area_toc->area_description_offset, strlen((char *)area_toc + area_toc->area_description_offset), current_charset, "UTF-8");
    if (area_toc->copyright_offset)
        area->copyright = charset_convert((char *) area_toc + area_toc->copyright_offset, strlen((char *) area_toc + area_toc->copyright_offset), current_charset, "UTF-8");
    if (area_toc->area_description_phonetic_offset)
        area->description_phonetic = charset_convert((char *)area_toc + area_toc->area_description_phonetic_offset, strlen((char *)area_toc + area_toc->area_description_phonetic_offset), current_charset, "UTF-8");
    if (area_toc->copyright_phonetic_offset)
        area->copyright_phonetic = charset_convert((char *) area_toc + area_toc->copyright_phonetic_offset, strlen((char *) area_toc + area_toc->copyright_phonetic_offset), current_charset, "UTF-8");

    if (area_toc->version.major > SUPPORTED_VERSION_MAJOR || area_toc->version.minor > SUPPORTED_VERSION_MINOR)
    {
        fwprintf(stdout, L"Notice in scarletbook_read_area_toc(): Unsupported area_toc version: %2i.%2i\n", area_toc->version.major, area_toc->version.minor);
        LOG(lm_main, LOG_NOTICE, ("Notice in scarletbook_read_area_toc(): Unsupported area_toc version: %2i.%2i", area_toc->version.major, area_toc->version.minor));
    }

    // is this the 2 channel?
    if (area_toc->channel_count == 2 && area_toc->loudspeaker_config == 0)
    {
        handle->twoch_area_idx = area_idx;
    }
    else
    {
        handle->mulch_area_idx = area_idx;
    }

    // Area_TOC_0 size is one SACD_LSN_SIZE
    p += SACD_LSN_SIZE; total_sectors_read +=1;

    while (p < (area_data + area_toc->size * SACD_LSN_SIZE))
    {

        if (strncmp((char *) p, "SACDTRL1", 8) == 0)  // Track_List_1 must always be present in an Area TOC
        {
            area_tracklist_offset_t *tracklist;
            tracklist = area->area_tracklist_offset = (area_tracklist_offset_t *) p;
            for (i = 0; i < area_toc->track_count; i++)
            {
                SWAP32(tracklist->track_start_lsn[i]);
                SWAP32(tracklist->track_length_lsn[i]);
            }
            p += SACD_LSN_SIZE; total_sectors_read +=1;
            Track_List_1_is_present = 1;
        }
        else if (strncmp((char *) p, "SACDTRL2", 8) == 0)   // Track_List_2 must always be present in an Area TOC !!!
        {
            area->area_tracklist_time = (area_tracklist_t *) p;            
            p += SACD_LSN_SIZE; total_sectors_read +=1;
            Track_List_2_is_present = 1;
        }
        else if (strncmp((char *) p, "SACD_IGL", 8) == 0)   // ISRC_and_Genre_List must always be present in an Area TOC. The length of ISRC_and_Genre_List is always two Sectors
        {
            area->area_isrc_genre = (area_isrc_genre_t *) p;
            for (i = 0; i < area_toc->track_count; i++)
            {
                SWAP16(area->area_isrc_genre->track_genre[i].genre);
            }
            p += SACD_LSN_SIZE * 2;total_sectors_read +=2;
            ISRC_and_Genre_List_is_present = 1;
        }
        else if (strncmp((char *) p, "SACD_ACC", 8) == 0)   // Access_List must be present if the Audio Area is DST coded. If the current Audio Area is Plain DSD, the Access_List is not allowed to be present.
        {
            // skip
            p += SACD_LSN_SIZE * 32;total_sectors_read +=32;
        }                
        else if ((area_toc->track_text_offset != 0) && strncmp((char *) p, "SACDTTxt", 8) == 0)  // Track_Text is only present if Track_Text_Ptr is not set to zero
        {   //The maximum length of Track_Text is 32 Sectors
            // we discard all other SACDTTxt entries  ; If Track_Text is present, it must contain minimally one Text_Item
            //for (c=1; c<=N_Text_Channels; c++)
            if (sacd_text_idx == 0) // N_Text_Channels
            {

                LOG(lm_main, LOG_NOTICE, ("scarletbook_read: read_area_toc() ; area[%d]",area_idx));
                print_hex_dump(LOG_NOTICE, "SACDTTxt:", 64, 1, p, 128, 1);                
                //for (tno=1; tno<=N_Tracks; tno++)
                for (i = 0; i < area_toc->track_count; i++)
                {
                    //Track_Text_Item_Ptr[c][tno] , 2 bytes


                    Track_Text_Item_Ptr* area_text;
                    uint8_t        track_type, track_items;
                    char           *track_ptr;
                    area_text = area->area_text = (Track_Text_Item_Ptr*) p;  // Track_Text_Item_Ptr[c][tno] , 2 bytes 
                    SWAP16(area_text->track_text_position[i]);  // Track_Text_Item_Ptr[c][tno] , 2 bytes 
                    
                    LOG(lm_main, LOG_NOTICE, ("scarletbook_read: read_area_toc() ; area_text->track_text_position, Track_Text_Item_Ptr[0][%d] =%d",i,area_text->track_text_position[i]));
                    if (area_text->track_text_position[i] > 0)
                    {
                        track_ptr = (char *) (p + area_text->track_text_position[i]);
                        //DEBUG
                        print_hex_dump(LOG_NOTICE, "N_Items:", 64, 1, track_ptr, 64, 1);
                        track_items = *track_ptr; //N_Items[c][tno], 1 byte, values 1..10
                        LOG(lm_main, LOG_NOTICE, ("scarletbook_read: read_area_toc() ; (track_items), N_Items[0][%d] =%d",i,track_items));

                        track_ptr += 4;    // ---> Text_Item[c][tno][item], format TOC_Text
                        for (j = 0; j < track_items; j++)  // for (item=1; item<=N_Items[c][tno]; item++)
                        {   // Text_Item[c][tno][item] , format is TOC_Text {Text_Type 1byte, Padding1 1 byte, Sp_String, Padding2 0..3} total lenght = 4 byte
                            track_type = *track_ptr;  //  Text_Type , 1 byte
                            track_ptr++;              // Padding1 , 1 byte, must allways have value = 0x20 !!!!
                            // check to see if it is 0x20
                            if(*track_ptr != 0x20)
                            {
                                fwprintf(stdout, L"\n\n Error: Padding1 has not 0x20 value !!\n");
                                LOG(lm_main, LOG_ERROR, ("Error in scarletbook_read: Padding1 has not 0x20 value !!"));   
                            }

                            track_ptr++;              // Sp_String ; skip unknown 0x20, after SP_String follows Padding2 with variable lenght 0..3 bytesl;  value 0x00
                            /*
                                All Characters in a Special_String must belong to one and the same character set.
                                Sp_String or Special_String is a sequence of Special_Char one or two byte characters, terminated by a zero Special_Char.
                            */
                            if (*track_ptr != 0)
                            {
                                int track_text_len=strlen(track_ptr);
                                if (track_text_len > 1024)
                                {
                                    fwprintf(stdout, L"\n\n Warning: The lenght of track text is bigger than 1024!!; area_idx=%d; track_type=0x%02x; track number=%d", area_idx, track_type, i + 1);
                                    LOG(lm_main, LOG_NOTICE, ("Warning in scarletbook_read: The lenght of track text is bigger than 1024 !!; area_idx=%d; track_type=0x%02x; track number=%d\n", area_idx, track_type, i + 1));
                                    track_text_len = 1024; //break;
                                }

                                // check if exists illegal char code  
                                // [e.g Savall - The Celtic Viol - La Viole Celtique - The Treble Viol- AVSA9865 - has incorrect char code = 0x19 in text of Composer in tracks 5,6,7,15,21 !!]
                                char * track_ptr1 = track_ptr;
                                for(int te=0; te < (int)track_text_len; te++)
                                {
                                    if(*(uint8_t*)track_ptr1 < (uint8_t)0x20)
                                    {
                                        *((uint8_t *)track_ptr1) = (uint8_t)0x20;
                                        fwprintf(stdout, L"\n\n Warning: Illegal char in the track text! Corrected.; area_idx=%d; track_type=0x%02x; track number=%d\n", area_idx, track_type, i + 1);
                                        LOG(lm_main, LOG_NOTICE, ("Warning: Illegal char in the track text! Corrected. ;area_idx=%d; track_type=0x%02x; track number=%d", area_idx, track_type, i + 1));
                                    }                                      
                                    track_ptr1 ++;                                         
                                }
                                //DEBUG
                                //LOG(lm_main, LOG_NOTICE, ("Notice: track_ptr:%s; strlen()=%d; track_type=0x%02x; track number=%d", track_ptr, track_text_len, track_type, i + 1));

                                char *track_text_converted_ptr = charset_convert(track_ptr, track_text_len, current_charset, "UTF-8");
                                if(track_text_converted_ptr == NULL || strlen(track_text_converted_ptr)==0)
                                {
                                    fwprintf(stdout, L"\n\n Error: Cannot convert to UTF8 the track text!!; track_type=0x%02x; track number=%d", track_type, i + 1);
                                    LOG(lm_main, LOG_ERROR, ("Error: Cannot convert to UTF8 the track text!!; track_type=0x%02x; track number=%d", track_type, i+1));
                                }
                                //DEBUG
                                LOG(lm_main, LOG_NOTICE, ("Notice: track_text_converted_ptr --> UTF8:%s; strlen()=%d; track_type=0x%02x; track number=%d", track_text_converted_ptr, (int)strlen(track_text_converted_ptr) ,track_type, i + 1));

                                switch (track_type)
                                {
                                    case TRACK_TYPE_TITLE:
                                        area->area_track_text[i].track_type_title = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_PERFORMER:
                                        area->area_track_text[i].track_type_performer = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_SONGWRITER:
                                        area->area_track_text[i].track_type_songwriter = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_COMPOSER:
                                        area->area_track_text[i].track_type_composer = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_ARRANGER:
                                        area->area_track_text[i].track_type_arranger = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_MESSAGE:
                                        area->area_track_text[i].track_type_message = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_EXTRA_MESSAGE:
                                        area->area_track_text[i].track_type_extra_message = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_COPYRIGHT:
                                        area->area_track_text[i].track_type_copyright = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_TITLE_PHONETIC:
                                        area->area_track_text[i].track_type_title_phonetic = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_PERFORMER_PHONETIC:
                                        area->area_track_text[i].track_type_performer_phonetic = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_SONGWRITER_PHONETIC:
                                        area->area_track_text[i].track_type_songwriter_phonetic = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_COMPOSER_PHONETIC:
                                        area->area_track_text[i].track_type_composer_phonetic = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_ARRANGER_PHONETIC:
                                        area->area_track_text[i].track_type_arranger_phonetic = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_MESSAGE_PHONETIC:
                                        area->area_track_text[i].track_type_message_phonetic = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_EXTRA_MESSAGE_PHONETIC:
                                        area->area_track_text[i].track_type_extra_message_phonetic = track_text_converted_ptr;
                                        break;
                                    case TRACK_TYPE_COPYRIGHT_PHONETIC:
                                        area->area_track_text[i].track_type_copyright_phonetic = track_text_converted_ptr;
                                        break;                                     
                                    default:
                                        fwprintf(stdout, L"\n\n Notice: Unknown track text type!!\n");
                                        LOG(lm_main, LOG_NOTICE, ("Notice : scarletbook_read_area_toc(), Unknown track text type: 0x%02x", track_type));
                                        break;
                                }
                            }
                            if (j < track_items - 1)
                            {
                                while (*track_ptr != 0) 
                                    track_ptr++;

                                while (*track_ptr == 0) // Padding2, 0..3 bytes, value=0x00
                                    track_ptr++;
                            }
                        } //for (j = 0; j < track_items
                    } //if (area_text->track_text_position[i] > 0)
                } // for (i = 0; i < area_toc->track_count;
            } // end if (sacd_text_idx == 0) // N_Text_Channels
            sacd_text_idx++;
            p += SACD_LSN_SIZE;total_sectors_read +=1;
            if(total_sectors_read >= area_toc->size)break;
        }
        else        
        {
            break;
        }
    }

    if(Track_List_1_is_present ==0 || Track_List_2_is_present == 0 || ISRC_and_Genre_List_is_present == 0 )
    {
        fwprintf(stdout, L"\n\n Error: Track_List_1 or Track_List_2 or ISRC_and_Genre_List are not detected !!!\n");
        LOG(lm_main, LOG_ERROR, ("Error: Warning: Track_List_1 or Track_List_2 or ISRC_and_Genre_List are not detected !!!"));
        return 0;
    }


    return 1;
}

void scarletbook_frame_init(scarletbook_handle_t *handle)
{
    //handle->packet_info_idx = 0;
    handle->frame_info_idx = 0;

    handle->frame.size = 0;
    handle->frame.started = 0;
    handle->frame.sector_count = 0;
    handle->frame.channel_count = 0;
    handle->frame.dst_encoded = 0;

    handle->frame.timecode.minutes = (uint8_t)0;
    handle->frame.timecode.seconds = (uint8_t)0;
    handle->frame.timecode.frames = (uint8_t)0;

    memset(&handle->audio_sector, 0, sizeof(audio_sector_t));
}

static inline int get_channel_count(audio_frame_info_t *frame_info)
{
    if (frame_info->channel_bit_2 == 1 && frame_info->channel_bit_3 == 0)
    {
        return 6;
    }
    else if (frame_info->channel_bit_2 == 0 && frame_info->channel_bit_3 == 1)
    {
        return 5;
    }
    else
    {
        return 2;
    }
}

static inline void exec_read_callback(scarletbook_handle_t *handle, frame_read_callback_t frame_read_callback, void *userdata)
{
        handle->frame.started = 0;
        frame_read_callback(handle, handle->frame.data, handle->frame.size, userdata);  
}



//       Extract audio frames from blocks (LSN) a.k.a audio sectors and call frame_read_callback if succes
//       return nr of frames proccesed >=0 succes
//              -1 error (has sector bad reads)
//
int scarletbook_process_frames(scarletbook_handle_t *handle, uint8_t *read_buffer, int blocks_read_in, int last_block, frame_read_callback_t frame_read_callback, void *userdata)
{
    int frame_info_idx;
    uint8_t packet_info_idx;
    uint8_t *read_buffer_ptr_blocks = read_buffer;
    uint8_t *read_buffer_ptr;    
    int sector_bad_reads = 0;
    int nr_frames_proccesed=0;
    read_buffer_ptr = read_buffer_ptr_blocks;

    for (int j = 0; j < blocks_read_in; j++)
    {            
        // read Audio Sector Header
        memcpy(&handle->audio_sector.header, read_buffer_ptr, AUDIO_SECTOR_HEADER_SIZE);
        read_buffer_ptr += AUDIO_SECTOR_HEADER_SIZE;

        // read Audio Packet Info Header
#if defined(__BIG_ENDIAN__)
        memcpy(&handle->audio_sector.packet, read_buffer_ptr, AUDIO_PACKET_INFO_SIZE * handle->audio_sector.header.packet_info_count);
        read_buffer_ptr += AUDIO_PACKET_INFO_SIZE * handle->audio_sector.header.packet_info_count;
#else
        // Little Endian systems cannot properly deal with audio_packet_info_t
        {
            for (uint8_t i = 0; i < handle->audio_sector.header.packet_info_count; i++)
            {
                handle->audio_sector.packet[i].frame_start = (read_buffer_ptr[0] >> 7) & 1;
                handle->audio_sector.packet[i].data_type = (read_buffer_ptr[0] >> 3) & 7;
                handle->audio_sector.packet[i].packet_length = (read_buffer_ptr[0] & 7) << 8 | read_buffer_ptr[1];
                read_buffer_ptr += AUDIO_PACKET_INFO_SIZE;
            }
        }
#endif
        //  read Audio Frame Info Header 
        if (handle->audio_sector.header.dst_encoded)
        {
            if (handle->audio_sector.header.frame_info_count > 0)
            {
                memcpy(&handle->audio_sector.frame, read_buffer_ptr, AUDIO_FRAME_INFO_SIZE * handle->audio_sector.header.frame_info_count);
                read_buffer_ptr += AUDIO_FRAME_INFO_SIZE * handle->audio_sector.header.frame_info_count;
            }
        }
        else
        {
            for (uint8_t i = 0; i < handle->audio_sector.header.frame_info_count; i++)
            {
                memcpy(&handle->audio_sector.frame[i], read_buffer_ptr, AUDIO_FRAME_INFO_SIZE - 1);
                read_buffer_ptr += AUDIO_FRAME_INFO_SIZE - 1;
            }
        }

        if(handle->audio_sector.header.packet_info_count > (uint8_t)7)  // max 7 packets must contain an audio sector
        {
            sector_bad_reads = 1;
            handle->frame.started = 0;

            fwprintf(stdout, L"\n ERROR: scarletbook_process_frames(), > Max 7 packets!!\n");
            LOG(lm_main, LOG_ERROR, ("Error : scarletbook_process_frames(). > Max 7 packets!!, handle->audio_sector.header.packet_info_count:%d", handle->audio_sector.header.packet_info_count));
        }
        
        handle->frame_info_idx = 0;
        frame_info_idx = 0;
        for (packet_info_idx = 0; packet_info_idx < handle->audio_sector.header.packet_info_count; packet_info_idx++) //&& (sector_bad_reads == 0)
        {
            audio_packet_info_t* packet = &handle->audio_sector.packet[packet_info_idx];
            if(packet->packet_length > MAX_PACKET_SIZE)
            {
                sector_bad_reads = 1;
                continue;
            }
            switch (packet->data_type) 
            {
                case DATA_TYPE_AUDIO:
                    if (packet->frame_start)
                    {
                        // If frame is already started 
                        // try to save the entire previous audio frame
                        // checks if we have a completed frame
                        if (handle->frame.started){
                            if (handle->frame.size > 0){
                                if ((handle->frame.dst_encoded && handle->frame.sector_count == 0) ||
                                    (!handle->frame.dst_encoded && handle->frame.size == handle->frame.channel_count * FRAME_SIZE_64))
                                {
                                    exec_read_callback(handle, frame_read_callback, userdata);
                                    nr_frames_proccesed ++;                                  
                                } 
                            }
                        }
                        //check if timecode is consecutive (didn't miss a frame)
                        uint32_t frametimecode_prev=TIME_FRAMECOUNT(&handle->frame.timecode);
                        uint32_t frametimecode_current =TIME_FRAMECOUNT(&handle->audio_sector.frame[frame_info_idx].timecode);
                       
                        if (frametimecode_prev > 0)
                        {
                            // check if is consecutive
                            if(frametimecode_current != frametimecode_prev + 1 )
                            {
                                LOG(lm_main, LOG_ERROR, ("Error : scarletbook_process_frames(), frametimecode not succesive! frametimecode_current:%u, frametimecode_prev:%u", frametimecode_current, frametimecode_prev));
                            }
                        }                       

                        handle->frame.size = 0;
                        handle->frame.dst_encoded = handle->audio_sector.header.dst_encoded;
                        handle->frame.sector_count = handle->audio_sector.frame[frame_info_idx].sector_count;
                        handle->frame.channel_count = get_channel_count(&handle->audio_sector.frame[frame_info_idx]);
                        handle->frame.started = 1;
                        handle->frame.timecode.minutes = handle->audio_sector.frame[frame_info_idx].timecode.minutes;
                        handle->frame.timecode.seconds = handle->audio_sector.frame[frame_info_idx].timecode.seconds;
                        handle->frame.timecode.frames = handle->audio_sector.frame[frame_info_idx].timecode.frames;
                        handle->frame_info_idx = frame_info_idx;

                        // advance frame_info_idx
                        frame_info_idx++;
                    }
                    if (handle->frame.started)
                    {
                        if (handle->frame.size + packet->packet_length <= MAX_DST_SIZE)
                        {
                            memcpy(handle->frame.data + handle->frame.size, read_buffer_ptr, packet->packet_length);
                            handle->frame.size += packet->packet_length;
                            if (handle->frame.dst_encoded)
                            {
                                handle->frame.sector_count--;
                            }
                        }
                        else
                        {
                            sector_bad_reads = 1;
                            // buffer overflow error, try next frame..
                            handle->frame.started = 0;

                            fwprintf(stdout, L"\n ERROR: scarletbook_process_frames(), buffer overflow error in blocks_read:%d\n", j);
                            LOG(lm_main, LOG_ERROR, ("Error : scarletbook_process_frames(), buffer overflow error. in blocks_read:%d", j));                                                      
                        }
                    }
                    break;
                case DATA_TYPE_SUPPLEMENTARY:
                case DATA_TYPE_PADDING:
                    break;
                default:
                    break;
            }  // switch (packet->data_type)
            
            //if(sector_bad_reads > 0)
            //  break;
            // advance the source pointer
            read_buffer_ptr += packet->packet_length;

        } // end for  packet_info_idx

        // obtain the next sector data block
        read_buffer_ptr_blocks += SACD_LSN_SIZE;
        read_buffer_ptr = read_buffer_ptr_blocks;

    } // end for j   // while(blocks_read--)

    if (last_block )
    {
        // If frame is already started
        // try to save the entire last audio frame
        // checks if we have a completed audio frame
        
        if (handle->frame.started)
        {
            if (handle->frame.size > 0){
                if ((handle->frame.dst_encoded && handle->frame.sector_count == 0) ||
                    (!handle->frame.dst_encoded && handle->frame.size == handle->frame.channel_count * FRAME_SIZE_64))
                {
                    exec_read_callback(handle, frame_read_callback, userdata);
                    nr_frames_proccesed++;
                    
                }
            }  
        }
    }


    if (sector_bad_reads > 0)
        return -1;  
    else
        return nr_frames_proccesed;
}


