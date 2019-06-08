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
#define JPR_PROC_IMPLEMENTATION
#include "jpr_proc.h"

static audio_decoder *decoder = NULL;
static audio_processor *processor = NULL;
static video_generator *generator = NULL;
static Ihandle *dlg, *gridbox;
static Ihandle *startButton;
static Ihandle *songBtn, *songText;
static Ihandle *scriptBtn, *scriptText;
static Ihandle *videoplayerBtn, *videoplayerText;
static Ihandle *config;

static const char* const video_players[] = {
    "C:\\Program Files\\MPC-HC\\mpc-hc64.exe",
    "C:\\Program Files (x86)\\MPC-HC\\mpc-hc.exe",
    "",
};

const char *findVideoPlayer(void) {
    const char **p = (const char **)video_players;
    while(str_len(*p) > 0) {
        if(PathFileExists(*p)) return *p;
        p++;
    }
    return *p;
}

int lazySetTextCB(Ihandle *self, char *filename, int num, int x, int y) {
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
    }
    IupConfigSave(config);

    return IUP_DEFAULT;
}

int scriptBtnCb(Ihandle *self) {
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

    return IUP_DEFAULT;
}

int songBtnCb(Ihandle *self) {
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

    return IUP_DEFAULT;
}

int videoplayerBtnCb(Ihandle *self) {
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

    return IUP_DEFAULT;
}

int startButtonCb(Ihandle *self) {
    (void)self;
    char *songfile = IupGetAttribute(songText,"VALUE");
    char *scriptfile = IupGetAttribute(scriptText,"VALUE");
    char *videoplayerfile = IupGetAttribute(videoplayerText,"VALUE");
    char **args = NULL;
    char **a;
    unsigned int r = 0;
    int t = 0;
    jpr_proc_info process;
    jpr_proc_pipe child_stdin;

    if(str_len(songfile) == 0 ||
       str_len(scriptfile) == 0 ||
       str_len(videoplayerfile) == 0)
    {
        return IUP_DEFAULT;
    }

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(decoder == NULL) goto cleanshitup;
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(processor == NULL) goto cleanshitup;
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(generator == NULL) goto cleanshitup;

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

int main(int argc, char **argv) {
    char s[PATH_MAX];

    IupOpen(&argc, &argv);

    config = IupConfig();
    IupSetAttribute(config,"APP_NAME","lua-music-visualizer");
    IupConfigLoad(config);

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

    startButton = IupButton("Start",NULL);
    IupSetCallback(startButton,"ACTION",(Icallback) startButtonCb);

    IupSetCallback(songText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetCallback(scriptText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetCallback(videoplayerText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetAttribute(songText,"DROPFILESTARGET","YES");
    IupSetAttribute(scriptText,"DROPFILESTARGET","YES");
    IupSetAttribute(videoplayerText,"DROPFILESTARGET","YES");

    gridbox = IupGridBox(songBtn,songText,scriptBtn,scriptText,videoplayerBtn,videoplayerText,startButton,NULL);
    IupSetAttribute(gridbox,"ORIENTATION","HORIZONTAL");
    IupSetAttribute(gridbox,"NUMDIV","2");
    IupSetAttribute(gridbox,"GAPLIN","20");
    IupSetAttribute(gridbox,"GAPCOL","20");
    IupSetAttribute(gridbox,"NORMALIZESIZE","HORIZONTAL");
    IupSetAttribute(gridbox,"MARGIN","20x20");

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

    dlg = IupDialog(gridbox);
    IupSetAttribute(dlg,"TITLE","Lua Music Visualizer");
    IupSetAttribute(dlg,"SIZE","300x120");
    IupShowXY(dlg,IUP_CENTER,IUP_CENTER);
    IupMainLoop();

    IupClose();

    return 0;
}
