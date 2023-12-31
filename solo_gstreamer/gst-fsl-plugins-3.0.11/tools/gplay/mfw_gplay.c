/*
 * Copyright (C) 2009-2014 Freescale Semiconductor, Inc. All rights reserved.
 *
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <termio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include "mfw_gplay_core.h"
#include "playlist.h"
#include <string.h>

#include <signal.h>
#include <unistd.h>
#include <errno.h>
#define PRINT printf

typedef enum{
    FSL_PLAYER_REPEAT_NONE = 0,
    FSL_PLAYER_REPEAT_PLAYLIST = 1,
    FSL_PLAYER_REPEAT_CURRENT = 2,
    FSL_PLAYER_REPEAT_NUMBER = 3
} repeat_mode;

typedef struct{
    fsl_player_s32 repeat;
    void * pl;
    PlayItem * current;

    fsl_player_u32  info_interval_in_sec;
    fsl_player_bool quiet;
    fsl_player_bool handle_buffering;
    fsl_player_bool handle_redirect;
    fsl_player_bool enable_visual;
    
    int play_times;

} options;


options oopt = {0};
options * opt=&oopt;
static fsl_player_bool gbexit_display_thread = FSL_PLAYER_FALSE;
static fsl_player_bool gbexit_msg_thread = FSL_PLAYER_FALSE;
static fsl_player_bool gbdisplay = FSL_PLAYER_TRUE;
static fsl_player_bool gbexit_main = FSL_PLAYER_FALSE;
fsl_player* g_pplayer;


void alarm_signal(int signum)
{
    //printf("%s(): Time out: signum=%d\n", __FUNCTION__, signum);
}

fsl_player_ret_val player_exit(fsl_player_handle handle)
{
    fsl_player* pplayer = NULL;
    pplayer = (fsl_player*)handle;
    struct sigaction act;

    pplayer->klass->stop(pplayer);
    pplayer->klass->exit_message_loop(pplayer); // flush all messages left in the message queue.
    //pplayer->klass->send_message_exit(pplayer); // send a exit message.
    gbexit_main = FSL_PLAYER_TRUE;
    gbexit_msg_thread = FSL_PLAYER_TRUE;
    //_close_nolock(STDIN_FILENO); // no use for linux, but effective for win32

    // Register alarm handler for alarm
    act.sa_handler = &alarm_signal;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);
    // Set timer for 1 seconds
    alarm(1);

    printf("%s(): No more multimedia files, exit.\n", __FUNCTION__);

    return FSL_PLAYER_SUCCESS;
}

PlayItem * playlist_next(fsl_player_handle handle, options* opt)
{
    fsl_player* pplayer = NULL;
    pplayer = (fsl_player*)handle;
    fsl_player_drm_format drm_format;
    PlayItem * current = opt->current;
    PlayItem * next = getNextItem(current);

    switch( opt->repeat )
    {
        case FSL_PLAYER_REPEAT_NONE:
        {
            if( next )
            {
                opt->current = next;
                printf("%s\n", opt->current->name);
                pplayer->klass->stop(pplayer);
                pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
                pplayer->klass->play(pplayer);
            }
            else
            {
                //gcore_register_callback(gcore, NULL, NULL,0);
                // if not repeated mode or no next item, exit.
                //printf("%s(): No more multimedia files, exit.\n", __FUNCTION__);
                //player_exit(pplayer);
            }
            break;
        }
        case FSL_PLAYER_REPEAT_PLAYLIST:
        {
            if( next )
            {
                opt->current = next;
                printf("%s\n", opt->current->name);
                pplayer->klass->stop(pplayer);
                pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
                pplayer->klass->play(pplayer);
            }
            else
            {
                if (opt->play_times>0){
                  opt->play_times--;
                  if (opt->play_times==0) break;
                }
                next = getFirstItem(opt->pl);
                opt->current = next;
                pplayer->klass->stop(pplayer);
                pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
                pplayer->klass->play(pplayer);
            }
            break;
        }
        case FSL_PLAYER_REPEAT_CURRENT:
        {
            pplayer->klass->stop(pplayer);
            //pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
            pplayer->klass->play(pplayer);
            next = opt->current;
            break;
        }
        default:
        {
            break;
        }
    }
    return next;
}

fsl_player_ret_val playlist_previous(fsl_player_handle handle, options* opt)
{
    fsl_player* pplayer = NULL;
    pplayer = (fsl_player*)handle;
    fsl_player_drm_format drm_format;
    PlayItem * current = opt->current;
    PlayItem * next = getPrevItem(current);

    if(next)
    {
        opt->current = next;
        printf("%s\n", opt->current->name);
        pplayer->klass->stop(pplayer);
        pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
        pplayer->klass->play(pplayer);
    }
    switch( opt->repeat )
    {
        case FSL_PLAYER_REPEAT_NONE:
        {
            if( next )
            {
                opt->current = next;
                printf("%s\n", opt->current->name);
                pplayer->klass->stop(pplayer);
                pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
                pplayer->klass->play(pplayer);
            }
            else
            {
                //gcore_register_callback(gcore, NULL, NULL,0);
                // if not repeated mode or no next item, exit.
                //printf("%s(): No more multimedia files, exit.\n", __FUNCTION__);
                //player_exit(pplayer);
            }
            break;
        }
        case FSL_PLAYER_REPEAT_PLAYLIST:
        {
            if( next )
            {
                opt->current = next;
                printf("%s\n", opt->current->name);
                pplayer->klass->stop(pplayer);
                pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
                pplayer->klass->play(pplayer);
            }
            else
            {
                next = getLastItem(opt->pl);
                opt->current = next;
                pplayer->klass->stop(pplayer);
                pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
                pplayer->klass->play(pplayer);
            }
            break;
        }
        case FSL_PLAYER_REPEAT_CURRENT:
        {
            pplayer->klass->stop(pplayer);
            //pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
            pplayer->klass->play(pplayer);
            break;
        }
        default:
        {
            break;
        }
    }
    return FSL_PLAYER_SUCCESS;
}

fsl_player_s32 parse_options(fsl_player_config* pconfig, options * opt, fsl_player_s32 argc, fsl_player_s8* argv[])
{
    fsl_player_s32 ret = -1;
    fsl_player_s32 i;

    opt->pl = createPlayList(NULL);
    if (opt->pl==NULL){
        printf("Can not create Playlist!!\n");
        goto err;
    }

    opt->play_times = -1;

    for (i=1;i<argc;i++){
        if (strlen(argv[i])) {
            if (argv[i][0]=='-'){
                if ((strcmp(argv[i], "-h")==0)||(strcmp(argv[i], "--help")==0)){
                    printf("Usage of command line player:\n");
                    printf("    %s file_list\n", argv[0]);
                    ret = 1;
                    goto err;
                }

                if ((strcmp(argv[i], "--playbin")==0)){
                    pconfig->playbin_version = 1;
                    continue;
                }

                if ((strncmp(argv[i], "--visual", 8)==0)){
                    opt->enable_visual = 1;
                    if (argv[i][8]=='='){
                      pconfig->visual_name = &(argv[i][9]);
                    }
                    continue;
                }

                if (strcmp(argv[i], "--quiet")==0){
                    opt->info_interval_in_sec = 0;
                    continue;
                }

                if (strncmp(argv[i], "-r", 2)==0){
                  if (argv[i][2]=='='){
                    opt->play_times = atoi(&(argv[i][3]));
                  }
                  opt->repeat = FSL_PLAYER_REPEAT_PLAYLIST;
                  continue;
                }

                if (strncmp(argv[i], "--repeat", 8)==0){
                  if (argv[i][8]=='='){
                    opt->play_times = atoi(&(argv[i][9]));
                  }
                  opt->repeat = FSL_PLAYER_REPEAT_PLAYLIST;
                  continue;
                }

                if (strncmp(argv[i], "--timeout", 9)==0){
                  if (argv[i][9]=='='){
                    pconfig->timeout_second = atoi(&(argv[i][10]));
                  }
                  continue;
                }

                if ((strncmp(argv[i], "--video-sink", 12)==0)){
                  if (argv[i][12]=='='){
                    pconfig->video_sink_name = (&(argv[i][13]));
                  }
                  continue;
                }

                if ((strncmp(argv[i], "--audio-sink", 12)==0)){
                  if (argv[i][12]=='='){
                    pconfig->audio_sink_name = (&(argv[i][13]));
                  }
                  continue;
                }


                if ((strncmp(argv[i], "--info-interval", 14)==0)){
                  if (argv[i][14]=='='){
                    opt->info_interval_in_sec = atoi(&(argv[i][15]));
                  }
                  continue;
                }

                if ((strcmp(argv[i], "--handle-buffering")==0)){
                    opt->handle_buffering = 1;
                    pconfig->features &= (~GPLAYCORE_FEATURE_AUTO_BUFFERING);
                    continue;
                }

                if ((strcmp(argv[i], "--fade")==0)){
                    pconfig->features |= (GPLAYCORE_FEATURE_AUDIO_FADE);
                    continue;
                }


                if ((strcmp(argv[i], "--handle-redirect")==0)){
                    opt->handle_redirect= 1;
                    pconfig->features &= (~ GPLAYCORE_FEATURE_AUTO_REDIRECT_URI);
                    continue;
                }

                continue;
            }else{

              addItemAtTail(opt->pl, argv[i],0);
              continue;
            }
        }

    }

    opt->current=getFirstItem(opt->pl);
    if (opt->current==NULL){
        printf("NO File specified!!\n");
        goto err;
    }

    ret = 0;
    return ret;

err:
    if (opt->pl){
        destroyPlayList(opt->pl);
        opt->pl=NULL;
    }
    return ret;
}

void print_help(fsl_player_handle handle)
{
    fsl_player_s8* str_version = NULL;
    fsl_player* pplayer = (fsl_player*)handle;
    pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_VERSION, (void*)(&str_version));
    PRINT("\n%s\n", (str_version?str_version:"FSL_PLAYER_UNKNOWN_VERSION"));
    PRINT("\t[h]display the operation Help\n");
	  PRINT("\t[p]Play\n");
    PRINT("\t[s]Stop\n");
	  PRINT("\t[e]Seek\n");
    PRINT("\t[a]Pause when playing, play when paused\n");

    PRINT("\t[v]Volume\n");
    PRINT("\t[m]Switch to mute or not\n");
    PRINT("\t[>]Play next file\n");
    PRINT("\t[<]Play previous file\n");
    PRINT("\t[r]Switch to repeated mode or not\n");
//    PRINT("\t[n]Get the current video snapshot while playing\n");
//    PRINT("\t[o]Set video output mode(LCD,NTSC,PAL,LCD&NTSC,LCD&PAL)\n");
//    PRINT("\t[d]Select the audio track\n");
//    PRINT("\t[b]Select the subtitle\n");
    PRINT("\t[f]Set full screen or not\n");
    PRINT("\t[z]resize the width and height\n");
//    PRINT("\t[k]Set video input crop\n");
    PRINT("\t[t]Rotate\n");
    PRINT("\t[c]Setting play rate\n");

//    PRINT("\t[c]playing direction and speed control\n");
    PRINT("\t[i]Display the metadata\n");
    PRINT("\t[x]eXit\n");
}

void print_metadata(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_u64 duration=0;
    fsl_player_metadata metadata;
    pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_METADATA, (void*)(&metadata));
    pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_DURATION, (void*)(&duration));

    PRINT("\nMetadata of File: %s\n", metadata.currentfilename);
    PRINT("\tTitle: %s\n", metadata.title);
    PRINT("\tAritist: %s\n", metadata.artist);
    PRINT("\tAlbum: %s\n", metadata.album);
    PRINT("\tYear: %s\n", metadata.year);
    PRINT("\tGenre: %s\n", metadata.genre);
    PRINT("\tDuration: %d seconds\n", (fsl_player_s32)(duration/1000000000));
    PRINT("Video:\n");
    PRINT("\tWidth: %d\n", metadata.width);
    PRINT("\tHeight: %d\n", metadata.height);
    PRINT("\tFrame Rate: %d\n", metadata.framerate);
    PRINT("\tBitrate: %d\n", metadata.videobitrate);
    PRINT("\tCodec: %s\n", metadata.videocodec);
    PRINT("Audio:\n");
    PRINT("\tChannels: %d\n", metadata.channels);
    PRINT("\tSample Rate: %d\n", metadata.samplerate);
    PRINT("\tBitrate: %d\n", metadata.audiobitrate);
    PRINT("\tCodec: %s\n", metadata.audiocodec);
    return;
}

fsl_player_s32 display_thread_fun(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_s8 str_player_state[FSL_PLAYER_STATUS_NUMBER][20] = {"Stopped  ", "Paused   ", "Playing  ", "FF ", "SF ", "FB"};
    fsl_player_s8 str_repeated_mode[FSL_PLAYER_REPEAT_NUMBER][20] = {"", "(List Repeated)", "(Current Repeated)"};
    while(1)
    {
        if( FSL_PLAYER_TRUE == gbexit_display_thread )
        {
            return 0;
        }
        if( FSL_PLAYER_TRUE == gbdisplay )
        {
            fsl_player_s8 str_player_state_rate[16];
            fsl_player_s8 str_volume[16];
            fsl_player_s8* prepeated_mode = &(str_repeated_mode[opt->repeat][0]);
            fsl_player_u64 hour, minute, second;
            fsl_player_u64 hour_d, minute_d, second_d;
            fsl_player_u64 duration=0;
            fsl_player_u64 elapsed=0;
            fsl_player_state player_state = FSL_PLAYER_STATUS_STOPPED;
            double playback_rate = 0.0;
            fsl_player_bool bmute = 0;
            double volume = 0.0;
            fsl_player_u32 total_frames = 0;
            pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_DURATION, (void*)(&duration));
            pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_ELAPSED, (void*)(&elapsed));
            pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_PLAYER_STATE, (void*)(&player_state));
            pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_PLAYBACK_RATE, (void*)(&playback_rate));
            pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_MUTE, (void*)(&bmute));
            pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_VOLUME, (void*)(&volume));
            pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_TOTAL_FRAMES, (void*)(&total_frames));

            if (player_state == FSL_PLAYER_STATUS_PLAYING)
            {
                if( playback_rate > 1.0 && playback_rate <= 8.0 )
                {
                    player_state = FSL_PLAYER_STATUS_FASTFORWARD;
                }
                if( playback_rate > 0.0 && playback_rate < 1.0 )
                {
                    player_state = FSL_PLAYER_STATUS_SLOWFORWARD;
                }
                else if( playback_rate >= -8.0 && playback_rate <= -0.0/*-1.0*/ )
                {
                    player_state = FSL_PLAYER_STATUS_FASTBACKWARD;
                }
            }

            hour = (elapsed/ (fsl_player_u64)3600000000000);
            minute = (elapsed / (fsl_player_u64)60000000000) - (hour * 60);
            second = (elapsed / 1000000000) - (hour * 3600) - (minute * 60);
            hour_d = (duration/ (fsl_player_u64)3600000000000);
            minute_d = (duration / (fsl_player_u64)60000000000) - (hour_d * 60);
            second_d = (duration / 1000000000) - (hour_d * 3600) - (minute_d * 60);
            switch( player_state )
            {
                case FSL_PLAYER_STATUS_STOPPED:
                    sprintf(str_player_state_rate, "%s", str_player_state[player_state]);
                    break;
                case FSL_PLAYER_STATUS_PAUSED:
                    sprintf(str_player_state_rate, "%s", str_player_state[player_state]);
                    break;
                case FSL_PLAYER_STATUS_PLAYING:
                    sprintf(str_player_state_rate, "%s", str_player_state[player_state]);
                    break;
                case FSL_PLAYER_STATUS_FASTFORWARD:
                    sprintf(str_player_state_rate, "%s(%1.1fX)", str_player_state[player_state], playback_rate);
                    break;
                case FSL_PLAYER_STATUS_SLOWFORWARD:
                    sprintf(str_player_state_rate, "%s(%1.1fX)", str_player_state[player_state], playback_rate);
                    break;
                case FSL_PLAYER_STATUS_FASTBACKWARD:
                    sprintf(str_player_state_rate, "%s(%1.1fX)", str_player_state[player_state], playback_rate);
                    break;
                default:
                    break;
            }
            if( bmute )
            {
                sprintf(str_volume, "%s", "MUTE  ");
            }
            else
            {
                sprintf(str_volume, "Vol=%02d", (fsl_player_s32)(volume));
            }
            /* Get the real fps value */
            if ((hour*3600+minute*60+second) > 0 )
                printf("\r[%s%s][%s][%02d:%02d:%02d/%02d:%02d:%02d][fps:%.0f]",
                           str_player_state_rate/*str_player_state[player_state]*/,prepeated_mode/*(brepeated?"(Repeated)":"")*/, str_volume,
                           (fsl_player_s32)hour, (fsl_player_s32)minute, (fsl_player_s32)second,
                           (fsl_player_s32)hour_d, (fsl_player_s32)minute_d, (fsl_player_s32)second_d,
                           (float)(total_frames/(hour*3600+minute*60+second)));
            else
                printf("\r[%s%s][%s][%02d:%02d:%02d/%02d:%02d:%02d][fps:%.0f]",
                           str_player_state_rate/*str_player_state[player_state]*/,prepeated_mode/*(brepeated?"(Repeated)":"")*/, str_volume,
                           (fsl_player_s32)hour, (fsl_player_s32)minute, (fsl_player_s32)second,
                           (fsl_player_s32)hour_d, (fsl_player_s32)minute_d, (fsl_player_s32)second_d,
                           0);

            fflush (stdout);
        }
		FSL_PLAYER_SLEEP(opt->info_interval_in_sec*1000);
    }
    return 0;
}

fsl_player_s32 msg_thread_fun(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_ui_msg * msg;

    while(1)
    {
        if( FSL_PLAYER_TRUE == gbexit_msg_thread )
        {
            return 0;
        }

        msg = NULL;
        if( FSL_PLAYER_SUCCESS == pplayer->klass->wait_message( handle, &msg, FSL_PLAYER_FOREVER) )
        {
            switch( msg->msg_id )
            {
                case FSL_PLAYER_UI_MSG_EOS:
                {
                    printf("FSL_PLAYER_UI_MSG_EOS\n");
                    pplayer->klass->stop(handle);
                    // Wait until eos message has been processed completely. Fix the bug of play/stop hang problem.
                    pplayer->klass->post_eos_semaphore(handle);
                    //printf("\nmsg_thread_fun:=======================================================\n");
                    if (playlist_next(handle, opt)==NULL){
                      player_exit(handle);
                    }
                    fsl_player_ui_msg_free(msg);
                    break;
                }
                case FSL_PLAYER_UI_MSG_EXIT:
                {
                    printf("FSL_PLAYER_UI_MSG_EXIT\n");
                    player_exit(handle);
                    fsl_player_ui_msg_free(msg);
                    return 0;
                }
                case FSL_PLAYER_UI_MSG_INTERNAL_ERROR:
                {
                    printf("FSL_PLAYER_UI_MSG_INTERNAL_ERROR: internal error message received.\n");
                    fsl_player_ui_msg_free(msg);
                    player_exit(handle);
                    return 0;
                }
                case FSL_PLAYER_UI_MSG_BUFFERING:
                {
                    fsl_player_ui_msg_body_buffering * pbody = (fsl_player_ui_msg_body_buffering *)(msg->msg_body);

                    if (opt->handle_buffering){
                        if (pbody->percent==0){
                            pplayer->klass->pause(pplayer);
                        }else if (pbody->percent>=100){
                            pplayer->klass->pause(pplayer);

                        }
                    }
                    fsl_player_ui_msg_free(msg);
                    break;
                 }
                default:
                {
                    fsl_player_ui_msg_free(msg);
                    break;
                }
            }


        }
    }

    return 0;
}

static struct termio term_backup;
static void kb_set_raw_term (fsl_player_s32 fd)
{
#if 0 // for convinience of test team.
    struct termio s;

    (void) ioctl (fd, TCGETA, &s);
    term_backup = s;

    s.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);
    s.c_oflag |= (OPOST | ONLCR | TAB3);
    s.c_oflag &= ~(OCRNL | ONOCR | ONLRET);
    s.c_cc[VMIN] = 1;
    s.c_cc[VTIME] = 0;
    (void) ioctl (fd, TCSETAW, &s);
#endif
}

static void kb_restore_term (fsl_player_s32 fd)
{
#if 0 // for convinience of test team.
    struct termio s;
    s = term_backup;
    (void) ioctl (fd, TCSETAW, &s);
#endif
}

void eos_callback(void *data)
{
    fsl_player_bool brepeated = FSL_PLAYER_FALSE;
    fsl_player_handle player_handle = (fsl_player_handle)data;
    fsl_player* pplayer = (fsl_player*)player_handle;

    if( NULL == data )
    {
        return;
    }

    pplayer->klass->stop(pplayer);
    switch( opt->repeat )
    {
        case FSL_PLAYER_REPEAT_NONE:
        {
            break;
        }
        case FSL_PLAYER_REPEAT_PLAYLIST:
        {
            playlist_next(pplayer, opt);
            break;
        }
        case FSL_PLAYER_REPEAT_CURRENT:
        {
            pplayer->klass->play(pplayer);
            break;
        }
        default:
        {
            break;
        }
    }
    return;
}
static void signal_handler(int sig)
{
    int ret = 0;
    printf(" Aborted by signal[%d] Interrupt...\n", sig);    

    {
        fsl_player_s8 *gts_log = getenv("GAT_LOG");
        if (gts_log != NULL) {
            if (strstr (gts_log, "gplay") != NULL) {
                if ((SIGILL == sig) || (SIGFPE == sig) || (SIGSEGV == sig))
                {
                    ret = sig;
                    kill(getppid(), sig);
                }
            }
        }
    }
    
    if(g_pplayer != NULL){
        gbexit_main = FSL_PLAYER_TRUE;
        g_pplayer->klass->send_message_exit(g_pplayer);
        }
}

int main(int argc,char *argv[])
{
    fsl_player_s8 sCommand[128];
    fsl_player_s8 uri_buffer[500];
    fsl_player_thread_t display_thread;
    fsl_player_thread_t msg_thread;
    fsl_player_ret_val ret_val = FSL_PLAYER_SUCCESS;
    fsl_player_handle player_handle = NULL;
    fsl_player* pplayer = NULL;
    fsl_player_drm_format drm_format;
    fsl_player_config config;
    fsl_player_s32 ret;
    fsl_player_s32 volume = 1;

    struct sigaction act;
    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    if( argc < 2 ) {
        printf("Usage of command line player:\n");
        printf("    %s file_list\n", argv[0]);
        goto bail;
    }

    // Default playbin.
    memset(&config, 0, sizeof(fsl_player_config));
    config.playbin_version = 2;
    config.api_version = GPLAYCORE_API_VERSION;
    config.features = (GPLAYCORE_FEATURE_AUTO_BUFFERING|GPLAYCORE_FEATURE_AUTO_REDIRECT_URI);
    config.timeout_second = GPLAYCORE_DEFAULT_TIMEOUT_SECOND;
    
    opt->info_interval_in_sec = 1;

    ret = parse_options(&config, opt, argc, argv);
    if( ret )
    {
        return -1;
    }

    fsl_player_element_property property[1];

    property[0].type = ELEMENT_TYPE_PLAYBIN;
    property[0].property_type = ELEMENT_PROPERTY_TYPE_INT;
    property[0].property_name = "flags";
    if (opt->enable_visual){
      property[0].value_int = 0x5f; /* default+native_video+buffering+visual */
    }else{
      property[0].value_int = 0x57; /* default+native_video+buffering */
    }
    property[0].next = NULL;
    config.ele_properties = property;

    if ((config.playbin_version==1)&&(config.video_sink_name==NULL)){
        config.video_sink_name = "autovideosink";
    }

    player_handle = fsl_player_init(&config);
    if( NULL == player_handle )
    {
        PRINT("Failed: player_handle == NULL returned by fsl_player_init().\n");
        goto bail;
    }

    pplayer = (fsl_player*)player_handle;
    g_pplayer = pplayer;

    if (opt->info_interval_in_sec){
        FSL_PLAYER_CREATE_THREAD( &display_thread, display_thread_fun, player_handle );
    }
    FSL_PLAYER_CREATE_THREAD( &msg_thread, msg_thread_fun, player_handle );

    // Play the multimedia file directory after starting command line player.
    pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
    pplayer->klass->play(pplayer);

    // Print the help menu to let user know the operation.
    print_help(player_handle);

    kb_set_raw_term(STDIN_FILENO);

    while( FSL_PLAYER_FALSE == gbexit_main )
    {
        sCommand[0] = ' ';
        errno = 0;
          scanf("%s", sCommand);
          //read(STDIN_FILENO, sCommand, 1);
          if( EINTR == errno )
          {
            //printf("Timed out: EINTR == %d", errno);
          }
        switch( sCommand[0] )
        {
            case 'h': // display the operation Help.
                print_help(player_handle);
                break;

            case 'p': // Play.
                {
                    fsl_player_state player_state = FSL_PLAYER_STATUS_STOPPED;
                    pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_PLAYER_STATE, (void*)(&player_state));
                    if (player_state == FSL_PLAYER_STATUS_STOPPED) {
                        pplayer->klass->set_media_location(pplayer, opt->current->name, &drm_format);
                        pplayer->klass->play(pplayer);
                    }
                }
                break;

            case 's': // Stop.
                pplayer->klass->stop(pplayer);
                break;

            case 'a': // pAuse when playing, play when paused.
                pplayer->klass->pause(pplayer);
                break;

            case 'e': // sEek.
            {
            #if 0
                fsl_player_u32 seek_point_sec = 0;
                fsl_player_u64 duration_ns = 0;
                fsl_player_u32 duration_sec = 0;
                pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_DURATION, (void*)(&duration_ns));
                duration_sec = duration_ns / 1000000000;
                PRINT("Set seek point between [0,%u] seconds:", duration_sec);
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%s",sCommand);
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                seek_point_sec = atoi(sCommand);
                if( seek_point_sec<0 || seek_point_sec>duration_sec  )
                {
                    printf("Invalid seek point!\n");
                    break;
                }
                pplayer->klass->seek(pplayer, seek_point_sec*1000);
                break;
            #else
                fsl_player_u32 seek_point_sec = 0;
                fsl_player_u64 duration_ns = 0;
                fsl_player_u32 duration_sec = 0;
                fsl_player_u32 seek_portion = 0;
                fsl_player_u32 seek_mode = 0;
                fsl_player_u32 flags = 0;
                fsl_player_bool seekable = 0;
                gbdisplay = FSL_PLAYER_FALSE;
                pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_SEEKABLE, (void*)(&seekable));
                if(!seekable){
                    printf("file is not seekable!\n");
                    gbdisplay = FSL_PLAYER_TRUE;
                    kb_set_raw_term(STDIN_FILENO);
                    break;
                }
                pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_DURATION, (void*)(&duration_ns));
                duration_sec = duration_ns / 1000000000;
                kb_restore_term(STDIN_FILENO);

                PRINT("Select seek mode[Fast seek:0,Accurate seek:1]:");
                scanf("%s",sCommand);
                seek_mode = atoi(sCommand);
                if( seek_mode<0 || seek_mode>1  )
                {
                    printf("Invalid seek mode!\n");
                    break;
                }else{
                    if (seek_mode){
                        flags |= FSL_PLAYER_FLAG_SEEK_ACCURATE;
                    }
                }
                PRINT("%s seek to percentage[0:100] or second [t?]:", seek_mode?"Accurate":"Normal");
                scanf("%s",sCommand);
                if (sCommand[0]=='t'){
                    seek_point_sec = atoi(&sCommand[1]);
                }else{
                    seek_portion = atoi(sCommand);
                    
                    if( seek_portion<0 || seek_portion>100  )
                    {
                        printf("Invalid seek point!\n");
                        break;
                    }
                    seek_point_sec = (fsl_player_u32)(seek_portion * duration_sec / 100);
                }
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                
                pplayer->klass->seek(pplayer, seek_point_sec*1000, flags);
                break;
            #endif
            }

            case 'v': // Volume
            {
                double volume;
                PRINT("Set volume[0-1.0]:");
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%lf",&volume);
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                pplayer->klass->set_volume(pplayer, (volume));
                break;
            }

            case 'm': // Switch to mute or not
                pplayer->klass->mute(pplayer);
                break;

            case '>': // Play next file
                printf("next\n");
                if (playlist_next(pplayer, opt)==NULL){
                    player_exit(pplayer);
                }
                break;

            case '<': // Play previous file
                printf("previous\n");
                playlist_previous(pplayer, opt);
                break;

            case 'r': // Switch to repeated mode or not
            {
                fsl_player_s32 repeated_mode;
                PRINT("input repeated mode[0 for no repeated,1 for play list repeated,2 for current file repeated]:");
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%d",&repeated_mode);
                if( repeated_mode<0 || repeated_mode>2  )
                {
                    printf("Invalid repeated mode!\n");
                }
                else
                {
                    opt->repeat = repeated_mode;
                }
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                //pplayer->klass->repeat(pplayer);
                break;
            }
#if 1
            case 'n': // Get the current video snapshot while playing
                //pplayer->klass->snapshot(pplayer);
                break;

            case 'o': // Set video output mode(LCD,NTSC,PAL,LCD&NTSC,LCD&PAL)
            {
                fsl_player_s32 mode = 0;
                PRINT("Set video output mode(LCD:0,NTSC:1,PAL:2,LCD&NTSC:3,LCD&PAL:4):");
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%d",&mode);
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                if( mode < 0 || mode >4 )
                {
                    printf("Invalid video output mode!\n");
                    break;
                }
                pplayer->klass->set_video_output(pplayer, mode);
                break;
            }

            case 'd': // Select the audio track
            {
                #if 1
                fsl_player_s32 audio_track_no = 0;
                fsl_player_s32 total_audio_no = 0;
                fsl_player_u64 elapsed=0;
                fsl_player_u32 flags = 0;
                pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_TOTAL_AUDIO_NO, (void*)(&total_audio_no));
                PRINT("input audio track number[0,%d]:",total_audio_no-1);
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%d",&audio_track_no);
                if( audio_track_no < 0 || audio_track_no > total_audio_no-1 )
                {
                    printf("Invalid audio track!\n");
                }
                else
                {
                    pplayer->klass->select_audio_track(pplayer, audio_track_no);
                }
                pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_ELAPSED, (void*)(&elapsed));
                pplayer->klass->seek(pplayer, (fsl_player_u32)(elapsed/1000), flags);
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                #endif
                break;
            }

            case 'b': // Select the subtitle
            {
                #if 1
                fsl_player_s32 subtitle_no = 0;
                fsl_player_s32 total_subtitle_no = 0;
                pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_TOTAL_SUBTITLE_NO, (void*)(&total_subtitle_no));
                PRINT("input subtitle number[0,%d]:",total_subtitle_no-1);
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%d",&subtitle_no);
                if( subtitle_no < 0 || subtitle_no > total_subtitle_no-1 )
                {
                    printf("Invalid audio track!\n");
                }
                else
                {
                    pplayer->klass->select_subtitle(pplayer, subtitle_no);
                }
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                #endif
                break;
            }
#endif
            case 'f': // Set full screen or not
                pplayer->klass->full_screen(pplayer);
                break;

            case 'y': // Set display screen mode
            {
                fsl_player_s32 display_screen_mode = 0;
                PRINT("Input screen mode[Normal:0,FullScreen:1,Zoom:2]:");
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%d",&display_screen_mode);
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                if( display_screen_mode < 0 || display_screen_mode > 2 )
                {
                    printf("Invalid display screen mode!\n");
                    break;
                }
                pplayer->klass->display_screen_mode(pplayer, display_screen_mode);
                break;
            }

            case 'z': // resize the width and height
            {
            #if 0
                fsl_player_display_parameter display_parameter;
                fsl_player_s32 width = 720;
                fsl_player_s32 height = 576;
                static fsl_player_s32 resize_index = 0;
                static float width_height_table[] = {0.25, 0.5, 0.75, 1, 2, 3, 4};
                static fsl_player_s32 offset_x_y_table[] = {600, 500, 400, 300, 200, 100, 0};
                if ((++resize_index)==6)
                    resize_index = 0;
                display_parameter.offsetx = 0;//offset_x_y_table[resize_index];
                display_parameter.offsety = 0;//offset_x_y_table[resize_index];
                display_parameter.disp_width = width_height_table[resize_index]*width;
                display_parameter.disp_height = width_height_table[resize_index]*height;
                pplayer->klass->resize(pplayer, display_parameter);
                break;
            #else
                fsl_player_display_parameter display_parameter;
                fsl_player_s8 sCommand_x[128];
                fsl_player_s8 sCommand_y[128];
                fsl_player_s8 sCommand_width[128];
                fsl_player_s8 sCommand_height[128];
                PRINT("Input [x y width height]:");
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%s %s %s %s",sCommand_x,sCommand_y,sCommand_width,sCommand_height);
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                display_parameter.offsetx = atoi(sCommand_x);//offset_x_y_table[resize_index];
                display_parameter.offsety = atoi(sCommand_y);//offset_x_y_table[resize_index];
                display_parameter.disp_width = atoi(sCommand_width);
                display_parameter.disp_height = atoi(sCommand_height);
                if( display_parameter.offsetx < 0 || display_parameter.offsety < 0
                    || display_parameter.disp_width <= 0 || display_parameter.disp_height <= 0 )
                {
                    printf("Invalid resize parameters!\n");
                    break;
                }
                pplayer->klass->resize(pplayer, display_parameter);
                break;
            #endif
            }
            case 'k': // Set video input crop. 
            {
                fsl_player_video_crop sVideoCrop;
                fsl_player_s8 s8Left[128];
                fsl_player_s8 s8Right[128];
                fsl_player_s8 s8Top[128];
                fsl_player_s8 s8Bottom[128];
                pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_VIDEO_CROP, \
                        (void*)(&sVideoCrop));
                PRINT("Current [left: %d,ritht: %d,top: %d,bottom: %d]\n", \
                        sVideoCrop.left, sVideoCrop.right, sVideoCrop.top, sVideoCrop.bottom);
                PRINT("Input [left,ritht,top,bottom]:");
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%s %s %s %s",s8Left,s8Right,s8Top,s8Bottom);
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                sVideoCrop.left = atoi(s8Left);
                sVideoCrop.right = atoi(s8Right);
                sVideoCrop.top = atoi(s8Top);
                sVideoCrop.bottom = atoi(s8Bottom);
                if( sVideoCrop.left < 0 || sVideoCrop.right < 0
                    || sVideoCrop.top < 0 || sVideoCrop.bottom < 0 )
                {
                    printf("Invalid video crop parameters!\n");
                    break;
                }
                pplayer->klass->video_crop(pplayer, sVideoCrop);
                break;
            }
            case 't': // Rotate 90 degree every time
            {
                fsl_player_rotation rotate_value;
                PRINT("Set rotation between 0, 90, 180, 270: ");
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%s",sCommand);
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                rotate_value = (fsl_player_rotation)atoi(sCommand);
                if( (fsl_player_s32)rotate_value != 0  && (fsl_player_s32)rotate_value != 90 &&  (fsl_player_s32)rotate_value != 180 
                &&   (fsl_player_s32)rotate_value != 270 )
                {
                    printf("Invalid rotation value=%d, rotation value should be between [0, 90, 180, 270]\n", rotate_value);
                    break;
                }
                pplayer->klass->rotate(player_handle, rotate_value);
                break;
            }

            case 'c': // playing direction and speed Control.
            {
                double playback_rate;
                PRINT("Set playing speed[-8,-4,-2,0.125,0.25,0.5,1,2,4,8]:");
                kb_restore_term(STDIN_FILENO);
                gbdisplay = FSL_PLAYER_FALSE;
                scanf("%lf",&playback_rate);
                gbdisplay = FSL_PLAYER_TRUE;
                kb_set_raw_term(STDIN_FILENO);
                pplayer->klass->set_playback_rate(pplayer, playback_rate);
                break;
            }

            case 'i': // Display Metadata Information
            {
                print_metadata(player_handle);
                break;
            }

            case 'x': // eXit
            {
                //pplayer->klass->stop(pplayer);
                //pplayer->klass->exit_message_loop(pplayer); // flush all messages left in the message queue.
                //pplayer->klass->send_message_exit(pplayer); // send a exit message.
                gbexit_main = FSL_PLAYER_TRUE;
                //player_exit(pplayer);
                pplayer->klass->send_message_exit(pplayer);
                break;
            }

            case '*': // Sleep 1 seconds
            {
                FSL_PLAYER_SLEEP(1000);
                break;
            }

            case '#': // Sleep 10 seconds
            {
                FSL_PLAYER_SLEEP(10000);
                break;
            }

            case '$': // Check if seek successfully
            {
              fsl_player_u64 elapsed1, elapsed2;
              elapsed1 = elapsed2 = 0;
              pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_ELAPSED, (void*)(&elapsed1));
              FSL_PLAYER_SLEEP(1000);
              pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_ELAPSED, (void*)(&elapsed2));
              if (elapsed2 == elapsed1)
                g_print ("seek failed.\n");
              break;
            }

            case '@': // Check if pause successfully
            {
              fsl_player_u64 elapsed1, elapsed2;
              elapsed1 = elapsed2 = 0;
              pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_ELAPSED, (void*)(&elapsed1));
              FSL_PLAYER_SLEEP(1000);
              pplayer->klass->get_property(pplayer, FSL_PLAYER_PROPERTY_ELAPSED, (void*)(&elapsed2));
              if (elapsed2 != elapsed1)
                g_print ("pause failed.\n");
              break;
            }

            default:
                //printf("Default: Nothing has been done!\n");
                break;
        }
    }
    kb_restore_term (STDIN_FILENO);

    gbexit_display_thread = FSL_PLAYER_TRUE;
    if (opt->info_interval_in_sec){
        FSL_PLAYER_THREAD_JOIN(display_thread);
    }
    FSL_PLAYER_THREAD_JOIN(msg_thread);

    fsl_player_deinit(player_handle);

bail:
    if (opt->pl){
        destroyPlayList(opt->pl);
        opt->pl=NULL;
    }

    return 0;
}


