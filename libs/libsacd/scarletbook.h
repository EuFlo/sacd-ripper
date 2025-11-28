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

#ifndef SCARLETBOOK_H_INCLUDED
#define SCARLETBOOK_H_INCLUDED

#include <inttypes.h>
#include <list.h>

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__) &&  !defined(__MINGW32__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED    __attribute__ ((packed))
#define PRAGMA_PACK         0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK    1
#endif

/**
 * reversing TODO:
 *  - SACD_Ind (index list)
 *  - SACDRTOC (revocation toc)
 *  - SACD_WLL (track weblink list)
 *  - SACDPLAY (set of playlists)
 */

/**
 * The length of one Logical Block of an SACD.
 */
#define SACD_LSN_SIZE                  2048
#define SACD_SAMPLING_FREQUENCY        2822400
#define SACD_FRAME_RATE                75

#define START_OF_FILE_SYSTEM_AREA      0
#define START_OF_MASTER_TOC            510
#define MASTER_TOC_LEN                 10
#define MAX_AREA_TOC_SIZE_LSN          96
#define MAX_LANGUAGE_COUNT             8
#define MAX_CHANNEL_COUNT              6
#define MAX_DST_SIZE                   (1024 * 64)
#define SAMPLES_PER_FRAME              588
#define FRAME_SIZE_64                  4704    // (SAMPLES_PER_FRAME * 64 / 8)   // = 4704 bytes for fs=2822400 for uncompressed frame
#define MAX_PACKET_SIZE                2045
#define SUPPORTED_VERSION_MAJOR        1
#define SUPPORTED_VERSION_MINOR        20

#define MAX_GENRE_COUNT                30 // was 29
#define MAX_CATEGORY_COUNT             3

#define MAX_PROCESSING_BLOCK_SIZE      512

enum frame_format_t
{
    FRAME_FORMAT_DST = 0, // DST coded. Flexible format
    // = 1 ; Reserved
    FRAME_FORMAT_DSD_3_IN_14 = 2,  // Fixed format. 2-Channel Stereo, Plain DSD, 3 Frames in 14 Sectors.
    FRAME_FORMAT_DSD_3_IN_16 = 3   // Fixed format. 2-Channel Stereo, Plain DSD, 3 Frames in 16 Sectors
    //  = 4..15 ; Reserved for future use
};

enum character_set_t                // Character_Set_Code
{
    CHAR_SET_UNKNOWN       = 0,     // Not used
    CHAR_SET_ISO646        = 1,    // ISO 646 International Reference Version (IRV),escape sequences are not allowed, 1 byte
    CHAR_SET_ISO8859_1     = 2,    // ISO 8859-1, no escape sequences allowed, 1 byte
    CHAR_SET_RIS506        = 3,    // RIS 506; 2 bytes pe character; MusicShiftJIS, per RIS-506 (RIAJ), Music Shift-JIS Kanji
    CHAR_SET_KSC5601       = 4,    // Korean KSC 5601-1989[149]; Korean KSC 5601-1987; 2 bytes
    CHAR_SET_GB2312        = 5,    // Chinese GB 2312-80[58]; 2 bytes;Chinese GB 2312-80
    CHAR_SET_BIG5          = 6,    // Big5, 2 bytes
    CHAR_SET_ISO8859_1_ESC = 7     // ISO 8859-1, escape sequences to single byte character sets are allowed
     // 8..255 reseved for future use
};

// string representation for character sets
extern const char *character_set[];

extern char *album_genre[];

// General Genre Table
enum genre_t {
	GENRE_NOT_USED               = 0,       // 12
	GENRE_NOT_DEFINED            = 1,       // 12
	GENRE_ADULT_CONTEMPORARY     = 2,       // 12
	GENRE_ALTERNATIVE_ROCK       = 3,       // 40
	GENRE_CHILDRENS_MUSIC        = 4,       // 12
	GENRE_CLASSICAL              = 5,       // 32
	GENRE_CONTEMPORARY_CHRISTIAN = 6,       // 140
	GENRE_COUNTRY                = 7,       // 2
	GENRE_DANCE                  = 8,       // 3
	GENRE_EASY_LISTENING         = 9,       // 98
	GENRE_EROTIC                 = 10,      // 12
	GENRE_FOLK                   = 11,      // 80
	GENRE_GOSPEL                 = 12,      // 38
	GENRE_HIP_HOP                = 13,      // 7
	GENRE_JAZZ                   = 14,      // 8
	GENRE_LATIN                  = 15,      // 86
	GENRE_MUSICAL                = 16,      // 77
	GENRE_NEW_AGE                = 17,      // 10
	GENRE_OPERA                  = 18,      // 103
	GENRE_OPERETTA               = 19,      // 104
	GENRE_POP_MUSIC              = 20,      // 13
	GENRE_RAP                    = 21,      // 15
	GENRE_REGGAE                 = 22,      // 16
	GENRE_ROCK_MUSIC             = 23,      // 17
	GENRE_RHYTHM_AND_BLUES       = 24,      // 14
	GENRE_SOUND_EFFECTS          = 25,      // 37
	GENRE_SOUND_TRACK            = 26,      // 24
	GENRE_SPOKEN_WORD            = 27,      // 101
	GENRE_WORLD_MUSIC            = 28,      // 12
	GENRE_BLUES                  = 29       // 0
};

//Genre_Table, 1 byte , Uint8 ; identifies the table used by Genre_Index ; 0= Not Used, 1=General Genre Table, 2=Japanese Genre Table. See RIS504, 3..255 Reserved for future standardization ;
enum category_t {
	CATEGORY_NOT_USED = 0,
	CATEGORY_GENERAL  = 1,
	CATEGORY_JAPANESE = 2
};

extern const char *album_category[];

/*

0   Reserved
1   Title
2   Name(s) of the performer(s)
3   Name(s) of the songwriter(s)
4   Name(s) of the composer(s)
5   Name(s) of the arranger(s)
6   Message(s) from the content provider and/or the artist
7   Extra message(s)
8   Copyright
9..128   Reserved
129  Title, phonetic text
130  Name(s) of the performer(s), phonetic text
131  Name(s) of the songwriter(s), phonetic text
132  Name(s) of the composer(s), phonetic text
133  Name(s) of the arranger(s), phonetic text
134  Message(s) from the content provider and/or the artist, phonetic text
135  Extra message(s), phonetic text
136  Copyright, phonetic text
137..255  Reserved

*/

enum track_type_t {
	TRACK_TYPE_TITLE                  = 0x01,
	TRACK_TYPE_PERFORMER              = 0x02,
	TRACK_TYPE_SONGWRITER             = 0x03,
	TRACK_TYPE_COMPOSER               = 0x04,
	TRACK_TYPE_ARRANGER               = 0x05,
	TRACK_TYPE_MESSAGE                = 0x06,
	TRACK_TYPE_EXTRA_MESSAGE          = 0x07,
    TRACK_TYPE_COPYRIGHT              = 0x08,

	TRACK_TYPE_TITLE_PHONETIC         = 0x81,
	TRACK_TYPE_PERFORMER_PHONETIC     = 0x82,
	TRACK_TYPE_SONGWRITER_PHONETIC    = 0x83,
	TRACK_TYPE_COMPOSER_PHONETIC      = 0x84,
	TRACK_TYPE_ARRANGER_PHONETIC      = 0x85,
	TRACK_TYPE_MESSAGE_PHONETIC       = 0x86,
    TRACK_TYPE_EXTRA_MESSAGE_PHONETIC = 0x87,
    TRACK_TYPE_COPYRIGHT_PHONETIC     = 0x88
};

#if PRAGMA_PACK
#pragma pack(1)
#endif

/**
 * Common
 *
 * The following structures are used in both the Master and area TOCs.
 */

/**
 * Genre Information.
 *  Genre_Code()
 * {
        Genre_Table, 1 byte , Uint8 ; identifies the table used by Genre_Index ; 0= Not Used, 1=General Genre Table, 2=Japanese Genre Table. See RIS504, 3..255 Reserved for future standardization ; 
        Reserved  , 1 byte , Uint8, value =0
        Genre_Index , 2 bytes , Uint16
    }
 */
typedef struct
{
    uint8_t  category;                        // category_t  ; Genre_Table
    //uint16_t reserved;                        // BUG !!!!!!!!! must be uint8_t  not uint16_t 
    uint8_t reserved; 
    //uint8_t  genre;                           // genre_t  ; Genre_Index 2 bytes ; BUG  !! must be uint16_t
    uint16_t  genre; 
}
ATTRIBUTE_PACKED genre_table_t;

/**
 * Language & character set
 * Language_Code[c] must contain the ISO 639 Language Code that is used with Text Channel c. All
 *  text in Text Channel c must be according to this Language Code. The value $0000 is not allowed for Language_Code[c].
 */
typedef struct
{
    char    language_code[2];                 // Language_Code, 2 bytes, String, :  "en" (ISO-639-1)
    uint8_t character_set;                    // Character_Set_Code,1 byte, value = 1..7;  (ISO 646),  2 (ISO 8859-1)
    uint8_t reserved;
}
ATTRIBUTE_PACKED locale_table_t;

/**
 * Master TOC
 *
 * The following structures are needed for Master TOC information.
 */
typedef struct
{
// ---------------------------------- Master_TOC_0  ----------------------------------------------------------------------
    // -----------------------------------M_TOC_0_Header , 16 bytes----------------------------------------
    char           id[8];                     // SACDMTOC; Master_TOC_Signature
    struct
    {
        uint8_t major;
        uint8_t minor;
    } ATTRIBUTE_PACKED version;               //Spec_Version   1.20 / 0x0114  
    uint8_t        reserved01[6];
    // ----------------------------------------END of M_TOC_0_Header , 16 bytes--------------------------------------
    // -----------------------------------------Album_Info , 48 bytes -----------------------------------------------
    uint16_t       album_set_size;          // Album_Set_Size,2bytes,  1..65535
    uint16_t       album_sequence_number;   // Album_Sequence_Number
    uint8_t        reserved02[4];
    char           album_catalog_number[16];  // Album_Catalog_Number, 16 bytes, String; 0x00 when empty, else padded with spaces for shorter strings
    genre_table_t  album_genre[4];            // Album_Genre, 4x4 bytes;
    uint8_t        reserved03[8];
    // -------------------------------------- END of Album_Info
    // /*** Album text fields are in Master_Text[c] with c=1..8 ***/
    // ----------------------------------------Disc_Info , 64 bytes -------------------------------------------------------
    uint32_t       area_1_toc_1_start;   /*LSN for AREA_TOC_1 of 2 channel */  // 2CH_TOC_1_Address, 4bytes, Uint32, values 0, 544
    uint32_t       area_1_toc_2_start;   /*LSN for AREA_TOC_2 of 2 channel */  // 2CH_TOC_2_Address, 4bytes, Uint32
    uint32_t       area_2_toc_1_start;   /*LSN for AREA_TOC_1 of M channel */  // MC_TOC_1_Address, 4bytes, Uint32
    uint32_t       area_2_toc_2_start;   /*LSN for AREA_TOC_2 of M channel */  // MC_TOC_2_Address, 4bytes, Uint32
#if defined(__BIG_ENDIAN__)
    uint8_t        disc_type_hybrid     : 1;        // Disc_Flags, 1 byte
    uint8_t        disc_type_reserved   : 7;        // Hybr, b7
#else
    uint8_t        disc_type_reserved   : 7;        // Disc_Flags, 1 byte
    uint8_t        disc_type_hybrid     : 1;        // Hybr, b7
#endif
    uint8_t        reserved04[3];
    uint16_t       area_1_toc_size;  /*Length in Sectors of AREA_TOC of  2ch */ // 2CH_TOC_Length, 2byte, Uint16, value 0, 5..
    uint16_t       area_2_toc_size;  /*Length in Sectors of AREA_TOC of M channel  */  // MC_TOC_Length, 2byte, Uint16, value 0, 37..
    char           disc_catalog_number[16];   // Disc_Catalog_Number, 16 bytes, String; 0x00 when empty, else padded with spaces for shorter strings
    genre_table_t  disc_genre[4];       // Disc_Genre, 4x4 bytes
    uint16_t       disc_date_year;      // Disc_Date , 4 bytes
    uint8_t        disc_date_month;
    uint8_t        disc_date_day;
    uint8_t        reserved05[4];
    // --------------------------------------end of Disc_Info, 64 bytes ---------------------------------------
    /*** Disc text fields are in Master_Text[c] with c=1..8 ***/
    // -------------------------------------Text_Channels, 40 bytes --------------------------------------------
    uint8_t        text_area_count;       // N_Text_Channels, 1 byte, Uint8  , values =0..8
    uint8_t        reserved06[7];           // value 0
    locale_table_t locales[MAX_LANGUAGE_COUNT]; // N_Text_Channels values= 0...8
}
ATTRIBUTE_PACKED master_toc_t;
// ---------------------------------------------- END Text_Channels--------------------------------------------
//------------------------------------Disc_WebLink_Info , 128 bytes---------------------------------------- v2.0
// -------------------------------------Disc_Info_2, 64 bytes ----------------------------------------------v2.0
// ------------------------------------reserved  until 2048-------------------------------------------------
// ----------------------------------END of  Master_TOC_0 , 2048 bytes ----------------------------------------------------------------------

/**
 * Master Album Information
 * -----------------------------------------------Master_Text()[c] -------------------------------------------------
 */
typedef struct
{
    char     id[8];                           // SACDText; Master_Text_Signature, 8 bytes, String
    uint8_t  reserved[8];
    uint16_t album_title_position;  // Album_Title_Ptr, if (Album_Title_Ptr!=0) { Album_Title Special_String); value=64
    uint16_t album_artist_position; // Album_Artist_Ptr
    uint16_t album_publisher_position; // Album_Publisher_Ptr
    uint16_t album_copyright_position;  // Album_Copyright_Ptr
    uint16_t album_title_phonetic_position; // Album_Title_Phonetic_Ptr
    uint16_t album_artist_phonetic_position; // Album_Artist_Phonetic_Ptr
    uint16_t album_publisher_phonetic_position; //Album_Publisher_Phonetic_Ptr
    uint16_t album_copyright_phonetic_position; // Album_Copyright_Phonetic_Ptr
    uint16_t disc_title_position;  // Disc_Title_Ptr
    uint16_t disc_artist_position;  // Disc_Artist_Ptr 
    uint16_t disc_publisher_position; // Disc_Publisher_Ptr
    uint16_t disc_copyright_position;  // Disc_Copyright_Ptr
    uint16_t disc_title_phonetic_position; // Disc_Title_Phonetic_Ptr
    uint16_t disc_artist_phonetic_position; // Disc_Artist_Phonetic_Ptr
    uint16_t disc_publisher_phonetic_position; //Disc_Publisher_Phonetic_Ptr
    uint16_t disc_copyright_phonetic_position; // Disc_Copyright_Phonetic_Ptr
    uint8_t  data[2000];  // reserved until 2048 bytes
}
ATTRIBUTE_PACKED master_sacd_text_t;

typedef struct
{
    char *album_title;
    char *album_title_phonetic;
    char *album_artist;
    char *album_artist_phonetic;
    char *album_publisher;
    char *album_publisher_phonetic;
    char *album_copyright;
    char *album_copyright_phonetic;
    char *disc_title;
    char *disc_title_phonetic;
    char *disc_artist;
    char *disc_artist_phonetic;
    char *disc_publisher;
    char *disc_publisher_phonetic;
    char *disc_copyright;
    char *disc_copyright_phonetic;
} 
master_text_t;

/**
 * Manuf_Info
 */
typedef struct
{
    char    id[8];                          // 'SACD_Man', manufacturer information
    uint8_t information[2040];              //  Information, The content and the format of the data in the Information field is decided by the disc manufacturer. 
                                            // If manufacturer information is not stored in this Sector, all bytes in the Information field must be set to zero
}
ATTRIBUTE_PACKED master_man_t;

/**
 * Area TOC,   Area_TOC, 2048 bytes, 1 sector, LSN
 *
 * The following structures are needed for Area TOC information.
 *
 */
typedef struct
{
    // ---------------- A_TOC_0_Header , 16 bytes-----------------------------------------------------------------------------------------
    char           id[8];                     // Area_TOC_Signature : TWOCHTOC or MULCHTOC
    struct
    {
        uint8_t major;
        uint8_t minor;
    } ATTRIBUTE_PACKED version;               // Spec_Version  : 1.20 / 0x0114
    uint16_t       size;                      // Area_TOC_Length :  5..40 (total size of TOC); length of the Area_TOC in Sectors
    uint8_t        reserved01[4];
    // --------------------------------------- END of HEADER ; 16 bytes--------------------------------------------------------------------------------
    // -----------------------------------------Area_Data, 112 bytes  ---------------------------------------------------------------------------------
    /*up is for A_TOC_0_Header */
    uint32_t       max_byte_rate;              /* Max_Byte_Rate; Max Average Byte Rate of Multiplexed Frames*/
    uint8_t        sample_frequency;          //  FS_Code ; 0x04 = (64 * 44.1 kHz) (physically there can be no other values, or..? :)
#if defined(__BIG_ENDIAN__)
    uint8_t        reserved02   : 4;           //     Area_Flags   1 byte;
    uint8_t        frame_format : 4;
#else
    uint8_t        frame_format : 4;            //     Area_Flags  1 byte;
    uint8_t        reserved02   : 4;
#endif
    uint8_t        reserved03[10];
    uint8_t        channel_count;           // uchar N_channels; /* the num of audio channels for each frame */ ;N_Channels , 1 byte; Uint8 values = 2,5,6
#if defined(__BIG_ENDIAN__)
    uint8_t        loudspeaker_config : 5;  //    Loudspeker-config b4...b0   
    uint8_t        extra_settings : 3;      // Area_Config, 1 byte:  Extra_Settings  b7...b5  
#else
    uint8_t        extra_settings : 3;      // Area_Config, 1 byte:  Extra_Settings  b7...b5
    uint8_t        loudspeaker_config : 5;  //    Loudspeker-config b4...b0
#endif
    uint8_t        max_available_channels;    //uchar max_ok_channels; Max_Available_Channels 0..6
    uint8_t        area_mute_flags;   // Area_Mute_Flags, 1 byte
    uint8_t        reserved04[12];
#if defined(__BIG_ENDIAN__)
    uint8_t        reserved05 : 4;    // uchar area_copy_mng; Area_Copy_Management, 1 byte
    uint8_t        track_attribute : 4;
#else
    uint8_t        track_attribute : 4; // Area_Copy_Management, 1 byte ; Track_Atribute b3...b0
    uint8_t        reserved05 : 4;
#endif
    uint8_t        reserved06[15];
    struct                                  // Total_Area_Play_Time  ; uchar total_play_time[3];
    {
        uint8_t minutes;        // 0...255
        uint8_t seconds;        // 0...59
        uint8_t frames;         // 0...74
    } ATTRIBUTE_PACKED total_playtime;
    uint8_t        reserved07;
    uint8_t        track_offset;    /* track offset of a disc is from an album */  // Track_Offset, 1 byte; 0...255
    uint8_t        track_count;    /*The tracks number in the current audio area*/ // N_Tracks 1 byte; 1...255
    uint8_t        reserved08[2];  // First_Bonus_Track_Number, 1 byte; reserverd 1 byte
    uint32_t       track_start;     /* uint track_start_addr;   */ // Track_Area_Start_Address, 4byte; Uint32
    uint32_t       track_end;       /* uint track_end_addr;   */  // Track_Area_End_Address  4byte; Uint32
    
            //-----------------------------Text_Channels , 40 bytes-------------------------------------------------------------
    uint8_t        text_area_count;  // N_Text_Channels, 1 byte, value=0..8; offset =80 (0x50)
    uint8_t        reserved09[7];
                                    // for (c=1; c<s=N_Text_Channels; c++)
    locale_table_t languages[MAX_LANGUAGE_COUNT];
    uint8_t        reserved091[8];        
                                    // reserved 32-4*N_Text_Channels x 1 bytes; Uint8, value =0
            // ---------------------------END of Text_Channel, 40 bytes --------------------------------------------------------
                                    // Reserved until 128 bytes, value 0
    // ----------------------------------------END of Area Data , 112 bytes----------------------------------------------------------------------------
          /*up is for Area_Data*/  
    // ---------------------------------------List_Pointers , 16 bytes------------------------------------------------------------------------------------------
    uint16_t       track_text_offset; // Track_Text_Ptr , 2bytes, Uint16, values 0,5,37
 
    uint16_t       index_list_offset;  // Index_List_Ptr,  2bytes, Uint16
    uint16_t       access_list_offset; // Access_List_Ptr,  2bytes, Uint16
    uint8_t        reserved10[10];     // char reserved7[8];  // Track_WebLink_List_Ptr ,Track_List_3_Ptr ,Set_Of_PlayLists_Ptr; 3 x 2bytes, Uint16; reserverd 4 bytes, value=0
    /*up is for List_pointer */
    //--------------------------------------END List_Pointers ; 16 bytes------------------------------------------------------------------
    //---------------------------------------Area_Text ; 1904 bytes; Area_TOC_0 must always contain Area_Text., c=number of channels--------------------------------------------
    //                                              for (c=1; c<=N_Text_Channels; c++)
    uint16_t       area_description_offset;   // Area_Description_Ptr[c], 2bytes, Uint16
    uint16_t       copyright_offset;            //Area_Copyright_Ptr[c], 2bytes, Uint16
    uint16_t       area_description_phonetic_offset; //Area_Description_Phonetic_Ptr[c], 2bytes, Uint16
    uint16_t       copyright_phonetic_offset; //  Area_Copyright_Phonetic_Ptr[c], 2bytes, Uint16
                                                // reserved until 208 
                                                // for (c=1; c<=N_Text_Channels; c++) 
                                                // {  if (Area_Description_Ptr[c]!=0)
    uint8_t        data[1896];    // area_text[1904]
    //--------------------------------------END of Area_Text ; 1904 bytes------------------------------------------------------------------------------------
}
ATTRIBUTE_PACKED area_toc_t;

typedef struct
{
    char *track_type_title;
    char *track_type_performer;
    char *track_type_songwriter;
    char *track_type_composer;
    char *track_type_arranger;
    char *track_type_message;
    char *track_type_extra_message;
    char *track_type_copyright;
    char *track_type_title_phonetic;
    char *track_type_performer_phonetic;
    char *track_type_songwriter_phonetic;
    char *track_type_composer_phonetic;
    char *track_type_arranger_phonetic;
    char *track_type_message_phonetic;
    char *track_type_extra_message_phonetic;
    char *track_type_copyright_phonetic;
} 
area_track_text_t;

typedef struct
{
    char     id[8];                           // Track_Text_Signature  ; SACDTTxt, Track Text
    uint16_t track_text_position[];   // Track_Text_Item_Ptr[c][tno] , 2 bytes
}
ATTRIBUTE_PACKED Track_Text_Item_Ptr;

typedef struct
{
    char country_code[2];
    char owner_code[3];
    char recording_year[2];
    char designation_code[5];
}
ATTRIBUTE_PACKED isrc_t;

typedef struct
{
    char          id[8];                      // SACD_IGL, ISRC and Genre List
    isrc_t        isrc[255];
    uint32_t      reserved;
    genre_table_t track_genre[255];
}
ATTRIBUTE_PACKED area_isrc_genre_t;

typedef struct
{
    char        id[8];                            // SACD_ACC, Access List
    uint16_t    entry_count;
    uint8_t     main_step_size;
    uint8_t     reserved01[5];
    uint8_t     main_access_list[6550][5];
    uint8_t     reserved02[2];
    uint8_t     detailed_access_list[32768];
}
ATTRIBUTE_PACKED area_access_list_t;

typedef struct
{
    char     id[8];                           // SACDTRL1
    uint32_t track_start_lsn[255];
    uint32_t track_length_lsn[255];
}
ATTRIBUTE_PACKED area_tracklist_offset_t;

// typedef struct
// {
//     uint8_t minutes;
//     uint8_t seconds;
//     uint8_t frames;
// #if defined(__BIG_ENDIAN__)
//     uint8_t extra_use : 3;
//     uint8_t reserved : 5;
// #else
//     uint8_t reserved : 5;
//     uint8_t extra_use : 3;
// #endif
// } 
// ATTRIBUTE_PACKED area_tracklist_time_start_t;

typedef struct
{
    uint8_t minutes;
    uint8_t seconds;
    uint8_t frames;
#if defined(__BIG_ENDIAN__)
    uint8_t track_flags_ilp : 1;
    uint8_t track_flags_tmf4 : 1;
    uint8_t track_flags_tmf3 : 1;
    uint8_t track_flags_tmf2 : 1;
    uint8_t track_flags_tmf1 : 1;
    uint8_t reserved : 3;
#else
    uint8_t reserved : 3;
    uint8_t track_flags_tmf1 : 1;
    uint8_t track_flags_tmf2 : 1;
    uint8_t track_flags_tmf3 : 1;
    uint8_t track_flags_tmf4 : 1;
    uint8_t track_flags_ilp : 1;
#endif
}
ATTRIBUTE_PACKED area_tracklist_time_t; // used to be  area_tracklist_time_duration_t but now is common with area_tracklist_time_start_t

#define TIME_FRAMECOUNT(m) ((uint32_t)(m)->minutes * 60 * SACD_FRAME_RATE + (uint32_t)(m)->seconds * SACD_FRAME_RATE + (m)->frames)

typedef struct
{
    char                        id[8];                           // SACDTRL2
    area_tracklist_time_t       start[255];    // used to have type of  area_tracklist_time_start_t
    area_tracklist_time_t       duration[255];  // used to have type of  area_tracklist_time_duration_t
} 
ATTRIBUTE_PACKED area_tracklist_t;

enum audio_packet_data_type_t {
	DATA_TYPE_AUDIO         = 2,
	DATA_TYPE_SUPPLEMENTARY = 3,
	DATA_TYPE_PADDING       = 7
};

// It's no use to make a little & big endian struct. On little 
// endian systems this needs to be filled manually anyway.
typedef struct
{
    uint8_t  frame_start   : 1;
    uint8_t  reserved      : 1;
    uint8_t  data_type     : 3;
    uint16_t packet_length : 11;
} 
ATTRIBUTE_PACKED audio_packet_info_t;
#define AUDIO_PACKET_INFO_SIZE    2U

typedef struct
{
    struct
    {
        uint8_t minutes;
        uint8_t seconds;
        uint8_t frames;
    } ATTRIBUTE_PACKED timecode;

    // Note: the following byte is only filled 
    // on DST encoded audio frames
#if defined(__BIG_ENDIAN__)
    uint8_t channel_bit_1 : 1;
    uint8_t sector_count  : 5;
    uint8_t channel_bit_2 : 1;  // 1 = 6 channels
    uint8_t channel_bit_3 : 1;  // 1 = 5 channels, 0 = Stereo
#else
    uint8_t channel_bit_3 : 1;
    uint8_t channel_bit_2 : 1;
    uint8_t sector_count  : 5;
    uint8_t channel_bit_1 : 1;
#endif
} 
ATTRIBUTE_PACKED audio_frame_info_t;
#define AUDIO_FRAME_INFO_SIZE    4U

typedef struct
{
#if defined(__BIG_ENDIAN__)
    uint8_t packet_info_count : 3; // N_Packets
    uint8_t frame_info_count  : 3; // N_Frame_Starts
    uint8_t reserved          : 1;
    uint8_t dst_encoded       : 1;
#else
    uint8_t dst_encoded       : 1;
    uint8_t reserved          : 1;
    uint8_t frame_info_count  : 3;  // N_Frame_Starts
    uint8_t packet_info_count : 3;  // N_Packets
#endif
}
ATTRIBUTE_PACKED audio_frame_header_t;
#define AUDIO_SECTOR_HEADER_SIZE    1U

typedef struct
{
    audio_frame_header_t    header;
    audio_packet_info_t     packet[7];
    audio_frame_info_t      frame[7];
} 
ATTRIBUTE_PACKED audio_sector_t;

typedef struct  
{
    uint8_t                  * area_data;
    area_toc_t               * area_toc;
    area_tracklist_offset_t  * area_tracklist_offset;
    area_tracklist_t         * area_tracklist_time;
    Track_Text_Item_Ptr      * area_text;
    area_track_text_t          area_track_text[255];                      // max of 255 supported tracks
    area_isrc_genre_t        * area_isrc_genre;

    char                     * description;
    char                     * copyright;
    char                     * description_phonetic;
    char                     * copyright_phonetic;
}
scarletbook_area_t;

typedef struct
{
    uint8_t            *data;     // must be allocated MAX_DST_SIZE  ; (1024 * 64)
    int                 size;
    int                 started;

    int                 sector_count;
    int                 channel_count;

    int                 dst_encoded;

    struct
    {
        uint8_t minutes;
        uint8_t seconds;
        uint8_t frames;
    } ATTRIBUTE_PACKED timecode;

} 
scarletbook_audio_frame_t;

typedef struct
{
    void                     * sacd;                                      // sacd_reader_t

    uint8_t                  * master_data;
    master_toc_t             * master_toc;
    master_man_t             * master_man;
    master_text_t              master_text;

    int                        twoch_area_idx;
    int                        mulch_area_idx;
    int                        area_count;
    scarletbook_area_t         area[4];   // added for backup 2 more areas:  2= TWOCHTOC  TOC-2;    3 =MULCHTOC  TOC-2

    scarletbook_audio_frame_t  frame;
    audio_sector_t             audio_sector;
    
    int                        frame_info_idx;  // added for retrieving timecode of current frame;   e.g. handle->audio_sector.frame[handle->frame_info_idx].timecode
    int                        audio_frame_trimming;    // if No pauses included if 1.  Trimm out audioframes in trimecode interval [area_tracklist_time->start...+duration]
    uint32_t                   count_frames;                              // keep the number of audio frames in a track (for verification)
    int                        dsf_nopad;
    int                        concatenate;
    int                        id3_tag_mode;  // 0=no id3tag inserted; 1=id3v2.3/utf16; 2=miminal id3v2.3/iso8859-1;3=id3v2.3/iso8859-1; 4=id3v2.4/utf8;5=minimal id3v2.4/utf8
    int                        artist_flag;
    int                        performer_flag;
    uint32_t                   total_sectors_iso;
} 
scarletbook_handle_t;

#if PRAGMA_PACK
#pragma pack()
#endif

#endif /* SCARLETBOOK_H_INCLUDED */
