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
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wchar.h>
#include <locale.h>
#include <time.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#if defined(WIN32) || defined(_WIN32)
#include <io.h>
#endif

#include <pthread.h>
#include <charset.h>
#include <logging.h>

#include "getopt.h"
#include "sacd_reader.h"
#include "scarletbook.h"
#include "scarletbook_read.h"
#include "scarletbook_output.h"
#include "scarletbook_print.h"
#include "scarletbook_helpers.h"
#include "scarletbook_id3.h"
#include "cuesheet.h"
#include "endianess.h"
#include "fileutils.h"
#include "utils.h"
#include "yarn.h"
#include "version.h"
#include "scarletbook_xml.h"



static struct opts_s
{
    int            two_channel;
    int            multi_channel;
    int            output_dsf;
    int            output_dsdiff_em;
    int            output_dsdiff;
    int            output_iso;
    int            convert_dst;
    int            export_cue_sheet;
    int            print;
    char           *input_device; /* Access method driver should use for control */
    char           *output_dir_base;
    char           *output_dir_conc_base;
    int            select_tracks;
    uint8_t        selected_tracks[256]; /* scarletbook is limited to 255 tracks */
    int            dsf_nopad;
    int            audio_frame_trimming; // if 1  trimm out audioframes in trimecode interval [area_tracklist_time->start...+duration]
    int            artist_flag;          // if artist ==1 then the artist name is added in folder name
    int            performer_flag;       // if performer ==1 the performer from each track is added
    int            concatenate;  // concatenate consecutive tracks specified in selected_tracks
    int            logging;  // if 1 save logs in a file
    int            id3_tag_mode; // 0=>no id3 inserted; 1=>id3 v2.3 with UTF-16 encoding; 2=>miminal id3v2.3 tag with UTF-16 encoding; 3=>id3v2.3 with ISO_8859_1 encoding; 4=>id3v2.4 with UTF-8 encoding; 5=>id3v2.4 minimal with UTF-8 encoding
    int            version;
    int            concurrent;
} opts;

scarletbook_handle_t *handle;
scarletbook_output_t *output;

/* Parse all options. */
static int parse_options(int argc, char *argv[]) 
{
    int opt; /* used for argument parsing */
    char *program_name = NULL;

    static const char help_text[] =
        "Usage: %s [options] -i[=FILE]\n"
        "Options:\n"
        "  -2, --2ch-tracks                : Export two channel tracks (default)\n"
        "  -m, --mch-tracks                : Export multi-channel tracks\n"
        "  -e, --output-dsdiff-em          : output as Philips DSDIFF (Edit Master) file\n"
        "  -p, --output-dsdiff             : output as Philips DSDIFF file\n"
        "  -s, --output-dsf                : output as Sony DSF file\n"
        "  -z, --dsf-nopad                 : Do not zero pad DSF (cannot be used with -t)\n"
        "  -t, --select-track              : only output selected track(s) (ex. -t 1,5,13)\n"
        "  -k, --concatenate               : concatenate consecutive selected track(s) (ex. -k -t 2,3,4)\n"
        "  -I, --output-iso                : output as RAW ISO\n"
#ifndef SECTOR_LIMIT
        "  -w, --concurrent                : Concurrent ISO+DSF/DSDIFF processing mode\n"
#endif
        "  -c, --convert-dst               : convert DST to DSD\n"
        "  -C, --export-cue                : Export a CUE Sheet\n"
        "  -o, --output-dir[=DIR]          : Output directory for ISO or DSDIFF Edit Master\n"
        "  -y, --output-dir-conc[=DIR]     : Output directory for DSF or DSDIFF \n"
        "  -P, --print                     : display disc and track information\n"
        "  -A, --artist                    : artist name is added in folder name. Default is disabled\n"
        "  -a, --performer                 : performer name is added in track filename. Default is disabled\n"
        "  -b, --pauses                    : all pauses will be included. Default is disabled\n"
        "  -v, --version                   : Display version\n"
        "\n"
        "  -i, --input[=FILE]              : set source and determine if \"iso\" image, \n"
        "                                    device or server (ex. -i 192.168.1.10:2002)\n"
        "\n"
        "Help options:\n"
        "  -?, --help                      : Show this help message\n"
        "  --usage                         : Display brief usage message\n";

    static const char usage_text[] =
        "Usage: %s [-2|--2ch-tracks] [-m|--mch-tracks] [-p|--output-dsdiff]\n"
#ifdef SECTOR_LIMIT
        "        [-e|--output-dsdiff-em] [-s|--output-dsf] [-I|--output-iso]\n"
#else
        "        [-e|--output-dsdiff-em] [-s|--output-dsf] [-I|--output-iso] [-w|--concurrent]\n"
#endif
        "        [-c|--convert-dst] [-C|--export-cue] [-i|--input FILE] [-o|--output-dir DIR] [-y|--output-dir-conc DIR] [-P|--print]\n"
        "        [-?|--help] [--usage]\n";


#ifdef SECTOR_LIMIT
    static const char options_string[] = "2mepszt:kIcCo:y:PAabvi:?u";
#else
    static const char options_string[] = "2mepszt:kIwcCo:y:PAabvi:?u";
#endif

    static const struct option options_table[] = {
        {"2ch-tracks", no_argument, NULL, '2'},
        {"mch-tracks", no_argument, NULL, 'm'},
        {"output-dsdiff-em", no_argument, NULL, 'e'},
        {"output-dsdiff", no_argument, NULL, 'p'},
        {"output-dsf", no_argument, NULL, 's'},
        {"dsf-nopad", no_argument, NULL, 'z'},
        {"select-track",required_argument, NULL,'t'},
        {"concatenate", no_argument, NULL, 'k'},
        {"output-iso", no_argument, NULL, 'I'},
#ifndef SECTOR_LIMIT
        {"concurrent", no_argument, NULL, 'w'}, 
#endif
        {"convert-dst", no_argument, NULL, 'c'},
        {"export-cue", no_argument, NULL, 'C'},
        {"output-dir", required_argument, NULL, 'o'},
        {"output-dir-conc", required_argument, NULL, 'y'},
        {"print", no_argument, NULL, 'P'},
        {"artist", no_argument, NULL, 'A'},
        {"performer", no_argument, NULL, 'a'},
        {"pauses", no_argument, NULL, 'b'},
        {"version", no_argument, NULL, 'v'},
        {"input", required_argument, NULL, 'i'},                
        {"help", no_argument, NULL, '?'},
        {"usage", no_argument, NULL, 'u'},
        {NULL, 0, NULL, 0}};

    program_name = strrchr(argv[0],'/');
    program_name = program_name ? strdup(program_name+1) : strdup(argv[0]);

    while ((opt = getopt_long(argc, argv, options_string, options_table, NULL)) >= 0) {
        switch (opt) {
        case '2': 
            opts.two_channel = 1; 
            break;
        case 'm': 
            opts.multi_channel = 1; 
            break;
        case 'e': 
            opts.output_dsdiff_em = 1;
            //opts.output_dsdiff = 0;
            //opts.output_dsf = 0;
            //opts.output_iso = 0;
            opts.export_cue_sheet = 1;
            break;
        case 'p': 
            //opts.output_dsdiff_em = 0; 
            opts.output_dsdiff = 1;
            //opts.output_dsf = 0; 
            //opts.output_iso = 0;
            break;
        case 's': 
            //opts.output_dsdiff_em = 0; 
            //opts.output_dsdiff = 0;
            //opts.output_iso = 0; 
            opts.output_dsf = 1;
            // if(opts.two_channel == 0 && opts.multi_channel == 0)
            //     opts.two_channel = 1; 

            break;
        case 't': 
            {
                for(int m=0;m<255;m++)opts.selected_tracks[m]=0x00;
                int track_nr, count = 0;
                char *track = strtok(optarg, " ,");
                while (track != 0)
                {
                    track_nr = atoi(track);
                    track = strtok(0, " ,");
                    if (!track_nr)
                        continue;
                    track_nr = (track_nr - 1) & 0xff;
                    opts.selected_tracks[track_nr] = 0x01;
                    count++;
                }
                opts.select_tracks = count != 0;
            }
            break;
        case 'z':
            opts.dsf_nopad = 1;
            break;
        case 'b':
            opts.audio_frame_trimming = 0;
            break;
        case 'A':
            opts.artist_flag = 1;
            break;
        case 'a':
            opts.performer_flag = 1;
            break;
        case 'k': // concatenate consecutive tracks specified in selected_tracks
            opts.concatenate = 1;
            // must enable include pauses
            if (opts.audio_frame_trimming == 1)opts.audio_frame_trimming = 0;        
            break;
        case 'I': 
            //opts.output_dsdiff_em = 0; 
            //opts.output_dsdiff = 0; 
            //opts.output_dsf = 0; 
            opts.output_iso = 1;
            break;
		case 'w':
            opts.concurrent = 1;
            break;	
        case 'c': opts.convert_dst = 1; break;
        case 'C': opts.export_cue_sheet = 1; break;
        case 'i':
        {
            size_t n = strlen(optarg);
            if (n >= MAX_BUFF_FULL_PATH_LEN) n = MAX_BUFF_FULL_PATH_LEN-1;
            //opts.input_device = strdup(optarg);
            opts.input_device = calloc(n+1, sizeof(char));
            memcpy(opts.input_device, optarg, n);   
            break;

        } 
        case 'o':
        {
			size_t n = strlen(optarg);
            if (n >= MAX_BUFF_FULL_PATH_LEN) n = MAX_BUFF_FULL_PATH_LEN-1;
            if (n >= 2)
            {     				
				// remove double quotes if exists (especially in Windows)
				char * start_dir;
                if (optarg[0] ==  '\"' )
                {  
					start_dir=optarg + 1;
                    n =n-1;
                }
				else
					start_dir=optarg;
				if (start_dir[n - 1] ==  '\"' )  
				  n = n-1;
								 
                // strip ending slash if exists
                // if (start_dir[n - 1] == '\\' ||
                //     start_dir[n - 1] == '/')
                // {
				// 	n=n-1;
                // }
                //opts.output_dir_base = strndup(start_dir, n - 1); //  strndup didn't exist in Windows
                opts.output_dir_base = calloc(n+1, sizeof(char));
                memcpy(opts.output_dir_base, start_dir, n);                               
            }
            break;
        }
        case 'y': 
        {
			size_t n = strlen(optarg);
            if (n >= MAX_BUFF_FULL_PATH_LEN) n = MAX_BUFF_FULL_PATH_LEN-1;
            if (n >= 2)
            {     				
				// remove double quotes if exists (especially in Windows)
				char * start_dir;
                if (optarg[0] ==  '\"' )
                {
                    start_dir = optarg + 1;
                    n = n - 1;
                }
                else
					start_dir=optarg;
				if (start_dir[n - 1] ==  '\"' )  
				  n = n-1;
								 
                // strip ending slash if exists
                // if (start_dir[n - 1] == '\\' ||
                //     start_dir[n - 1] == '/')
                // {
				// 	n=n-1;
                // }
                //opts.output_dir_conc_base = strndup(start_dir, n - 1); //  strndup didn't exist in Windows
                opts.output_dir_conc_base = calloc(n+1, sizeof(char));
                memcpy(opts.output_dir_conc_base, start_dir, n);                               
            }		
            break;
        }
        case 'P': opts.print = 1; break;
        case 'v': opts.version = 1; break;

        case '?':
            fprintf(stdout, help_text, program_name);
            free(program_name);
            return 0;
            break;

        case 'u':
            fprintf(stdout, usage_text, program_name);
            free(program_name);
            return 0;
            break;
        }
    }
    free(program_name);

    return 1;
}

static lock *g_fwprintf_lock = 0;

static int safe_fwprintf(FILE *stream, const wchar_t *format, ...)
{
    int retval;
    va_list arglist;

    possess(g_fwprintf_lock);

    va_start(arglist, format);
    retval = vfwprintf(stream, format, arglist);
    va_end(arglist);

    fflush(stream);

    release(g_fwprintf_lock);

    return retval;
}

static void handle_sigint(int sig_no)
{
    safe_fwprintf(stdout, L"\n\n Program interrupted...                                                      \n");
    scarletbook_output_interrupt(output);
}


static void handle_status_update_track_callback(char *filename, int current_track, int total_tracks)
{
    wchar_t *wide_filename;

    CHAR2WCHAR(wide_filename, filename);
    safe_fwprintf(stdout, L"\nProcessing [%ls] (%d/%d)..\n", wide_filename, current_track, total_tracks);
    free(wide_filename);
}

static time_t started_processing;

static void handle_status_update_progress_callback(uint32_t stats_total_sectors, uint32_t stats_total_sectors_processed,
                                 uint32_t stats_current_file_total_sectors, uint32_t stats_current_file_sectors_processed)
{
    // safe_fwprintf(stdout, L"\rCompleted: %d%% (%.1fMB), Total: %d%% (%.1fMB) at %.2fMB/sec", (stats_current_file_sectors_processed*100/stats_current_file_total_sectors), 
    //                                          ((float)((double) stats_current_file_sectors_processed * SACD_LSN_SIZE / 1048576.00)),
    //                                          (stats_total_sectors_processed * 100 / stats_total_sectors),
    //                                          ((float)((double) stats_current_file_total_sectors * SACD_LSN_SIZE / 1048576.00)),
    //                                          (float)((double) stats_total_sectors_processed * SACD_LSN_SIZE / 1048576.00) / (float)(time(0) - started_processing)
    //                                          );
    if (stats_current_file_total_sectors == stats_total_sectors) // no need to print both stats because is one file  (ISO or DFF)
    {
        safe_fwprintf(stdout, L"\rCompleted: %d%% (file sectors processed: %d / total sectors:%d)",
                    (stats_current_file_sectors_processed * 100 / stats_current_file_total_sectors),
                    stats_current_file_sectors_processed,
                    stats_current_file_total_sectors);
    }
    else
    {
        safe_fwprintf(stdout, L"\rCompleted: %d%% (file sectors processed: %d / total sectors:%d), Total: %d%% (total sectors processed: %d / total sectors: %d)",
                    (stats_current_file_sectors_processed * 100 / stats_current_file_total_sectors),
                    stats_current_file_sectors_processed,
                    stats_current_file_total_sectors,
                    (stats_total_sectors_processed * 100 / stats_total_sectors),
                    stats_total_sectors_processed,
                    stats_total_sectors);
    }

}

/* Initialize global variables. */
static void init(void) 
{
    /* Default option values. */
    opts.two_channel        = 0;
    opts.multi_channel      = 0;
    opts.output_dsf         = 0;
    opts.output_iso         = 0;
    opts.output_dsdiff      = 0;
    opts.output_dsdiff_em   = 0;
    opts.convert_dst        = 0;
    opts.export_cue_sheet   = 0;
    opts.print              = 0;
    opts.output_dir_base        = NULL;
    opts.output_dir_conc_base	= NULL;
    opts.input_device       = NULL; //"/dev/cdrom";
    opts.version            = 0;
    opts.dsf_nopad          = 0;
    opts.audio_frame_trimming=1;  // default is On ; eliminates pauses
    opts.artist_flag        = 0;    // if artist ==1 then the artist name is added in folder name
    opts.performer_flag     = 0; // if performer ==1 the performer from each track is added
    opts.concatenate        = 0; // concatenate consecutive tracks specified in t
    opts.select_tracks      = 0;
    opts.logging            = 0;
    opts.id3_tag_mode       = 3; // default id3v2.3 ; ISO_8859_1 encoding // id3v2.4 tag and UTF8 encoding
    opts.concurrent         = 0;

#if defined(WIN32) || defined(_WIN32)
    signal(SIGINT, handle_sigint);
	
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_sigint;
    sigaction(SIGINT, &sa, NULL);
#endif

        //init_logging(1);   //init_logging(0); 0 = not create a log file
        g_fwprintf_lock = new_lock(0);
}

void print_start_time()
{
	started_processing = time(0);
	wchar_t *wide_asctime;
	CHAR2WCHAR(wide_asctime, asctime(localtime(&started_processing)));
	fwprintf(stdout, L"\n Started at: %ls    \n", wide_asctime );
	free(wide_asctime);
}
void print_end_time()
{
	time_t ended_processing=time(0);
	time_t seconds = difftime(ended_processing,started_processing);

	char elapsed_time[100];
	strftime(elapsed_time, 90, "%H hours:%M minutes:%S seconds", gmtime(&seconds));
    wchar_t *wide_result_time, *wide_asctime;
    CHAR2WCHAR(wide_result_time, elapsed_time);

	CHAR2WCHAR(wide_asctime, asctime(localtime(&ended_processing)));

	fwprintf(stdout, L"\n\n Ended at: %ls [elapsed: %ls]\n", wide_asctime, wide_result_time);
	free(wide_result_time);
	free(wide_asctime);	
}
#if defined(WIN32) || defined(_WIN32)
/*  Convert wide argv to UTF8   */
/*  only for Windows           */

char ** convert_wargv_to_UTF8(int argc,wchar_t *wargv[])
{
    int i;

    char **argv = malloc((argc + 1) * sizeof(*argv));

    for (i = 0; i < argc; i++)
    {
        argv[i] = (char *)charset_convert((char *)wargv[i], wcslen((const wchar_t *)wargv[i]) * sizeof(wchar_t), "UCS-2-INTERNAL", "UTF-8");
    }
         
    argv[i] = NULL;
    return argv;
}
#endif


//   read from file sacd_extract.cfg
//   if artist=1 then artist name will be added to the name of folder
//   if sacd_extract.cfg didn't exist then return 0 
//       else 1
//    
int read_config()
{
    int ret;
    const char *filename_cfg = "sacd_extract.cfg"; //char str[] = "sacd_extract.cfg";

#if defined(WIN32) || defined(_WIN32)
    struct _stat fileinfo_win;
    ret = _stat(filename_cfg, &fileinfo_win);
#else
    struct stat fileinfo;
    ret = stat(filename_cfg, &fileinfo);
#endif

    if (ret != 0) // if cfg file do not exists
    {
Use_default:
        fwprintf(stdout, L"\nUse default configuration settings...\n" );
        return 0; 
    }

#if defined(WIN32) || defined(_WIN32)
    if ((fileinfo_win.st_mode & _S_IFMT) == _S_IFREG)
#else
    if (S_ISCHR(fileinfo.st_mode) ||
        S_ISREG(fileinfo.st_mode))
#endif
    {
        FILE *fp;
        char content[100]; // content to be read

        fp = fopen(filename_cfg, "r");
        if (!fp)
            return 0;

        while (fgets(content, 100, fp) != NULL)
        {
            if ((strstr(content, "artist=1") != NULL) ||(strstr(content, "artist=yes") != NULL))
                opts.artist_flag = 1;
            if ((strstr(content, "performer=1") != NULL) || (strstr(content, "performer=yes") != NULL))
                opts.performer_flag = 1;
            if ((strstr(content, "pauses=1") != NULL) || (strstr(content, "pauses=yes") != NULL))
                opts.audio_frame_trimming = 0;
            if ((strstr(content, "nopad=1") != NULL) || (strstr(content, "nopad=yes") != NULL))
                opts.dsf_nopad = 1;
            if ((strstr(content, "concatenate=1") != NULL) || (strstr(content, "concatenate=yes") != NULL))
            {
                opts.concatenate = 1;
                opts.audio_frame_trimming = 0; // when concatenate must include all pausese and disable dsf_pad !!!
            }  
            if ((strstr(content, "logging=1") != NULL) || (strstr(content, "logging=yes") != NULL))
                    opts.logging = 1;

            if ((strstr(content, "id3tag=0") != NULL) || (strstr(content, "id3tag=no") != NULL)) // 0=no id3 inserted
                opts.id3_tag_mode=0;
            if (strstr(content, "id3tag=1") != NULL) // 1=id3 v2.3;  UTF-16 encoding
                opts.id3_tag_mode = 1;
            if (strstr(content, "id3tag=2") != NULL) // 2=miminal id3v2.3 tag; UTF-16 encoding
                opts.id3_tag_mode = 2;
            if (strstr(content, "id3tag=3") != NULL) // 3=id3v2.3 ; ISO_8859_1 encoding
                opts.id3_tag_mode = 3;
            if (strstr(content, "id3tag=4") != NULL) // 4=id3v2.4; UTF-8 encoding
                opts.id3_tag_mode = 4;
            if (strstr(content, "id3tag=5") != NULL) // 5=id3v2.4 minimal;UTF-8 encoding
                opts.id3_tag_mode = 5;
        }
        fclose(fp);
        fwprintf(stdout, L"\nFound configuration 'sacd_extract.cfg' file...\n" );

        return 1;
    }
    else
    {
        goto Use_default;
    }
    
} // end read_config

void show_options()
{
        
    fwprintf(stdout, L"\tArtist will be added in folder name [artist=%d] %ls\n",opts.artist_flag, opts.artist_flag > 0 ? L"yes" : L"no");
    fwprintf(stdout, L"\tPerformer will be added in filename of track [performer=%d] %ls\n",opts.performer_flag, opts.performer_flag > 0 ? L"yes" : L"no");
    fwprintf(stdout, L"\tPadding-less [nopad=%d] %ls\n", opts.dsf_nopad, opts.dsf_nopad != 0 ? L"yes" : L"no");
    fwprintf(stdout, L"\tPauses included [pauses=%d] %ls\n", !opts.audio_frame_trimming, opts.audio_frame_trimming == 0 ? L"yes" : L"no");
    fwprintf(stdout, L"\tConcatenate [concatenate=%d] %ls\n", opts.concatenate, opts.concatenate > 0 ? L"yes" : L"no");
    switch (opts.id3_tag_mode)
    {
    case 0:
        fwprintf(stdout, L"\tID3tag not inserted [id3tag = %d] \n", opts.id3_tag_mode);
        break;
    case 1:
        fwprintf(stdout, L"\tID3tagV2.3 UCS-2 encoding (UTF-16 encoded Unicode with BOM) [id3tag = %d]\n", opts.id3_tag_mode);
        break;
    case 2:
        fwprintf(stdout, L"\tID3tagV2.3 (UCS-2 encoding, miminal) [id3tag = %d]\n", opts.id3_tag_mode);
        break;   
    case 3:
        fwprintf(stdout, L"\tID3tagV2.3 (ISO_8859_1 encoding) [id3tag = %d]\n", opts.id3_tag_mode);
        break;
    case 4:
         fwprintf(stdout, L"\tID3tagV2.4 (UTF-8 encoding) [id3tag = %d]\n", opts.id3_tag_mode);
        break;   
    case 5:
        fwprintf(stdout, L"\tID3tagV2.4 (UTF-8 encoding, minimal) [id3tag = %d]\n", opts.id3_tag_mode);
        break;
    default:
        fwprintf(stdout, L"\tID3tag unknonw [id3tag = %d]\n", opts.id3_tag_mode);
        break;
    }
    fwprintf(stdout, L"\tLogging [logging = %d] %ls\n", opts.logging, opts.logging != 0 ? L"yes" : L"no");


    fwprintf(stdout, L"Options received:\n");
    
    if(opts.input_device != NULL)
    {
        wchar_t *wide_filename;
        CHAR2WCHAR(wide_filename, opts.input_device);
        fwprintf(stdout, L"\tInput -i (iso or connection) [%ls]\n",wide_filename);
        free(wide_filename);
    }
    if(opts.output_dir_base !=NULL)
    {
        wchar_t *wide_filename;
        CHAR2WCHAR(wide_filename, opts.output_dir_base);
        fwprintf(stdout, L"\tOutput folder -o [%ls]\n", wide_filename);
        free(wide_filename);        
    }
    if(opts.output_dir_conc_base !=NULL) 
    {  
        wchar_t *wide_filename;
        CHAR2WCHAR(wide_filename, opts.output_dir_conc_base);
        fwprintf(stdout, L"\tOutput folder for concurent -y [%ls]\n",wide_filename);
        free(wide_filename);           
    }
    if(opts.print != 0)fwprintf(stdout, L"\tPrint details of album -P \n");
    if(opts.two_channel != 0)fwprintf(stdout, L"\tAsked two channels -2 \n");
    if(opts.multi_channel != 0)fwprintf(stdout, L"\tAsked multi channels -m \n");
    if(opts.output_dsf != 0)fwprintf(stdout, L"\tAsked dsf -s \n");
    if(opts.output_dsdiff != 0)fwprintf(stdout, L"\tAsked dsddif -p \n");
    if(opts.output_dsdiff_em != 0)fwprintf(stdout, L"\tAsked dsf -e \n");
    if(opts.dsf_nopad  != 0)fwprintf(stdout, L"\tAsked dsf nopad -z \n");
    if(opts.output_iso != 0)fwprintf(stdout, L"\tAsked ISO -I \n");
    if(opts.convert_dst != 0)fwprintf(stdout, L"\tAsked for DST decompression -c \n");
    if(opts.export_cue_sheet != 0)fwprintf(stdout, L"\tAsked for cuesheet+xml metadata -C \n");
    if(opts.concurrent != 0)fwprintf(stdout, L"\tAsked for concurrent -w \n");

    if(opts.select_tracks > 0) 
    {
        fwprintf(stdout, L"\tTracks selected: -t:");
        for (int j=0;j< 255; j++ )
        {
            if(opts.selected_tracks[j] == 0x01)
             fwprintf(stdout, L" %d", j+1);
        }
        
    }


}

//#define MAX_PATH_OUTPUT_SIZE 16384U  ---> replaced by MAX_BUFF_FULL_PATH_LEN in fileutils.h

//   Creates directory tree like: Album title (\ (Disc no.. )\ Stereo (or Multich)
//   Useful for dsf, dff files.
//     input: handle
//            area_idx
//            If there is no multichannel area then it did not add \Stereo..or Multich 
//            base_output_dir = directory from where to start creating new directory tree
//
char *create_path_output(scarletbook_handle_t *handle, int area_idx, char * base_output_dir)
{
    char *path_output;
    char *album_path;
    size_t album_path_len;
    size_t path_output_size = MAX_BUFF_FULL_PATH_LEN; //MAX_PATH_OUTPUT_SIZE;
	
    album_path = get_path_disc_album(handle);
	if(album_path==NULL)
    {
        fwprintf(stdout, L"\n\n ERROR in main:create_path_output()...get_path_disc_album() returned NULL\n");
        LOG(lm_main, LOG_ERROR, ("ERROR in main:create_path_output()...get_path_disc_album() returned NULL"));        
        return NULL;
    }

    album_path_len = strlen(album_path);

    LOG(lm_main, LOG_NOTICE, ("NOTICE in main:create_path_output()...after get_path-disc_album: album_path=[%s]; len=[%d]", album_path,album_path_len));
	
	if(base_output_dir !=NULL)
	{
        size_t base_output_dir_len;

        base_output_dir_len =  strlen(base_output_dir);

        if(base_output_dir_len > MAX_BUFF_FULL_PATH_LEN)  // cannot create a folder here
        {
            free(album_path);
            fwprintf(stdout, L"\nERROR in main:create_path_output()...output folder is huge(> %d). Try one shorter.\n",MAX_BUFF_FULL_PATH_LEN);
            LOG(lm_main, LOG_ERROR, ("ERROR in main:create_path_output()...output folder is huge (> %d). Try one shorter.",MAX_BUFF_FULL_PATH_LEN)); 
            return NULL;       
        }

        if(base_output_dir_len + album_path_len > MAX_BUFF_FULL_PATH_LEN - 8 )  // shrink the size of album_path
        {
            free(album_path);
            fwprintf(stdout, L"\nERROR in main:create_path_output(). The total size of output folder + the one will be generated is huge(> %d). Try one shorter.\n",MAX_BUFF_FULL_PATH_LEN);
            LOG(lm_main, LOG_ERROR, ("ERROR in main:create_path_output(). The total size of output folder + the one will be generated is huge (> %d). Try one shorter.",MAX_BUFF_FULL_PATH_LEN)); 
            return NULL;     
        }


        path_output_size = min(base_output_dir_len + album_path_len + 20,  MAX_BUFF_FULL_PATH_LEN - 1 );
        path_output = calloc(path_output_size, sizeof(char));

        if(path_output != NULL)
        {
            memcpy(path_output, base_output_dir, base_output_dir_len);

        #if defined(WIN32) || defined(_WIN32)      
            if (base_output_dir[base_output_dir_len-1] != '\\')
                strcat(path_output, "\\");
        #else
            if (base_output_dir[base_output_dir_len-1] != '/' )
                strcat(path_output, "/");
        #endif      
                

            if( base_output_dir_len + 2 + album_path_len < path_output_size)
                strcat(path_output, album_path);
        }
        else
        {
            wchar_t *wide_filename;
  Err1:     CHAR2WCHAR(wide_filename, album_path);
            fwprintf(stdout, L"\n ERROR in main:create_path_output()...calloc() returned NULL; album_path=%ls\n",wide_filename);
            free(wide_filename);

            LOG(lm_main, LOG_ERROR, ("ERROR in main:create_path_output()...calloc() returned NULL; album_path=%s",album_path));
            free(album_path);
            return NULL;        
        }

	}
	else  // base_output_dir ==NULL
    {
        path_output_size = min(album_path_len + 20, MAX_BUFF_FULL_PATH_LEN); //MAX_PATH_OUTPUT_SIZE
        path_output = calloc(path_output_size, sizeof(char));

        if(path_output != NULL)
        {
            strcat(path_output, album_path);
        }
        else
        {
            goto Err1;

        }

    }
 
    free(album_path);

    if (has_multi_channel(handle))
    {
        
    #if defined(WIN32) || defined(_WIN32)            
        strcat(path_output, "\\");
    #else      
        strcat(path_output, "/");
    #endif   

        strcat(path_output, get_speaker_config_string(handle->area[area_idx].area_toc));
    }

    if (path_dir_exists(path_output) == 0)
    {
        // not exists, then create it

        int ret_mkdir = recursive_mkdir(path_output, base_output_dir, 0777);

        if (ret_mkdir != 0)
        {
            wchar_t *wide_filename;
            CHAR2WCHAR(wide_filename, path_output);
            fwprintf(stdout, L"\nERROR in main:create_path_output()...folder '%ls' can't be created\n", wide_filename);
            free(wide_filename);

            LOG(lm_main, LOG_ERROR, ("ERROR in main:create_path_output()...folder '%s' can't be created", path_output));

            free(path_output);
            return NULL;
        }
    }

    LOG(lm_main, LOG_NOTICE, ("NOTICE in main:create_path_output()...directory created: %s", path_output));
    LOG(lm_main, LOG_NOTICE, ("NOTICE in main:create_path_output()...base_output_dir: %s", base_output_dir));
    return path_output;
}
//
//  Get the current working directory:
//  the returned buffer must be freed after use
//
char * return_current_directory()
{
    // Get the current working directory:
    char *buffer;
#if defined(WIN32) || defined(_WIN32)
    if ((buffer = _getcwd(NULL, 0)) == NULL)
    {
        perror("_getcwd error");
        fwprintf(stdout, L"\n\n Error: Cannot get the working directory.\n");
    }
        
#else

    if((buffer = getcwd(NULL,0)) == NULL)
    {
        perror("_getcwd error");
        fwprintf(stdout, L"\n\n Error: Cannot get the working directory.\n");
    }
#endif

    // if(buffer != NULL)
    // {
    //     // remove the last trail
    //     // strip ending slash if exists
    //     size_t n= strlen(buffer);

    //     if (buffer[n - 1] == '\\' ||
    //         buffer[n - 1] == '/')
    //     {
    //         buffer[n - 1]='\0';
    //     }
    // }
    return buffer;
}


#if defined(WIN32) || defined(_WIN32)
    int wmain(int argc, wchar_t *wargv[])      
#else
    int main(int argc, char *argv[])
#endif
{
    char *album_filename = NULL, *musicfilename = NULL, *file_path = NULL, *output_dir = NULL;;
    char *file_path_iso_unique = NULL;
    int i, area_idx;
    sacd_reader_t *sacd_reader = NULL;
	int exit_main_flag=0; //0=succes; -1 failed

#ifdef PTW32_STATIC_LIB
    pthread_win32_process_attach_np();
    pthread_win32_thread_attach_np();
#endif

    init();
   
#if defined(WIN32) || defined(_WIN32)
    char **argvw_utf8 = convert_wargv_to_UTF8(argc,wargv);
    if (parse_options(argc, argvw_utf8))
#else
    if (parse_options(argc, argv))
#endif
    {
        setlocale(LC_ALL, "");
        if (fwide(stdout, 1) < 0)
        {
            fprintf(stderr, "\nERROR: Output not set to wide.\n");
			exit_main_flag=-1;
            goto exit_main_1;
        }
        fwprintf(stdout, L"\nsacd_extract client " SACD_RIPPER_VERSION_STRING "\n");
        fwprintf(stdout, L"\nEnhanced by euflo ....starting!\n");

        read_config();
        init_logging(opts.logging); //init_logging(0); 1= write logs in a file

        show_options();

        // Just print the current (working) directory:
        char *buffer;
        if ((buffer = return_current_directory() ) != NULL)   
        {
            wchar_t *wide_filename;
            CHAR2WCHAR(wide_filename, buffer);
            fwprintf(stdout, L"\nCurrent (working) directory (for the app and 'sacd_extract.cfg' file): [%ls]\n",wide_filename);
            free(wide_filename);
            free(buffer);
        }


        LOG(lm_main, LOG_NOTICE, ("sacd_extract Version: %s  ", SACD_RIPPER_VERSION_STRING));

        if (opts.version==1)
        {
            //fwprintf(stdout, L"\n" SACD_RIPPER_VERSION_INFO "\n");
            fwprintf(stdout, L"git repository: " SACD_RIPPER_REPO "\n");
            
            goto exit_main;
        }

        // default to 2 channel
        if (opts.two_channel == 0 && opts.multi_channel == 0)
        {
            opts.two_channel = 1;
        }

#if defined(WIN32) || defined(_WIN32)
        if ((opts.output_dir_base == NULL) && (opts.output_dir_conc_base == NULL))
        {
            // Get the current working directory:
            char *buffer;          
            if ((buffer = return_current_directory()) != NULL)
            {
                opts.output_dir_base = strdup(buffer);
                free(buffer);
            }                                
        }
#endif

        if (opts.output_dir_base != NULL   ) // test if exists 
        {
            if (path_dir_exists(opts.output_dir_base) == 0)
            {
                wchar_t *wide_filename;
                CHAR2WCHAR(wide_filename, opts.output_dir_base);
                fwprintf(stdout, L"%ls output dir doesn't exist or is not a directory.\n",wide_filename);
                free(wide_filename);

                LOG(lm_main, LOG_ERROR, ("ERROR in main: output dir [%s] doesn't exist or is not a directory!!\n", opts.output_dir_base));

				exit_main_flag=-1;
                goto exit_main;
            }
            if (opts.output_dir_conc_base == NULL && opts.concatenate)
                opts.output_dir_conc_base = strdup(opts.output_dir_base);
        }
		
		if (opts.output_dir_conc_base != NULL   ) // test if exists 
        {
            if (path_dir_exists(opts.output_dir_conc_base) == 0)
            {
                wchar_t *wide_filename;
                CHAR2WCHAR(wide_filename, opts.output_dir_conc_base);
                fwprintf(stdout, L"%ls doesn't exist or is not a directory.\n",wide_filename);
                free(wide_filename);

                LOG(lm_main, LOG_ERROR, ("ERROR in main: output dir conc [%s] doesn't exist or is not a directory!!\n", opts.output_dir_conc_base));

                exit_main_flag=-1;
                goto exit_main;
            }
            if (opts.output_dir_base == NULL)
                opts.output_dir_base = strdup(opts.output_dir_conc_base);
        }

        if(opts.input_device == NULL)
        {
            opts.input_device = strdup("/dev/cdrom");
        }

        fwprintf(stdout, L"\nStart reading sacd...\n");
        LOG(lm_main, LOG_NOTICE, ("Start reading sacd..."));
Open_sacd:
        sacd_reader = sacd_open(opts.input_device);
        if (sacd_reader != NULL) 
        {
            handle = scarletbook_open(sacd_reader);
            if (handle)
            {
                handle->concatenate = opts.concatenate;
                handle->audio_frame_trimming = opts.audio_frame_trimming;  
                handle->dsf_nopad = opts.dsf_nopad;
                handle->id3_tag_mode=opts.id3_tag_mode;
                handle->artist_flag=opts.artist_flag;
                handle->performer_flag=opts.performer_flag;

                if (opts.print) 
                { 
                    scarletbook_print(handle); 
                    opts.print = 0;
                }

                if( output_dir  == NULL)
                {
                    // generate the main output folder stored in 'output_dir' 
                    char *album_path = get_path_disc_album(handle);
                    size_t album_path_size;
                    size_t output_dir_size;

                    album_path_size = strlen(album_path);

                    //LOG(lm_main, LOG_NOTICE, ("NOTICE in main:after get_path_disc_album(); album_path=[%s]",album_path));

                    if (opts.output_dir_base != NULL)
                    {
                        size_t size_output_dir_base = strlen(opts.output_dir_base);

                        output_dir_size = size_output_dir_base + 1 + album_path_size + 1; 

                        // do some safety checks
                        if(size_output_dir_base > MAX_BUFF_FULL_PATH_LEN-1)
                        {
                            LOG(lm_main, LOG_ERROR, ("ERROR in main: output folder is huge(> %d). Try one shorter.", MAX_BUFF_FULL_PATH_LEN));
                            fwprintf(stdout, L"\nERROR in main: output folder is huge(> %d). Try one shorter.\n", MAX_BUFF_FULL_PATH_LEN);
                            goto Err_close;
                        } 

                        if(output_dir_size > MAX_BUFF_FULL_PATH_LEN-1)
                        {
                            LOG(lm_main, LOG_ERROR, ("ERROR in main: The total size of output folder + the one will be generated is huge(> %d). Try one shorter.", MAX_BUFF_FULL_PATH_LEN));
                            fwprintf(stdout, L"\nERROR in main: The total size of output folder + the one will be generated is huge(> %d). Try one shorter.\n", MAX_BUFF_FULL_PATH_LEN);
                            goto Err_close;
                        } 

                        
                        output_dir = calloc(output_dir_size, sizeof(char));
                        LOG(lm_main, LOG_NOTICE, ("NOTICE in main:after calloc(output_dir_size=[%d], 1)",output_dir_size));

                        if(output_dir !=NULL)
                        {
                            strncpy(output_dir, opts.output_dir_base,output_dir_size);
                        
                        #if defined(WIN32) || defined(_WIN32)
                            if (opts.output_dir_base[size_output_dir_base - 1] != '\\')
                                strcat(output_dir, "\\");
                        #else
                            if (opts.output_dir_base[size_output_dir_base - 1] != '/')
                                strcat(output_dir, "/"); 
                        #endif
                                

                            if( album_path_size + size_output_dir_base + 1 < output_dir_size)
                                strcat(output_dir, album_path);
                        }
                        else
                        {
Err_calloc1:                            
                            LOG(lm_main, LOG_ERROR, ("ERROR in main: at calloc(); cannot create output_dir"));
                            fwprintf(stdout, L"\nERROR in main: at calloc(); cannot create output_dir\n");

Err_close:                  scarletbook_close(handle);
                            sacd_close(sacd_reader);
                            exit_main_flag=-1;
                            
                            goto exit_main;                            
                        }     

                    }
                    else
                    {
                            output_dir_size = album_path_size + 1;
                            output_dir = calloc(output_dir_size, sizeof(char));
                            if(output_dir != NULL)
                                strcat(output_dir, album_path);
                            else goto Err_calloc1;    
                    }


                    free(album_path);
                    LOG(lm_main, LOG_NOTICE, ("NOTICE in main: after get_path_disc_album()...output_dir=[%s]", output_dir));
                
                } // if( output_dir  == NULL)

                if(album_filename == NULL)
                {
                    album_filename = get_album_dir(handle);

                    if(album_filename == NULL)
                    {
                        LOG(lm_main, LOG_ERROR, ("ERROR in main: cannot create album_filename"));
                        fwprintf(stdout, L"\nERROR in main: cannot create album_filename\n"); 
                        goto Err_close;                       

                    }
                    //LOG(lm_main, LOG_NOTICE, ("NOTICE in main: after get_album_dir()...album_filename=[%s]", album_filename));
                }

                if (opts.export_cue_sheet && file_path_iso_unique == NULL)  // export XML metadata at first; if already iso export is done, then do not export xml second time
                {

                    if (path_dir_exists(output_dir) == 0)
                    {
                        // not exists, then create it
                        int ret_mkdir = recursive_mkdir(output_dir, opts.output_dir_base, 0777);

                        if (ret_mkdir != 0)
                        {
                            LOG(lm_main, LOG_ERROR, ("ERROR in main: exporting XML, after recursive_mkdir...output_dir='%s'; ret=%d", output_dir, ret_mkdir));
                            free(album_filename);
                            free(output_dir);
                            
                            goto Err_close;
                        }
                    }

                    // create file XML metadata file
                    char *metadata_file_path_unique = get_unique_filename(NULL, output_dir, album_filename, "xml");
                    if (metadata_file_path_unique == NULL)
                        fwprintf(stdout, L"\n ERROR: cannot create get_unique_filename XML for metadata (==NULL) !!\n");
                    else
                    {
                        wchar_t *wide_filename;
                        CHAR2WCHAR(wide_filename, metadata_file_path_unique);
                        fwprintf(stdout, L"\n\nExporting metadata in XML file: [%ls] ... \n", wide_filename);
                        free(wide_filename);

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
                        char filename_long[MAX_BUFF_FULL_PATH_LEN];
                        memset(filename_long, '\0', sizeof(filename_long));
                        strcpy(filename_long, "\\\\?\\");
                        strncat(filename_long, metadata_file_path_unique, MAX_BUFF_FULL_PATH_LEN-8);

                        write_metadata_xml(handle, filename_long);

#else
                        write_metadata_xml(handle, metadata_file_path_unique);

#endif

                        free(metadata_file_path_unique);
                        fwprintf(stdout, L"\n\nWe are done exporting metadata in XML file. \n");
                        LOG(lm_main, LOG_NOTICE, ("NOTICE in main: done exporting metadata in XML file."));
                    }
                } // end if XML export       

                if (opts.output_iso)
                {
                    opts.output_iso = 0;   

                    // create the output folder
                    LOG(lm_main, LOG_NOTICE, ("NOTICE in main: extracting ISO, before recursive_mkdir(output_dir,..)...output_dir: %s", output_dir));

                    if (path_dir_exists(output_dir) == 0)
                    {
                        // not exists, then create it

                        int ret_mkdir = recursive_mkdir(output_dir, opts.output_dir_base, 0777);

                        if (ret_mkdir != 0)
                        {
                            LOG(lm_main, LOG_ERROR, ("ERROR in main: ISO, after recursive_mkdir...output_dir: %s; ret=%d;", output_dir, ret_mkdir));
                            free(album_filename);
                            free(output_dir);
                          
                            goto Err_close;
                        }
                    }

                    output = scarletbook_output_create(handle, handle_status_update_track_callback, handle_status_update_progress_callback, safe_fwprintf);

                    
#ifdef SECTOR_LIMIT
#define FAT32_SECTOR_LIMIT 2090000
                    uint32_t sector_size = FAT32_SECTOR_LIMIT;
                    uint32_t sector_offset = 0;
                    uint32_t total_sectors;

                    total_sectors = handle->total_sectors_iso;

                    if (total_sectors > FAT32_SECTOR_LIMIT)
                    {
                        musicfilename = (char *) malloc(512);
                        file_path = make_filename(NULL, output_dir, album_filename, "iso");
                        for (i = 1; total_sectors != 0; i++)
                        {
                            sector_size = min(total_sectors, FAT32_SECTOR_LIMIT);
                            snprintf(musicfilename, 512, "%s.%03d", file_path, i);
                            scarletbook_output_enqueue_raw_sectors(output, sector_offset, sector_size, musicfilename, "iso");
                            sector_offset += sector_size;
                            total_sectors -= sector_size;
                        }
                        free(file_path);
                        free(musicfilename);
                        
                    }
                    else
#endif
                    {
                        file_path_iso_unique = get_unique_filename(NULL, output_dir, album_filename, "iso");

                        wchar_t *wide_filename;
                        CHAR2WCHAR(wide_filename, file_path_iso_unique);
                        fwprintf(stdout, L"\nExporting ISO output in file: [%ls] ... \n", wide_filename);
                        free(wide_filename);

                        scarletbook_output_enqueue_raw_sectors(output, 0, handle->total_sectors_iso, file_path_iso_unique, "iso");
                        
                    }
                    
                    print_start_time();
                    scarletbook_output_start(output);
                    scarletbook_output_destroy(output);
                    print_end_time();

                    fwprintf(stdout, L"\nWe are done exporting ISO.\n");

                } // end if (opts.output_iso)


                if(opts.concurrent && file_path_iso_unique != NULL) // if concurent, use 'file_path_iso_unique' as an input device
                {
                    opts.concurrent = 0;
                    scarletbook_close(handle);
                    sacd_close(sacd_reader);
                    free(opts.input_device);
                    opts.input_device = strdup(file_path_iso_unique);  // use the freshly already created iso file as an input device/net
                    fwprintf(stdout, L"\nConcurent mode. Start re-opening sacd iso file locally... \n");
                    LOG(lm_main, LOG_NOTICE, ("NOTICE in main: Concurent mode. Start re-opening sacd iso file locally... "));
                    goto Open_sacd;
                }


                if (opts.output_dsf || opts.output_dsdiff || opts.output_dsdiff_em || opts.export_cue_sheet)
                {

                    while (opts.two_channel + opts.multi_channel > 0)
                    {
                        if (opts.multi_channel && (!has_multi_channel(handle))) // skip if we want multich but disc have no multich area
                        {
                            fwprintf(stdout, L"\n Asked for multi-channel format but disc has no multichannel area. So skip processing...\n");
                            opts.multi_channel = 0;
                            continue;
                        }

                        if (opts.two_channel && (!has_two_channel(handle) )) // skip;   if want 2ch but disc have no 2 ch area (YES !!! Exists these type of discs  - e.g Rubinstein - Grieg..only multich area)                                                    
                        {
                                fwprintf(stdout, L"\n Asked for stereo format but disc has no stereo area. So skip processing...\n");
                                opts.two_channel = 0;
                                continue;
                        }

                        // select the channel area
                        area_idx = has_multi_channel(handle) && opts.multi_channel ? handle->mulch_area_idx : handle->twoch_area_idx;
                        

                        // create the output folder with Stereo/MulCh
                        char *output_dir_dsd = create_path_output(handle, area_idx, opts.output_dir_base);
                        if (output_dir_dsd == NULL)
                        {
                            free(album_filename);
                            free(output_dir);
                            LOG(lm_main, LOG_ERROR, ("ERROR in main: after create_path_output() for dsf/dff/dff_em"));
                            
                            goto Err_close;
                        }

                        if (opts.output_dsdiff_em)
                        {

                            char *file_path_dsdiff_unique = get_unique_filename(NULL, output_dir_dsd, album_filename, "dff");   

                            wchar_t *wide_filename;
                            CHAR2WCHAR(wide_filename, file_path_dsdiff_unique);
                            fwprintf(stdout, L"\nExporting DFF edit master output in file: [%ls] ... \n", wide_filename);
                            free(wide_filename);

                            output = scarletbook_output_create(handle, handle_status_update_track_callback, handle_status_update_progress_callback, safe_fwprintf);

                            scarletbook_output_enqueue_track(output, area_idx, 0, file_path_dsdiff_unique, "dsdiff_edit_master",
                                                            (opts.convert_dst ? 1 : handle->area[area_idx].area_toc->frame_format != FRAME_FORMAT_DST));

                            free(file_path_dsdiff_unique);

                            print_start_time();
                            
                            scarletbook_output_start(output);
                            scarletbook_output_destroy(output);
                            
                            print_end_time();						

                            fwprintf(stdout, L"\n\nWe are done exporting DFF edit master.\n");

                            // Must generate cue sheet because it is mandatory just for dsdiff_em files
                            opts.export_cue_sheet=1;

                        } // end if  opts.output_dsdiff_em

                        if (opts.export_cue_sheet)
                        {

                            char *cue_file_path_unique = get_unique_filename(NULL, output_dir_dsd, album_filename, "cue");

                            wchar_t *wide_filename;
                            CHAR2WCHAR(wide_filename, cue_file_path_unique);
                            fwprintf(stdout, L"\n\nExporting CUE sheet: [%ls] ... \n", wide_filename);
                            free(wide_filename);

                            file_path = make_filename(NULL, NULL, album_filename, "dff");

							int rez_cuesheet= write_cue_sheet(handle, file_path, area_idx, cue_file_path_unique);
							if(rez_cuesheet != -1)
								fwprintf(stdout, L"\n\nWe are done exporting CUE sheet. \n");
							else
								fwprintf(stdout, L"\n\n ERROR: Cannot create CUE sheet file. \n");    
                            
                            free(cue_file_path_unique);
                            free(file_path);                                

                        }

                        if (opts.output_dsf || opts.output_dsdiff)
                        {

                            wchar_t *wide_folder;
                            CHAR2WCHAR(wide_folder, output_dir_dsd);
                            if (opts.output_dsf)
                                fwprintf(stdout, L"\nExporting DSF output in folder: [%ls] ... \n", wide_folder);
                            else
                                fwprintf(stdout, L"\nExporting DSDIFF output in folder: [%ls]  ... \n", wide_folder);

                            free(wide_folder);

                            output = scarletbook_output_create(handle, handle_status_update_track_callback, handle_status_update_progress_callback, safe_fwprintf);

                            if(opts.concatenate == 0)
                            {
                                int no_of_enqued_tracks=0;
                                int no_total_tracks = handle->area[area_idx].area_toc->track_count;
                                // fill the queue with items to rip
                                for (i = 0; i < no_total_tracks; i++)
                                {
                                    if (opts.select_tracks && opts.selected_tracks[i] == 0x0)
                                        continue;

                                    musicfilename = get_music_filename(handle, area_idx, i, "");

                                    if (opts.output_dsf)
                                    {
                                        file_path = make_filename(NULL, output_dir_dsd, musicfilename, "dsf");
                                        scarletbook_output_enqueue_track(output, area_idx, i, file_path, "dsf",
                                                                         1 /* always decode to DSD */);
                                        no_of_enqued_tracks++;                                       
                                    }
                                    else if (opts.output_dsdiff)
                                    {
                                        file_path = make_filename(NULL, output_dir_dsd, musicfilename, "dff");
                                        scarletbook_output_enqueue_track(output, area_idx, i, file_path, "dsdiff",
                                                                         (opts.convert_dst ? 1 : handle->area[area_idx].area_toc->frame_format != FRAME_FORMAT_DST));
                                        no_of_enqued_tracks++;
                                    }
                                    free(file_path);
                                    free(musicfilename);
                                }
                                if ((no_of_enqued_tracks < no_total_tracks) && !opts.select_tracks)
                                {
                                    fwprintf(stdout, L"\n Error: Number of processed tracks %d will be smaller than total tracks %d !!\n", no_of_enqued_tracks, no_total_tracks);
                                }
                            }
                            else  // made concatenation
                            {
                                // fill the queue with item to rip
                                if (opts.select_tracks)
                                {
                                    int first_track = handle->area[area_idx].area_toc->track_count-1;
                                    int last_track  = 0;
                                    // find first track and last track in list
                                    for (i = 0; i < handle->area[area_idx].area_toc->track_count; i++)
                                    {
                                        if (opts.selected_tracks[i] == 0x01)
                                        {
                                            if (first_track > i)
                                                first_track = i;
                                            if (last_track <  i)
                                                last_track = i;
                                        }                                         
                                    }


                                    if ((first_track < handle->area[area_idx].area_toc->track_count)&&
                                        (last_track < handle->area[area_idx].area_toc->track_count) )
                                    {
                                        char conc_string[32];
                                        snprintf(conc_string, sizeof(conc_string), "[%02d-%02d]",first_track + 1, last_track + 1);

                                        musicfilename = get_music_filename(handle, area_idx, first_track, conc_string);

                                        fwprintf(stdout, L"\n Concatenate tracks: %d to %d\n", first_track+1,last_track+1);
                                        if (opts.output_dsf)
                                        {
                                            file_path = make_filename(NULL, output_dir_dsd, musicfilename, "dsf");
                                            scarletbook_output_enqueue_concatenate_tracks(output, area_idx, first_track, file_path, "dsf",
                                                                                          1 /* always decode to DSD */, last_track);
                                        }
                                        else if (opts.output_dsdiff)
                                        {
                                            file_path = make_filename(NULL, output_dir_dsd, musicfilename, "dff");
                                            scarletbook_output_enqueue_concatenate_tracks(output, area_idx, first_track, file_path, "dsdiff",
                                                                                          (opts.convert_dst ? 1 : handle->area[area_idx].area_toc->frame_format != FRAME_FORMAT_DST), last_track);
                                        }
                                        free(file_path);
                                        free(musicfilename);
                                    
                                    }
                                }
                                else  // no tracks specified
                                {
                                    fwprintf(stdout, L"\n\n Warning! Concatenation activated but no tracks selected!\n");
                                }                                                                                                  
                            }                          

                            print_start_time();

                            LOG(lm_main, LOG_NOTICE, ("Start processing dsf/dff files"));
                            scarletbook_output_start(output);
                            LOG(lm_main, LOG_NOTICE, ("Start destroy dsf/dff"));
                            scarletbook_output_destroy(output);
                            LOG(lm_main, LOG_NOTICE, ("Finish destroy dsf/dff"));
                            
                            print_end_time();

                            if (opts.output_dsf)
                                fwprintf(stdout, L"\n\nWe are done exporting DSF...\n");                       
                            else
                                fwprintf(stdout, L"\n\nWe are done exporting DSDIFF...\n");

                        } // end if (opts.output_dsf || opts.output_dsdiff)

                        
                        if (opts.multi_channel == 1)
                            opts.multi_channel = 0;
                        else if(opts.two_channel == 1)
                            opts.two_channel = 0;

                        free(output_dir_dsd);

                    } // end while opts.two_channel + opts.multi_channel

                }  // end if (opts.output_dsf || opts.output_dsdiff || opts.output_dsdiff_em || opts.export_cue_sheet)

                free(output_dir);
                free(album_filename);
                free(file_path_iso_unique);

                scarletbook_close(handle);

            }  // end if handle
			else
            {
                fwprintf(stdout, L"\nErrors reading sacd data!!\n");
                LOG(lm_main, LOG_ERROR, ("Error in main(), reading sacd data!!"));
				exit_main_flag=-1;
            }
            
			sacd_close(sacd_reader);
        }  // end if (sacd_reader != NULL
		else
        {
            fwprintf(stdout, L"\nErrors opening sacd !!\n");
            LOG(lm_main, LOG_ERROR, ("Error in main(), opening sacd !!"));
			exit_main_flag=-1;
        }

        

exit_main:
    fwprintf(stdout, L"\nProgram terminates!\n");
    LOG(lm_main, LOG_NOTICE, ("NOTICE in main:Program terminates!"));
#ifndef _WIN32
            if(freopen(NULL, "w", stdout) == NULL)
            {
                fwprintf(stdout, L"\nError in main: cannot change the mode of the stream to w (re-initialize) !\n");
                LOG(lm_main, LOG_ERROR, ("Error in main: cannot change the mode of the stream to w (re-initialize)!"));
            }
#endif
        if (fwide(stdout, -1) >= 0)
        {
            fprintf(stderr, "ERROR in main: Output not set to byte oriented!\n");
            LOG(lm_main, LOG_ERROR, ("Error in main: Output not set to byte oriented!"));
        }
    }
exit_main_1:
    free_lock(g_fwprintf_lock);
    destroy_logging();

    free(opts.output_dir_base);

	free(opts.output_dir_conc_base);

    free(opts.input_device);

#ifdef PTW32_STATIC_LIB
    pthread_win32_process_detach_np();
    pthread_win32_thread_detach_np();
#endif

    printf("\n");
    
#if defined(WIN32) || defined(_WIN32)
     for (int t=0; t < argc;t++)
	 {
		 free(argvw_utf8[t]);		 
	 }
	 free(argvw_utf8);
	 
#endif
	
    return exit_main_flag;
}
