#include <stdio.h>
#include <string.h>
#include <iup.h>
#include <iup_config.h>
#include <windows.h>
#include <shlwapi.h>
#include <io.h>
#include <fcntl.h>
#include <tchar.h>
#include <strsafe.h>
#include "audio-decoder.h"
#include "audio-processor.h"
#include "video-generator.h"
#include "str.h"
#include "scan.h"
#include "fmt.h"

#define JPR_PROC_IMPLEMENTATION
#include "jpr_proc.h"

static audio_decoder *decoder = NULL;
static audio_processor *processor = NULL;
static video_generator *generator = NULL;

static Ihandle *dlg;
static Ihandle *config;

static Ihandle *startBox, *startVbox;
static Ihandle *startButton;

/* "Basics" tab */
static Ihandle *gridbox;
static Ihandle *songBtn, *songText;
static Ihandle *scriptBtn, *scriptText;
static Ihandle *videoplayerBtn, *videoplayerText;

/* "Video" tab */
static Ihandle *videoBox;
static Ihandle *widthLabel, *widthText;
static Ihandle *heightLabel, *heightText;
static Ihandle *fpsLabel, *fpsDropdown;

/* "Audio" tab */
static Ihandle *audioBox;
static Ihandle *barsLabel, *barsText;

/* "Misc" tab */
static Ihandle *miscBox;
static Ihandle *cwdBtn, *cwdText;


static void activateStartButton(void) {
    if(str_len(IupGetAttribute(songText,"VALUE")) == 0) return;
    if(str_len(IupGetAttribute(scriptText,"VALUE")) == 0) return;
    if(str_len(IupGetAttribute(videoplayerText,"VALUE")) == 0) return;
    IupSetAttribute(startButton,"ACTIVE","YES");
}

static const char* const video_players[] = {
    "C:\\Program Files\\MPC-HC\\mpc-hc64.exe",
    "C:\\Program Files (x86)\\MPC-HC\\mpc-hc.exe",
    "",
};

static const char *findVideoPlayer(void) {
    const char **p = (const char **)video_players;
    while(str_len(*p) > 0) {
        if(PathFileExists(*p)) return *p;
        p++;
    }
    return *p;
}

static int dumpValue(Ihandle *self) {
    fprintf(stderr,"%s\n",IupGetAttribute(self,"VALUE"));
    return IUP_DEFAULT;
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
    }
    IupConfigSave(config);
    activateStartButton();

    return IUP_DEFAULT;
}

static int cwdBtnCb(Ihandle *self) {
    (void)self;
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

    return IUP_DEFAULT;
}

static int scriptBtnCb(Ihandle *self) {
    (void)self;
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

    return IUP_DEFAULT;
}

static int songBtnCb(Ihandle *self) {
    (void)self;
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
    activateStartButton();

    return IUP_DEFAULT;
}

static int videoplayerBtnCb(Ihandle *self) {
    (void)self;
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
    activateStartButton();

    return IUP_DEFAULT;
}

static int startButtonCb(Ihandle *self) {
    (void)self;

    char *songfile = IupGetAttribute(songText,"VALUE");
    char *scriptfile = IupGetAttribute(scriptText,"VALUE");
    char *videoplayerfile = IupGetAttribute(videoplayerText,"VALUE");
    char *workdir = IupGetAttribute(cwdText,"VALUE");
    char *fps_t = IupGetAttribute(fpsDropdown,IupGetAttribute(fpsDropdown,"VALUE"));
    char *width_t = IupGetAttribute(widthText,"VALUE");
    char *height_t = IupGetAttribute(heightText,"VALUE");
    char *bars_t = IupGetAttribute(barsText,"VALUE");
    unsigned int fps, width, height, bars;
    char wdir[PATH_MAX];
    char **args = NULL;
    char **a;
    unsigned int r = 0;
    int t = 0;
    jpr_proc_info process;
    jpr_proc_pipe child_stdin;

    scan_uint(fps_t,&fps);
    scan_uint(width_t,&width);
    scan_uint(height_t,&height);
    scan_uint(bars_t,&bars);

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
        str_cpy(wdir,workdir);
    }
    else {
        GetCurrentDirectory(PATH_MAX,wdir);
    }
    if(!SetCurrentDirectory(wdir)) goto cleanshitup;

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(decoder == NULL) goto cleanshitup;
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(processor == NULL) goto cleanshitup;
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(generator == NULL) goto cleanshitup;

    generator->width = width;
    generator->height = height;
    generator->fps = fps;
    processor->spectrum_bars = bars;

    jpr_proc_info_init(&process);
    jpr_proc_pipe_init(&child_stdin);

    args = (char **)malloc(sizeof(char *) * 10);
    if(args == NULL) goto cleanshitup;
    a = args;
    *a++ = videoplayerfile;

    if(strstr(videoplayerfile,"vlc") != NULL) {
        *a++ = "--file-caching";
        *a++ = "1500";
        *a++ = "--network-caching";
        *a++ = "1500";
    }
    else {
        *a++ = "-";
    }
    *a = NULL;

    if(jpr_proc_spawn(&process,(const char * const*)args,&child_stdin,NULL,NULL)) goto cleanshitup;

    if(video_generator_init(generator,processor,decoder,songfile,scriptfile,child_stdin.pipe)) {
        fprintf(stderr,"error starting the video generator\n");
        goto cleanshitup;
    }
    fprintf(stderr,"sending video...\n");

    do {
        r = video_generator_loop(generator);
    } while (r == 0);

    video_generator_close(generator);

cleanshitup:
    jpr_proc_pipe_close(&child_stdin);
    jpr_proc_info_wait(&process,&t);
    if(args != NULL) free(args);

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

    return IUP_DEFAULT;
}

static int updateAndSaveConfig(Ihandle *self) {
    char t[100];
    if(self == fpsDropdown) {
        IupConfigSetVariableStr(config,"video","fps",
          IupGetAttribute(self,
            IupGetAttribute(self,"VALUE")));
    }
    else if(self == widthText) {
        IupConfigSetVariableStr(config,"video","width",
          IupGetAttribute(self,"VALUE"));
    }
    else if(self == heightText) {
        IupConfigSetVariableStr(config,"video","height",
          IupGetAttribute(self,"VALUE"));
    }
    else if(self == barsText) {
        IupConfigSetVariableStr(config,"audio","visualizer bars",
          IupGetAttribute(self,"VALUE"));
    }
    IupConfigSave(config);

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

static void createMiscBox(void) {
    /*
    char wdir[PATH_MAX];
    GetCurrentDirectory(PATH_MAX,wdir) = 0;
    */

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
    char s[PATH_MAX];

    songBtn = IupButton("Song", NULL);
    songText = IupText(NULL);
    IupSetAttribute(songText, "EXPAND", "HORIZONTAL");
    IupSetCallback(songBtn,"ACTION",(Icallback) songBtnCb);

    scriptBtn = IupButton("Script", NULL);
    scriptText = IupText(NULL);
    IupSetAttribute(scriptText, "EXPAND", "HORIZONTAL");
    IupSetCallback(scriptBtn,"ACTION",(Icallback) scriptBtnCb);

    videoplayerBtn = IupButton("Player",NULL);
    videoplayerText = IupText(NULL);
    IupSetAttribute(videoplayerText, "EXPAND", "HORIZONTAL");
    IupSetCallback(videoplayerBtn,"ACTION",(Icallback) videoplayerBtnCb);

    IupSetCallback(songText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetCallback(scriptText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetCallback(videoplayerText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetAttribute(songText,"DROPFILESTARGET","YES");
    IupSetAttribute(scriptText,"DROPFILESTARGET","YES");
    IupSetAttribute(videoplayerText,"DROPFILESTARGET","YES");

    gridbox = IupGridBox(songBtn,songText,scriptBtn,scriptText,videoplayerBtn,videoplayerText,NULL);
    IupSetAttribute(gridbox,"ORIENTATION","HORIZONTAL");
    IupSetAttribute(gridbox,"NUMDIV","2");
    IupSetAttribute(gridbox,"GAPLIN","20");
    IupSetAttribute(gridbox,"GAPCOL","20");
    IupSetAttribute(gridbox,"NORMALIZESIZE","HORIZONTAL");
    IupSetAttribute(gridbox,"MARGIN","20x20");
    IupSetAttribute(gridbox,"TABTITLE","Basics");

    IupSetAttribute(songText,"VALUE",IupConfigGetVariableStrDef(config,"global","songfile",""));
    IupSetAttribute(scriptText,"VALUE",IupConfigGetVariableStrDef(config,"global","scriptfile",""));
    IupSetAttribute(videoplayerText,"VALUE",IupConfigGetVariableStrDef(config,"global","videoplayer",""));

    if(str_len(IupGetAttribute(scriptText,"VALUE")) == 0) {
        if(PathFileExists("Lua/game-that-tune.lua")) {
            _fullpath(s,"Lua/game-that-tune.lua",PATH_MAX);
            IupConfigSetVariableStr(config,"global","scriptfile",s);
            IupSetAttribute(scriptText,"VALUE",s);
            IupConfigSave(config);
        }
    }

    if(str_len(IupGetAttribute(videoplayerText,"VALUE")) == 0) {
        IupSetAttribute(videoplayerText,"VALUE",findVideoPlayer());
        IupConfigSetVariableStr(config,"global","videoplayer",findVideoPlayer());
        IupConfigSave(config);
    }
}

int main(int argc, char **argv) {
    IupOpen(&argc, &argv);

    config = IupConfig();
    IupSetAttribute(config,"APP_NAME","lua-music-visualizer");
    IupConfigLoad(config);

    startButton = IupButton("Start",NULL);
    IupSetCallback(startButton,"ACTION",(Icallback) startButtonCb);
    IupSetAttribute(startButton,"ACTIVE","NO");
    IupSetAttribute(startButton,"PADDING","10x10");
    IupSetAttribute(startButton,"FONT","Arial, 24");

    startBox = IupHbox(IupFill(),startButton,IupFill(), NULL);
    startVbox = IupVbox(IupFill(),startBox,IupFill(),NULL);
    IupSetAttribute(startBox,"ALIGNMENT","ACENTER");
    IupSetAttribute(startVbox,"ALIGNMENT","ACENTER");

    createBasicBox();
    createVideoBox();
    createAudioBox();
    createMiscBox();

    activateStartButton();

    // dlg = IupDialog(IupTabs(gridbox,videoBox,miscBox,NULL));
    dlg = IupDialog(IupVbox(IupTabs(gridbox,videoBox,audioBox,miscBox,NULL),startVbox,NULL));
    IupSetAttribute(dlg,"TITLE","Lua Music Visualizer");
    IupSetAttribute(dlg,"SIZE","300x170");
    IupShowXY(dlg,IUP_CENTER,IUP_CENTER);
    IupMainLoop();

    IupClose();

    return 0;
}
