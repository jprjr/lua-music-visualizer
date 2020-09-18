#include "norm.h"
#if ENABLE_GUI
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iup.h>
#include <iup_config.h>
#include "audio-decoder.h"
#include "audio-processor.h"
#include "video-generator.h"
#include "str.h"
#include "scan.h"
#include "fmt.h"
#include "util.h"
#include "path.h"

#include "jpr_proc.h"

#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

/* paths to search for video players */

static const char* const video_players[] = {
#if defined(JPR_WINDOWS)
    "C:\\Program Files\\MPC-HC\\mpc-hc64.exe",
    "C:\\Program Files (x86)\\MPC-HC\\mpc-hc.exe",
    "C:\\Program Files\\MPC-HC\\mpc-hc.exe",
#elif defined(JPR_APPLE)
    "/usr/local/bin/mpv",
    "/usr/local/bin/ffplay",
#else
    "/usr/bin/mpv",
    "/usr/bin/ffplay",
#endif
    "",
};

static const char* const video_encoders[] = {
#if defined(JPR_UNIX)
    "/usr/local/bin/ffmpeg",
    "/usr/bin/ffmpeg",
#endif
    "",
};


static char *filterString;
static char *filterInfoString;

static audio_decoder *decoder = NULL;
static audio_resampler *resampler = NULL;
static audio_processor *processor = NULL;
static video_generator *generator = NULL;

static Ihandle *dlg;
static Ihandle *config;

static Ihandle *startBox, *startVbox;
static Ihandle *progressBar;
static Ihandle *startButton, *saveButton;

/* "Basics" tab */
static Ihandle *gridbox;
static Ihandle *songBtn, *songText;
static Ihandle *scriptBtn, *scriptText;
static Ihandle *trackLabel, *trackDropdown;

/* "Programs" tab */
static Ihandle *programBox;
static Ihandle *videoplayerBtn, *videoplayerText;
static Ihandle *ffmpegBtn, *ffmpegText;

/* "Video" tab */
static Ihandle *videoBox;
static Ihandle *widthLabel, *widthText;
static Ihandle *heightLabel, *heightText;
static Ihandle *fpsLabel, *fpsDropdown;

/* "Audio" tab */
static Ihandle *audioBox;
static Ihandle *barsLabel, *barsText;

/* "Encoder" tab */
static Ihandle *encoderBox;
static Ihandle *ffmpegArgsLabel, *ffmpegArgsText;

/* "Misc" tab */
static Ihandle *miscBox;
static Ihandle *cwdBtn, *cwdText;
static audio_info *audioInfo;

static char **trackList;

static int probeAudio(const char *filename) {
    audio_decoder *a;
    unsigned int i = 0;
    unsigned int tmp_len;
    char *tmp;
    char **t;
    char f[5];
    audio_info *tmpAudioInfo;

    f[0] = '\0';
    tmp = NULL;

    a = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(a == NULL) abort();
    audio_decoder_init(a);
    tmpAudioInfo = audio_decoder_probe(a,filename);
    free(a);
    if(tmpAudioInfo == NULL) {
        return 0;
    }
    if(audioInfo != NULL) {
        audio_decoder_free_info(audioInfo);
    }

    if(trackList != NULL) {
        t = trackList;
        while(*t != NULL) {
            free(*t);
            t++;
        }
        free(trackList);
        trackList = NULL;
    }

    audioInfo = tmpAudioInfo;

    IupSetAttribute(trackDropdown,"1",NULL);

    if(audioInfo->total > 1) {
        IupSetAttribute(trackDropdown,"ACTIVE","YES");
    } else {
        IupSetAttribute(trackDropdown,"ACTIVE","NO");
    }

    trackList = (char **)malloc(sizeof(char *) * (audioInfo->total + 1));
    if(trackList == NULL) abort();
    trackList[audioInfo->total] = NULL;

    for(i=0;i<audioInfo->total;i++) {
        f[fmt_uint(f,i+1)] = '\0';
        if(tmp != NULL) {
            free(tmp);
            tmp = NULL;
        }
        tmp_len = str_len(f) + 3;
        if(audioInfo->tracks[i].title != NULL && audioInfo->tracks[i].title[0] != '\0') {
            tmp_len += str_len(audioInfo->tracks[i].title);
        } else {
            tmp_len += str_len("(unknown)");
        }
        tmp = (char *)malloc(tmp_len * sizeof(char));
        if(tmp == NULL) abort();
        tmp[0] = '\0';
        str_cat(tmp,f);
        str_cat(tmp,": ");
        if(audioInfo->tracks[i].title != NULL && audioInfo->tracks[i].title[0] != '\0') {
            str_cat(tmp,audioInfo->tracks[i].title);
        } else {
            str_cat(tmp,"(unknown)");
        }
        trackList[i] = str_dup(tmp);
        if(trackList[i] == NULL) abort();
        IupSetAttribute(trackDropdown,f,trackList[i]);
    }

    if(tmp != NULL) free(tmp);
    return 1;
}

static void activateStartButton(void) {
    if(str_len(IupGetAttribute(songText,"VALUE")) == 0) return;
    if(str_len(IupGetAttribute(scriptText,"VALUE")) == 0) return;

    if(str_len(IupGetAttribute(videoplayerText,"VALUE")) != 0) {
        IupSetAttribute(startButton,"ACTIVE","YES");
    }

    if(str_len(IupGetAttribute(ffmpegText,"VALUE")) == 0) return;
    IupSetAttribute(saveButton,"ACTIVE","YES");
}

static const char *findVideoPlayer(void) {
    const char **p = (const char **)video_players;
    while(str_len(*p) > 0) {
        if(path_exists(*p)) return *p;
        p++;
    }
    return *p;
}

static const char *findVideoEncoder(void) {
    const char **p = (const char **)video_encoders;
    while(str_len(*p) > 0) {
        if(path_exists(*p)) return *p;
        p++;
    }
    return *p;
}

static int lazySetTextCB(Ihandle *self, char *filename, int num, int x, int y) {
    (void)num;
    (void)x;
    (void)y;
    IupSetAttribute(self,"VALUE",filename);
    if(self == songText) {
        IupConfigSetVariableStr(config,"global","songfile",filename);
    } else if(self == scriptText) {
        IupConfigSetVariableStr(config,"global","scriptfile",filename);
    } else if(self == videoplayerText) {
        IupConfigSetVariableStr(config,"global","videoplayer",filename);
    } else if(self == cwdText) {
        IupConfigSetVariableStr(config,"global","working directory",filename);
    } else if(self == ffmpegText) {
        IupConfigSetVariableStr(config,"global","ffmpeg",filename);
    }
    IupConfigSave(config);
    activateStartButton();

    return IUP_DEFAULT;
}

static int cwdBtnCb(Ihandle *self) {
    Ihandle *dlg = IupFileDlg();
    IupSetAttribute(dlg,"DIALOGTYPE","DIR");
    IupSetAttribute(dlg,"DIRECTORY",
      IupGetAttribute(cwdText,"VALUE"));
    IupSetAttribute(dlg,"TITLE","Choose a working directory");
    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);
    if(IupGetInt(dlg,"STATUS") == 0) {
        IupSetAttribute(cwdText,"VALUE",IupGetAttribute(dlg,"VALUE"));
        IupConfigSetVariableStr(config,"global","working directory",IupGetAttribute(dlg,"VALUE"));
        IupConfigSave(config);
    }
    IupDestroy(dlg);
	(void)self;

    return IUP_DEFAULT;
}

static int scriptBtnCb(Ihandle *self) {
    Ihandle *dlg = IupFileDlg();
    IupSetAttribute(dlg,"DIALOGTYPE","OPEN");
    IupSetAttribute(dlg,"TITLE","Choose a Lua Script");
    IupSetAttribute(dlg,"FILTER","*.lua");
    IupSetAttribute(dlg,"FILTERINFO","Lua scripts");
    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);
    if(IupGetInt(dlg,"STATUS") == 0) {
        IupSetAttribute(scriptText,"VALUE",IupGetAttribute(dlg,"VALUE"));
        IupConfigSetVariableStr(config,"global","scriptfile",IupGetAttribute(dlg,"VALUE"));
        IupConfigSave(config);
    }
    IupDestroy(dlg);
    activateStartButton();
	(void)self;

    return IUP_DEFAULT;
}

static int songBtnCb(Ihandle *self) {
    Ihandle *dlg = IupFileDlg();
    IupSetAttribute(dlg,"DIALOGTYPE","OPEN");
    IupSetAttribute(dlg,"TITLE","Choose a song");
    IupSetAttribute(dlg,"FILTER",filterString);
    IupSetAttribute(dlg,"FILTERINFO",filterInfoString);
    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);
    if(IupGetInt(dlg,"STATUS") == 0) {
        if(probeAudio(IupGetAttribute(dlg,"VALUE"))) {
            IupSetAttribute(trackDropdown,"VALUE","1");
            IupConfigSetVariableStr(config,"global","tracknum","1");
        }
        IupSetAttribute(songText,"VALUE",IupGetAttribute(dlg,"VALUE"));
        IupConfigSetVariableStr(config,"global","songfile",IupGetAttribute(dlg,"VALUE"));
        IupConfigSave(config);
    }
    IupDestroy(dlg);
	(void)self;
    activateStartButton();

    return IUP_DEFAULT;
}

static int ffmpegBtnCb(Ihandle *self) {

    Ihandle *dlg = IupFileDlg();
    IupSetAttribute(dlg,"DIALOGTYPE","OPEN");
    IupSetAttribute(dlg,"TITLE","Choose ffmpeg.exe");
    IupSetAttribute(dlg,"FILTER","*.exe");
    IupSetAttribute(dlg,"FILTERINFO","Executables");
    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);
    if(IupGetInt(dlg,"STATUS") == 0) {
        IupSetAttribute(ffmpegText,"VALUE",IupGetAttribute(dlg,"VALUE"));
        IupConfigSetVariableStr(config,"global","ffmpeg",IupGetAttribute(dlg,"VALUE"));
        IupConfigSave(config);
    }
    IupDestroy(dlg);
	(void)self;
    activateStartButton();

    return IUP_DEFAULT;
}

static int videoplayerBtnCb(Ihandle *self) {

    Ihandle *dlg = IupFileDlg();
    IupSetAttribute(dlg,"DIALOGTYPE","OPEN");
    IupSetAttribute(dlg,"TITLE","Choose a video player");
    IupSetAttribute(dlg,"FILTER","*.exe");
    IupSetAttribute(dlg,"FILTERINFO","Executables");
    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);
    if(IupGetInt(dlg,"STATUS") == 0) {
        IupSetAttribute(videoplayerText,"VALUE",IupGetAttribute(dlg,"VALUE"));
        IupConfigSetVariableStr(config,"global","videoplayer",IupGetAttribute(dlg,"VALUE"));
        IupConfigSave(config);
    }
    IupDestroy(dlg);
    (void)self;
    activateStartButton();

    return IUP_DEFAULT;
}

static void tearDownGenerator(void) {
    if(decoder != NULL) {
        free(decoder);
        decoder = NULL;
    }
    if(resampler != NULL) {
        free(resampler);
    }
    if(processor != NULL) {
        free(processor);
        processor = NULL;
    }
    if(generator != NULL) {
        free(generator);
        generator = NULL;
    }
}

static int setupVideoGenerator(void) {
    char *workdir = IupGetAttribute(cwdText,"VALUE");
    char *fps_t = IupGetAttribute(fpsDropdown,IupGetAttribute(fpsDropdown,"VALUE"));
    char *width_t = IupGetAttribute(widthText,"VALUE");
    char *height_t = IupGetAttribute(heightText,"VALUE");
    char *bars_t = IupGetAttribute(barsText,"VALUE");
    char *track_t = IupGetAttribute(trackDropdown,"VALUE");
    unsigned int fps, width, height, bars, track;

    scan_uint(fps_t,&fps);
    scan_uint(width_t,&width);
    scan_uint(height_t,&height);
    scan_uint(bars_t,&bars);
    scan_uint(track_t,&track);

    if(width == 0 || height == 0) {
        IupMessageError(dlg,"Unable to figure out the width or height wtf?");
        return IUP_DEFAULT;
    }

    if((width * 3 % 4) != 0) {
        IupMessageError(dlg,"Bad video width - try 640,1280,1920 etc");
        return IUP_DEFAULT;
    }
    if(height % 4 != 0) {
        IupMessageError(dlg,"Bad video height - try 480,720,1080 etc");
        return IUP_DEFAULT;
    }

    if(str_len(workdir) > 0) {
        if(path_setcwd(workdir) != 0) goto videogenerator_fail;
    }

    if(track > 0) track--;

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(UNLIKELY(decoder == NULL)) goto videogenerator_fail;
    resampler = (audio_resampler *)malloc(sizeof(audio_resampler));
    if(UNLIKELY(resampler == NULL)) goto videogenerator_fail;
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(UNLIKELY(processor == NULL)) goto videogenerator_fail;
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(UNLIKELY(generator == NULL)) goto videogenerator_fail;

    decoder->track = 1;
    if(audioInfo != NULL) {
        decoder->track = audioInfo->tracks[track].number;
    }
    generator->width = width;
    generator->height = height;
    generator->fps = fps;
    processor->spectrum_bars = bars;
    resampler->samplerate = 48000;

    return 0;

videogenerator_fail:
    tearDownGenerator();
    return 1;
}

static void startVideoGenerator(const char *songfile, const char *scriptfile, const char *const *args) {
    jpr_proc_info process;
    jpr_proc_pipe child_stdin;
    int t = 0;
    const char *const *arg = args;
    float progress = 0.0f;

    fprintf(stderr,"Running:");
    while(*arg) {
        fprintf(stderr," \"%s\"",*arg);
        arg++;
    }
    fprintf(stderr,"\n");

    if(setupVideoGenerator()) return;

    jpr_proc_info_init(&process);
    jpr_proc_pipe_init(&child_stdin);

    if(jpr_proc_spawn(&process,args,&child_stdin,NULL,NULL)) goto startvideo_cleanup;

    if(video_generator_init(generator,processor,resampler,decoder,1,NULL,songfile,scriptfile,&child_stdin)) {
        LOG_ERROR("error starting the video generator");
        goto startvideo_cleanup;
    }

    while(video_generator_loop(generator,&progress) == 0) {
        IupSetFloat(progressBar,"VALUE",progress);
        if(IupLoopStep() == IUP_CLOSE) {
            t = 1;
            break;
        }
    }

    video_generator_close(generator);

startvideo_cleanup:
    jpr_proc_pipe_close(&child_stdin);
    
    /* wait up to 30 seconds for video encoder to finish */
    if(jpr_proc_info_wait(&process,&t,30) == 2) {
        /* send a term signal, wait 5 seconds */
        jpr_proc_info_term(&process);
        if(jpr_proc_info_wait(&process,&t,5) == 2) {
            /* send a kill signal, something's wrong */
            jpr_proc_info_kill(&process);
            jpr_proc_info_wait(&process,&t,5);
        }
    }

    tearDownGenerator();

    if(t) IupExitLoop();
    return;

}

static int saveButtonCb(Ihandle *self) {
    Ihandle *dlg = IupFileDlg();
    IupSetAttribute(dlg,"DIALOGTYPE","SAVE");
    IupSetAttribute(dlg,"TITLE","Save video as");
    IupSetAttribute(dlg,"EXTDEFAULT","mp4");
    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);
    if(IupGetInt(dlg,"STATUS") < 0) {
        return IUP_DEFAULT;
    }


    char *songfile = IupGetAttribute(songText,"VALUE");
    char *scriptfile = IupGetAttribute(scriptText,"VALUE");
    char *ffmpegfile = IupGetAttribute(ffmpegText,"VALUE");
    char *og_ffmpegargs = IupGetAttribute(ffmpegArgsText,"VALUE");
    char *ffmpegargs = NULL;
    char *f = NULL;

    IupSetFloat(progressBar,"VALUE",0.0f);

    unsigned int total_args = 6;
    unsigned int i = 0;
    char *t;
    char **args = NULL;
    char **a;
    (void)self;

    if(og_ffmpegargs != NULL) {
        ffmpegargs = str_dup(og_ffmpegargs);
    } else {
        ffmpegargs = str_dup("");
    }

    f = ffmpegargs;
    do {
      t = str_chr(f,' ');
      if(t == NULL) t = &f[str_len(f)];
      if(t - f > 0) {
          if(f[0] != ' ' && str_len(f) > 0) {
              total_args++;
          }
          f += (t - f) + 1;
      }
      else {
          f++;
      }
    } while(*f);

    args = (char **)malloc(sizeof(char *)*total_args);
    if(args == NULL) goto cleanshitup_save;
    a = args;
    f = ffmpegargs;

    *a++ = ffmpegfile;
    *a++ = "-i";
    *a++ = "pipe:0";

    do {
      t = str_chr(f,' ');
      if(t == NULL) t = &f[str_len(f)];
      if(t - f > 0 ) {
          *t = '\0';
          if(f[0] != ' ' && str_len(f) > 0) {
              *a++ = f;
          }
          f += (t - f) + 1;
      }
      else {
          f++;
      }
    } while(*f);

    *a++ = "-y";
    *a++ = IupGetAttribute(dlg,"VALUE");
    *a = NULL;

    startVideoGenerator(songfile,scriptfile,(const char *const *)args);

cleanshitup_save:
    if(args != NULL) free(args);
    if(ffmpegargs != NULL) free(ffmpegargs);

    return IUP_DEFAULT;
}

static int startButtonCb(Ihandle *self) {
    char *songfile = IupGetAttribute(songText,"VALUE");
    char *scriptfile = IupGetAttribute(scriptText,"VALUE");
    char *videoplayerfile = IupGetAttribute(videoplayerText,"VALUE");
    char **args;
    char **a;
    (void)self;

    IupSetFloat(progressBar,"VALUE",0.0f);

    args = (char **)malloc(sizeof(char *) * 10);
    if(args == NULL) goto cleanshitup_start;
    a = args;
    *a++ = videoplayerfile;

    if( str_str(videoplayerfile,"vlc") != NULL) {
        *a++ = "--file-caching";
        *a++ = "1500";
        *a++ = "--network-caching";
        *a++ = "1500";
    }
    else {
        *a++ = "-";
    }
    *a = NULL;

    startVideoGenerator(songfile,scriptfile,(const char *const *)args);

cleanshitup_start:
    if(args != NULL) free(args);

    return IUP_DEFAULT;
}

static int updateAndSaveConfig(Ihandle *self) {
    char *sec = NULL;
    char *key = NULL;
    char *val = NULL;
    if(self == fpsDropdown) {
        sec = "video";
        key = "fps";
        val = IupGetAttribute(self, IupGetAttribute(self,"VALUE"));
    }
    else if(self == trackDropdown) {
        sec = "global";
        key = "tracknum";
        val = IupGetAttribute(self,"VALUE");
    }
    else if(self == widthText) {
        sec = "video";
        key = "width";
        val = IupGetAttribute(self,"VALUE");
    }
    else if(self == heightText) {
        sec = "video";
        key = "height";
        val = IupGetAttribute(self,"VALUE");
    }
    else if(self == barsText) {
        sec = "audio";
        key = "visualizer bars";
        val = IupGetAttribute(self,"VALUE");
    }
    else if(self == ffmpegArgsText) {
        sec = "global";
        key = "ffmpeg args";
        val = IupGetAttribute(self,"VALUE");
    }
    if(sec != NULL && key != NULL) {
        IupConfigSetVariableStr(config,sec,key,val);
        IupConfigSave(config);
    }
    activateStartButton();

    return IUP_DEFAULT;
}

static void createAudioBox(void) {
    barsLabel = IupLabel("Visualizer Bars");
    barsText = IupText(NULL);

    IupSetAttribute(barsText,"EXPAND","HORIZONTAL");
    IupSetAttribute(barsText,"VALUE",IupConfigGetVariableStrDef(config,"audio","visualizer bars","24"));
    IupSetAttribute(barsText,"SPINVALUE",IupConfigGetVariableStrDef(config,"audio","visualizer bars","24"));
    IupSetAttribute(barsText,"SPIN","YES");
    IupSetAttribute(barsText,"SPINMIN","0");
    IupSetAttribute(barsText,"SPINMAX","100");
    IupSetAttribute(barsText,"SPININC","1");
    IupSetCallback(barsText,"VALUECHANGED_CB",updateAndSaveConfig);

    audioBox = IupGridBox(barsLabel,barsText,NULL);
    IupSetAttribute(audioBox,"ORIENTATION","HORIZONTAL");
    IupSetAttribute(audioBox,"NUMDIV","2");
    IupSetAttribute(audioBox,"GAPLIN","20");
    IupSetAttribute(audioBox,"GAPCOL","20");
    IupSetAttribute(audioBox,"NORMALIZESIZE","HORIZONTAL");
    IupSetAttribute(audioBox,"MARGIN","20x20");
    IupSetAttribute(audioBox,"TABTITLE","Audio");
}


static void createVideoBox(void) {
    const char *fpsVal;
    fpsLabel = IupLabel("FPS");
    fpsDropdown = IupList(NULL);
    IupSetAttribute(fpsDropdown,"DROPDOWN","YES");
    IupSetAttribute(fpsDropdown,"1","30");
    IupSetAttribute(fpsDropdown,"2","60");
    fpsVal = IupConfigGetVariableStrDef(config,"video","fps","30");
    if(str_cmp(fpsVal,"60") == 0) {
        IupSetAttribute(fpsDropdown,"VALUE","2");
    } else {
        IupSetAttribute(fpsDropdown,"VALUE","1");
    }
    IupSetCallback(fpsDropdown,"ACTION",updateAndSaveConfig);

    widthLabel = IupLabel("Width");
    widthText = IupText(NULL);
    IupSetAttribute(widthText,"EXPAND","HORIZONTAL");
    IupSetAttribute(widthText,"VALUE",IupConfigGetVariableStrDef(config,"video","width","1280"));
    IupSetCallback(widthText,"VALUECHANGED_CB",updateAndSaveConfig);

    heightLabel = IupLabel("Height");
    heightText = IupText(NULL);
    IupSetAttribute(heightText,"EXPAND","HORIZONTAL");
    IupSetAttribute(heightText,"VALUE",IupConfigGetVariableStrDef(config,"video","height","720"));
    IupSetCallback(heightText,"VALUECHANGED_CB",updateAndSaveConfig);

    videoBox = IupGridBox(widthLabel,widthText,heightLabel,heightText,fpsLabel,fpsDropdown,NULL);
    IupSetAttribute(videoBox,"ORIENTATION","HORIZONTAL");
    IupSetAttribute(videoBox,"NUMDIV","2");
    IupSetAttribute(videoBox,"GAPLIN","20");
    IupSetAttribute(videoBox,"GAPCOL","20");
    IupSetAttribute(videoBox,"NORMALIZESIZE","HORIZONTAL");
    IupSetAttribute(videoBox,"MARGIN","20x20");
    IupSetAttribute(videoBox,"TABTITLE","Video");
}

static void createProgramBox(void) {

    videoplayerBtn = IupButton("Player",NULL);
    IupSetCallback(videoplayerBtn,"ACTION",(Icallback) videoplayerBtnCb);

    ffmpegBtn = IupButton("FFMpeg",NULL);
    IupSetCallback(ffmpegBtn,"ACTION",(Icallback) ffmpegBtnCb);

    videoplayerText = IupText(NULL);
    IupSetAttribute(videoplayerText, "EXPAND", "HORIZONTAL");
    IupSetCallback(videoplayerText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetAttribute(videoplayerText,"DROPFILESTARGET","YES");
    IupSetAttribute(videoplayerText,"VALUE",IupConfigGetVariableStrDef(config,"global","videoplayer",""));

    if(str_len(IupGetAttribute(videoplayerText,"VALUE")) == 0) {
        IupSetAttribute(videoplayerText,"VALUE",findVideoPlayer());
        IupConfigSetVariableStr(config,"global","videoplayer",findVideoPlayer());
        IupConfigSave(config);
    }

    ffmpegText = IupText(NULL);
    IupSetAttribute(ffmpegText, "EXPAND", "HORIZONTAL");
    IupSetCallback(ffmpegText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetAttribute(ffmpegText,"DROPFILESTARGET","YES");
    IupSetAttribute(ffmpegText,"VALUE",IupConfigGetVariableStrDef(config,"global","ffmpeg",""));

    if(str_len(IupGetAttribute(ffmpegText,"VALUE")) == 0) {
        IupSetAttribute(ffmpegText,"VALUE",findVideoEncoder());
        IupConfigSetVariableStr(config,"global","ffmpeg",findVideoEncoder());
        IupConfigSave(config);
    }

    programBox = IupGridBox(videoplayerBtn,videoplayerText,ffmpegBtn,ffmpegText,NULL);
    IupSetAttribute(programBox,"ORIENTATION","HORIZONTAL");
    IupSetAttribute(programBox,"NUMDIV","2");
    IupSetAttribute(programBox,"GAPLIN","20");
    IupSetAttribute(programBox,"GAPCOL","20");
    IupSetAttribute(programBox,"NORMALIZESIZE","HORIZONTAL");
    IupSetAttribute(programBox,"MARGIN","20x20");
    IupSetAttribute(programBox,"TABTITLE","Programs");
}

static void createEncoderBox(void) {
    ffmpegArgsLabel = IupLabel("FFMpeg encode flags");
    ffmpegArgsText = IupText(NULL);
    IupSetAttribute(ffmpegArgsText,"EXPAND","HORIZONTAL");
    IupSetAttribute(ffmpegArgsText,"VALUE",IupConfigGetVariableStrDef(config,"global","ffmpeg args","-c:v libx264 -c:a aac -pix_fmt yuv420p -f mp4"));
    IupSetCallback(ffmpegArgsText,"VALUECHANGED_CB",updateAndSaveConfig);

    encoderBox = IupGridBox(ffmpegArgsLabel,ffmpegArgsText,NULL);
    IupSetAttribute(encoderBox,"ORIENTATION","HORIZONTAL");
    IupSetAttribute(encoderBox,"NUMDIV","2");
    IupSetAttribute(encoderBox,"GAPLIN","20");
    IupSetAttribute(encoderBox,"GAPCOL","20");
    IupSetAttribute(encoderBox,"NORMALIZESIZE","HORIZONTAL");
    IupSetAttribute(encoderBox,"MARGIN","20x20");
    IupSetAttribute(encoderBox,"TABTITLE","Encoder");
}

static void createMiscBox(void) {
    cwdBtn = IupButton("Working Directory",NULL);
    IupSetCallback(cwdBtn,"ACTION",(Icallback) cwdBtnCb);

    cwdText = IupText(NULL);
    IupSetAttribute(cwdText,"EXPAND","HORIZONTAL");
    IupSetAttribute(cwdText,"VALUE",IupConfigGetVariableStrDef(config,"global","working directory",""));
    IupSetCallback(cwdText,"DROPFILES_CB",(Icallback) lazySetTextCB);

    miscBox = IupGridBox(cwdBtn,cwdText,NULL);
    IupSetAttribute(miscBox,"ORIENTATION","HORIZONTAL");
    IupSetAttribute(miscBox,"NUMDIV","2");
    IupSetAttribute(miscBox,"GAPLIN","20");
    IupSetAttribute(miscBox,"GAPCOL","20");
    IupSetAttribute(miscBox,"NORMALIZESIZE","HORIZONTAL");
    IupSetAttribute(miscBox,"MARGIN","20x20");
    IupSetAttribute(miscBox,"TABTITLE","Misc");
}

static void createBasicBox(void) {
    const char *songFile;
    const char *trackNumber_t;
    unsigned int trackNumber;
    songBtn = IupButton("Song", NULL);
    songText = IupText(NULL);
    IupSetAttribute(songText, "EXPAND", "HORIZONTAL");
    IupSetCallback(songBtn,"ACTION",(Icallback) songBtnCb);

    trackLabel = IupLabel("Track");
    trackDropdown = IupList(NULL);
    IupSetAttribute(trackDropdown,"EXPAND","HORIZONTAL");
    IupSetAttribute(trackDropdown,"DROPDOWN","YES");
    IupSetAttribute(trackDropdown,"ACTIVE","NO");
    IupSetAttribute(trackDropdown,"1","(no song loaded)");
    IupSetAttribute(trackDropdown,"VALUE","1");
    IupSetCallback(trackDropdown,"ACTION",updateAndSaveConfig);

    scriptBtn = IupButton("Script", NULL);
    scriptText = IupText(NULL);
    IupSetAttribute(scriptText, "EXPAND", "HORIZONTAL");
    IupSetCallback(scriptBtn,"ACTION",(Icallback) scriptBtnCb);

    IupSetCallback(songText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetCallback(scriptText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetAttribute(songText,"DROPFILESTARGET","YES");
    IupSetAttribute(scriptText,"DROPFILESTARGET","YES");

    gridbox = IupGridBox(songBtn,songText,trackLabel,trackDropdown,scriptBtn,scriptText,NULL);
    IupSetAttribute(gridbox,"ORIENTATION","HORIZONTAL");
    IupSetAttribute(gridbox,"NUMDIV","2");
    IupSetAttribute(gridbox,"GAPLIN","20");
    IupSetAttribute(gridbox,"GAPCOL","20");
    IupSetAttribute(gridbox,"NORMALIZESIZE","HORIZONTAL");
    IupSetAttribute(gridbox,"MARGIN","20x20");
    IupSetAttribute(gridbox,"TABTITLE","Basics");

    IupSetAttribute(songText,"VALUE",IupConfigGetVariableStrDef(config,"global","songfile",""));
    IupSetAttribute(scriptText,"VALUE",IupConfigGetVariableStrDef(config,"global","scriptfile",""));

    songFile = IupConfigGetVariableStrDef(config,"global","songfile","");
    if(songFile[0] != '\0') {
        if(probeAudio(songFile)) {
            IupSetAttribute(songText,"VALUE",songFile);
            trackNumber_t = IupConfigGetVariableStrDef(config,"global","tracknum","1");
            scan_uint(trackNumber_t,&trackNumber);
            if(trackNumber <= audioInfo->total) {
                IupSetAttribute(trackDropdown,"VALUE",trackNumber_t);
            } else {
                IupSetAttribute(trackDropdown,"VALUE","1");
            }
        }
        else {
            IupSetAttribute(songText,"VALUE","");
            IupSetAttribute(trackDropdown,"VALUE","1");
        }
    }

}

static void append_string(char **dest, const char *src) {
    char *new;
    unsigned int len = 1;
    len += str_len(src);
    if(*dest != NULL) {
        len += str_len(*dest);
    }
    new = (char *)malloc(len);
    if(new == NULL) abort();
    new[0] = '\0';
    if(*dest != NULL) {
        str_cpy(new,*dest);
        free(*dest);
    }
    str_cat(new,src);
    *dest = new;
}

int gui_start(int argc, char **argv) {
    char **t;
    trackList = NULL;
    IupOpen(&argc, &argv);
    IupSetGlobal("UTF8MODE","YES");
    IupSetGlobal("UTF8MODE_FILE","YES");

    filterString = NULL;
    filterInfoString = NULL;
    audioInfo = NULL;

    int total_formats = 0;

    if(filterString != NULL) append_string(&filterString,";");
    if(filterInfoString != NULL) append_string(&filterInfoString,"/");
    append_string(&filterString,"*.m3u");
    append_string(&filterInfoString,"M3U Files");
    total_formats++;

#if DECODE_FLAC
    if(filterString != NULL) append_string(&filterString,";");
    if(filterInfoString != NULL) append_string(&filterInfoString,"/");
    append_string(&filterString,"*.flac");
    append_string(&filterInfoString,"FLAC");
    total_formats++;
#endif

#if DECODE_MP3
    if(filterString != NULL) append_string(&filterString,";");
    if(filterInfoString != NULL) append_string(&filterInfoString,"/");
    append_string(&filterString,"*.mp3");
    append_string(&filterInfoString,"MP3");
    total_formats++;
#endif

#if DECODE_WAV
    if(filterString != NULL) append_string(&filterString,";");
    if(filterInfoString != NULL) append_string(&filterInfoString,"/");
    append_string(&filterString,"*.wav");
    append_string(&filterInfoString,"WAV");
    total_formats++;
#endif

#if DECODE_NSF
    if(filterString != NULL) append_string(&filterString,";");
    if(filterInfoString != NULL) append_string(&filterInfoString,"/");
    append_string(&filterString,"*.nsf;*.nsfe");
    append_string(&filterInfoString,"NSF");
    total_formats++;
#endif

#if DECODE_SPC
    if(filterString != NULL) append_string(&filterString,";");
    if(filterInfoString != NULL) append_string(&filterInfoString,"/");
    append_string(&filterString,"*.spc");
    append_string(&filterInfoString,"SPC");
    total_formats++;
#endif

#if DECODE_VGM
    if(filterString != NULL) append_string(&filterString,";");
    if(filterInfoString != NULL) append_string(&filterInfoString,"/");
    append_string(&filterString,"*.vgm;*.vgz");
    append_string(&filterInfoString,"VGM");
    total_formats++;
#endif

#if DECODE_NEZ
    if(filterString != NULL) append_string(&filterString,";");
    if(filterInfoString != NULL) append_string(&filterString,"/");
#if DECODE_NSF
    append_string(&filterString,"*.hes;*.kss;*.gbr;*.gbs;*.at;*.sgc;*.nsd");
#else
    append_string(&filterString,"*.nsf;*.nsfe;*.hes;*.kss;*.gbr;*.gbs;*.at;*.sgc;*.nsd");
#endif
    append_string(&filterInfoString,"NSF/NSFE/HES/KSS/GBR/GBS/AY/SGC/NSD");
    total_formats++;
#endif

    if(total_formats > 3) {
        free(filterInfoString);
        filterInfoString = str_dup("Audio Files");
    }

    config = IupConfig();
    IupSetAttribute(config,"APP_NAME","lua-music-visualizer");
    IupConfigLoad(config);

    startButton = IupButton("Play",NULL);
    IupSetCallback(startButton,"ACTION",(Icallback) startButtonCb);
    IupSetAttribute(startButton,"ACTIVE","NO");
    IupSetAttribute(startButton,"PADDING","10x10");
    IupSetAttribute(startButton,"FONT","Arial, 24");

    saveButton = IupButton("Save",NULL);
    IupSetCallback(saveButton,"ACTION",(Icallback) saveButtonCb);
    IupSetAttribute(saveButton,"ACTIVE","NO");
    IupSetAttribute(saveButton,"PADDING","10x10");
    IupSetAttribute(saveButton,"FONT","Arial, 24");

    progressBar = IupProgressBar();

    startBox = IupHbox(IupFill(),startButton,saveButton,IupFill(), NULL);
    startVbox = IupVbox(IupFill(),progressBar,startBox,IupFill(),NULL);
    IupSetAttribute(startBox,"ALIGNMENT","ACENTER");
    IupSetAttribute(startVbox,"ALIGNMENT","ACENTER");

    createBasicBox();
    createProgramBox();
    createVideoBox();
    createAudioBox();
    createEncoderBox();
    createMiscBox();

    activateStartButton();

    dlg = IupDialog(IupVbox(IupTabs(gridbox,programBox,videoBox,audioBox,encoderBox,miscBox,NULL),startVbox,NULL));
    IupSetAttribute(dlg,"TITLE","Lua Music Visualizer");
    IupSetAttribute(dlg,"SIZE","300x170");
    IupShowXY(dlg,IUP_CENTER,IUP_CENTER);
    IupMainLoop();

    IupClose();

    free(filterString);
    free(filterInfoString);

    if(trackList != NULL) {
        t = trackList;
        while(*t != NULL) {
            free(*t);
            t++;
        }
        free(trackList);
    }

    return 0;
}
#else
int gui_start(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return -1;
}
#endif
