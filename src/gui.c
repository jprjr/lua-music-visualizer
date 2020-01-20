#include "norm.h"
#ifdef JPR_WINDOWS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iup.h>
#include <iup_config.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <tchar.h>
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

static audio_decoder *decoder = NULL;
static audio_processor *processor = NULL;
static video_generator *generator = NULL;

static Ihandle *dlg;
static Ihandle *config;

static Ihandle *startBox, *startVbox;
static Ihandle *startButton, *saveButton;

/* "Basics" tab */
static Ihandle *gridbox;
static Ihandle *songBtn, *songText;
static Ihandle *scriptBtn, *scriptText;

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
static Ihandle *outputBtn, *outputText;

/* "Misc" tab */
static Ihandle *miscBox;
static Ihandle *cwdBtn, *cwdText;

static int printable(char *s) {
    unsigned int i = 0;
    while(i < str_len(s)) {
        if(s[i] < 32) return 0;
        i++;
    }
    return 1;
}

static void activateStartButton(void) {
    if(str_len(IupGetAttribute(songText,"VALUE")) == 0) return;
    if(str_len(IupGetAttribute(scriptText,"VALUE")) == 0) return;

    if(str_len(IupGetAttribute(videoplayerText,"VALUE")) != 0) {
        IupSetAttribute(startButton,"ACTIVE","YES");
    }

    if(str_len(IupGetAttribute(outputText,"VALUE")) == 0) return;
    if(str_len(IupGetAttribute(ffmpegText,"VALUE")) == 0) return;
    IupSetAttribute(saveButton,"ACTIVE","YES");
}

static const char* const video_players[] = {
    "C:\\Program Files\\MPC-HC\\mpc-hc64.exe",
    "C:\\Program Files (x86)\\MPC-HC\\mpc-hc.exe",
    "C:\\Program Files\\MPC-HC\\mpc-hc.exe",
    "",
};

static const char *findVideoPlayer(void) {
    const char **p = (const char **)video_players;
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
    } else if(self == outputText) {
        IupConfigSetVariableStr(config,"global","output",filename);
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

static int outputBtnCb(Ihandle *self) {
    Ihandle *dlg = IupFileDlg();
    IupSetAttribute(dlg,"DIALOGTYPE","SAVE");
    IupSetAttribute(dlg,"TITLE","Save video as");
    IupSetAttribute(dlg,"EXTDEFAULT","mp4");
    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);
    if(IupGetInt(dlg,"STATUS") > 0) {
        IupSetAttribute(outputText,"VALUE",IupGetAttribute(dlg,"VALUE"));
        IupConfigSetVariableStr(config,"global","output",IupGetAttribute(dlg,"VALUE"));
        IupConfigSave(config);
    }
    IupDestroy(dlg);
	(void)self;
    activateStartButton();

    return IUP_DEFAULT;
}

static int songBtnCb(Ihandle *self) {
    Ihandle *dlg = IupFileDlg();
    IupSetAttribute(dlg,"DIALOGTYPE","OPEN");
    IupSetAttribute(dlg,"TITLE","Choose a song");
    IupSetAttribute(dlg,"FILTER","*.mp3;*.flac;*.wav");
    IupSetAttribute(dlg,"FILTERINFO","MP3/FLAC/WAV");
    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);
    if(IupGetInt(dlg,"STATUS") == 0) {
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
    unsigned int fps, width, height, bars;
	jpr_uint64 fps_tmp, width_tmp, height_tmp, bars_tmp;

    scan_uint(fps_t,&fps_tmp);
    scan_uint(width_t,&width_tmp);
    scan_uint(height_t,&height_tmp);
    scan_uint(bars_t,&bars_tmp);

	fps = (unsigned int)fps_tmp;
	width = (unsigned int)width_tmp;
	height = (unsigned int)height_tmp;
	bars = (unsigned int)bars_tmp;

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

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(UNLIKELY(decoder == NULL)) goto videogenerator_fail;
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(UNLIKELY(processor == NULL)) goto videogenerator_fail;
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(UNLIKELY(generator == NULL)) goto videogenerator_fail;

    generator->width = width;
    generator->height = height;
    generator->fps = fps;
    processor->spectrum_bars = bars;

    return 0;

videogenerator_fail:
    tearDownGenerator();
    return 1;
}

static void startVideoGenerator(const char *songfile, const char *scriptfile, const char *const *args) {
    jpr_proc_info process;
    jpr_proc_pipe child_stdin;
    int t = 0;

    if(setupVideoGenerator()) return;

    jpr_proc_info_init(&process);
    jpr_proc_pipe_init(&child_stdin);

    if(jpr_proc_spawn(&process,args,&child_stdin,NULL,NULL)) goto startvideo_cleanup;

    if(video_generator_init(generator,processor,decoder,1,NULL,songfile,scriptfile,&child_stdin)) {
        LOG_ERROR("error starting the video generator");
        goto startvideo_cleanup;
    }

    while(video_generator_loop(generator) == 0) {
        if(IupLoopStep() == IUP_CLOSE) {
            t = 1;
            break;
        }
    }

    video_generator_close(generator);

startvideo_cleanup:
    jpr_proc_pipe_close(&child_stdin);
    jpr_proc_info_wait(&process,&t);
    tearDownGenerator();

    if(t) IupExitLoop();
    return;

}

static int saveButtonCb(Ihandle *self) {
    char *songfile = IupGetAttribute(songText,"VALUE");
    char *scriptfile = IupGetAttribute(scriptText,"VALUE");
    char *ffmpegfile = IupGetAttribute(ffmpegText,"VALUE");
    char *outputfile = IupGetAttribute(outputText,"VALUE");
    char *ffmpegargs = IupGetAttribute(ffmpegArgsText,"VALUE");
    char *f = ffmpegargs;
    unsigned int total_args = 6;
    unsigned int i = 0;
    char *t;
    char **args = NULL;
    char **a;
    (void)self;

    do {
      t = str_chr(f,' ');
      if(t - f > 0) {
          /* we may get some extra args this way but no big deal */
          total_args++;
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
      if(t - f > 0 ) {
          *t = '\0';
          if(f[0] != ' ' && str_len(f) > 0 && printable(f)) {
              *a++ = f;
          }
          f += i + 1;
      }
      else {
          f++;
      }
    } while(*f);

    *a++ = "-y";
    *a++ = outputfile;
    *a = NULL;

    startVideoGenerator(songfile,scriptfile,(const char *const *)args);

cleanshitup_save:
    if(args != NULL) free(args);

    return IUP_DEFAULT;
}

static int startButtonCb(Ihandle *self) {
    char *songfile = IupGetAttribute(songText,"VALUE");
    char *scriptfile = IupGetAttribute(scriptText,"VALUE");
    char *videoplayerfile = IupGetAttribute(videoplayerText,"VALUE");
    char **args;
    char **a;
    (void)self;

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
    IupSetAttribute(ffmpegArgsText,"VALUE",IupConfigGetVariableStrDef(config,"global","ffmpeg args","-c:v libx264 -c:a aac"));
    IupSetCallback(ffmpegArgsText,"VALUECHANGED_CB",updateAndSaveConfig);

    outputBtn = IupButton("Save As",NULL);
    IupSetCallback(outputBtn,"ACTION",(Icallback) outputBtnCb);

    outputText = IupText(NULL);
    IupSetAttribute(outputText, "EXPAND", "HORIZONTAL");
    IupSetCallback(outputText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetAttribute(outputText,"DROPFILESTARGET","YES");
    IupSetAttribute(outputText,"VALUE",IupConfigGetVariableStrDef(config,"global","output",""));

    encoderBox = IupGridBox(ffmpegArgsLabel,ffmpegArgsText,outputBtn,outputText,NULL);
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
    songBtn = IupButton("Song", NULL);
    songText = IupText(NULL);
    IupSetAttribute(songText, "EXPAND", "HORIZONTAL");
    IupSetCallback(songBtn,"ACTION",(Icallback) songBtnCb);

    scriptBtn = IupButton("Script", NULL);
    scriptText = IupText(NULL);
    IupSetAttribute(scriptText, "EXPAND", "HORIZONTAL");
    IupSetCallback(scriptBtn,"ACTION",(Icallback) scriptBtnCb);

    IupSetCallback(songText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetCallback(scriptText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetAttribute(songText,"DROPFILESTARGET","YES");
    IupSetAttribute(scriptText,"DROPFILESTARGET","YES");

    gridbox = IupGridBox(songBtn,songText,scriptBtn,scriptText,NULL);
    IupSetAttribute(gridbox,"ORIENTATION","HORIZONTAL");
    IupSetAttribute(gridbox,"NUMDIV","2");
    IupSetAttribute(gridbox,"GAPLIN","20");
    IupSetAttribute(gridbox,"GAPCOL","20");
    IupSetAttribute(gridbox,"NORMALIZESIZE","HORIZONTAL");
    IupSetAttribute(gridbox,"MARGIN","20x20");
    IupSetAttribute(gridbox,"TABTITLE","Basics");

    IupSetAttribute(songText,"VALUE",IupConfigGetVariableStrDef(config,"global","songfile",""));
    IupSetAttribute(scriptText,"VALUE",IupConfigGetVariableStrDef(config,"global","scriptfile",""));

}

int gui_start(int argc, char **argv) {
    IupOpen(&argc, &argv);
    IupSetGlobal("UTF8MODE","YES");
    IupSetGlobal("UTF8MODE_FILE","YES");

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

    startBox = IupHbox(IupFill(),startButton,saveButton,IupFill(), NULL);
    startVbox = IupVbox(IupFill(),startBox,IupFill(),NULL);
    IupSetAttribute(startBox,"ALIGNMENT","ACENTER");
    IupSetAttribute(startVbox,"ALIGNMENT","ACENTER");

    createBasicBox();
    createProgramBox();
    createVideoBox();
    createAudioBox();
    createEncoderBox();
    createMiscBox();

    activateStartButton();

    // dlg = IupDialog(IupTabs(gridbox,videoBox,miscBox,NULL));
    dlg = IupDialog(IupVbox(IupTabs(gridbox,programBox,videoBox,audioBox,encoderBox,miscBox,NULL),startVbox,NULL));
    IupSetAttribute(dlg,"TITLE","Lua Music Visualizer");
    IupSetAttribute(dlg,"SIZE","300x170");
    IupShowXY(dlg,IUP_CENTER,IUP_CENTER);
    IupMainLoop();

    IupClose();

    return 0;
}
#else
int gui_start(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return -1;
}
#endif
