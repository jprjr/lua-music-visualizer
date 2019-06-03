#include <stdio.h>
#include <string.h>
#include <iup.h>
#include <iup_config.h>
#include <windows.h>
#include <shlwapi.h>
#include <tchar.h>
#include <strsafe.h>
#include "audio-decoder.h"
#include "audio-processor.h"
#include "video-generator.h"
#include "str.h"

static audio_decoder *decoder;
static audio_processor *processor;
static video_generator *generator;
static Ihandle *dlg, *gridbox;
static Ihandle *startButton;
static Ihandle *songLabel, *songText;
static Ihandle *scriptLabel, *scriptText;
static Ihandle *ffplayLabel, *ffplayText;
static Ihandle *config;

static const char* const video_players[] = {
    "C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe",
    "C:\\Program Files\\VideoLAN\\VLC\\vlc.exe",
    "",
};

const char *findVideoPlayer(void) {
    const char **p = video_players;
    while(str_len(*p) > 0) {
        if(PathFileExists(*p)) return *p;
        *p++;
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
    } else if(self == ffplayText) {
        IupConfigSetVariableStr(config,"global","ffplay",filename);
    }

    return IUP_DEFAULT;
}

int startButtonCb(Ihandle *self) {
    (void)self;
    char *songfile = IupGetAttribute(songText,"VALUE");
    char *scriptfile = IupGetAttribute(scriptText,"VALUE");
    char *ffplayfile = IupGetAttribute(ffplayText,"VALUE");
    unsigned int r = 0;
    if(str_len(songfile) == 0 ||
       str_len(scriptfile) == 0 ||
       str_len(ffplayfile) == 0)
    {
        return IUP_DEFAULT;
    }

    char cmdLine[1000];
    cmdLine[0] = 0;
    if(strstr(ffplayfile,"vlc") != NULL) {
        str_cat(cmdLine,"stream://\\\\\\.\\pipe\\lua-music-vis");
    }
    if(strstr(ffplayfile,"ffplay") != NULL) {
        str_cat(cmdLine,"\\\\.\\pipe\\lua-music-vis");
    }

    HANDLE pipe = CreateNamedPipe("\\\\.\\pipe\\lua-music-vis",
      PIPE_ACCESS_DUPLEX,
      PIPE_TYPE_BYTE | PIPE_WAIT,
      255,
      1024 * 1024 * 5,
      1024,
      30 * 1000,
      NULL);

    if(pipe == INVALID_HANDLE_VALUE) {
        fprintf(stderr,"error making a pipe: %d\n",GetLastError());
        // return IUP_CLOSE;
        return IUP_DEFAULT;
    }
    IupExecute(ffplayfile,cmdLine);

    if(video_generator_init(generator,processor,decoder,songfile,scriptfile,pipe)) {
        fprintf(stderr,"error starting the video generator\n");
        return IUP_DEFAULT;
    }
    fprintf(stderr,"sending video...\n");

    do {
        r = video_generator_loop(generator);
    } while (r == 0);
    fprintf(stderr,"done making video");

    video_generator_close(generator);

    return IUP_DEFAULT;
}

int main(int argc, char **argv) {

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(decoder == NULL) return 1;
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(processor == NULL) return 1;
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(generator == NULL) return 1;

    IupOpen(&argc, &argv);

    config = IupConfig();
    IupSetAttribute(config,"APP_NAME","lua-music-visualizer");
    IupConfigLoad(config);

    songLabel = IupLabel("Song:");
    songText = IupText(NULL);
    IupSetAttribute(songText,"VALUE",IupConfigGetVariableStr(config,"global","songfile"));
    IupSetAttribute(songText, "EXPAND", "HORIZONTAL");

    scriptLabel = IupLabel("Script:");
    scriptText = IupText(NULL);
    IupSetAttribute(scriptText,"VALUE",IupConfigGetVariableStr(config,"global","scriptfile"));
    IupSetAttribute(scriptText, "EXPAND", "HORIZONTAL");

    ffplayLabel = IupLabel("FFPlay:");
    ffplayText = IupText(NULL);
    IupSetAttribute(ffplayText,"VALUE",IupConfigGetVariableStr(config,"global","ffplay"));
    IupSetAttribute(ffplayText, "EXPAND", "HORIZONTAL");
    if(str_len(IupGetAttribute(ffplayText,"VALUE")) == 0) {
        IupSetAttribute(ffplayText,"VALUE",findVideoPlayer());
    }

    startButton = IupButton("Start",NULL);
    IupSetCallback(startButton,"ACTION",(Icallback) startButtonCb);

    IupSetCallback(songText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetCallback(scriptText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetCallback(ffplayText,"DROPFILES_CB",(Icallback) lazySetTextCB);
    IupSetAttribute(songText,"DROPFILESTARGET","YES");
    IupSetAttribute(scriptText,"DROPFILESTARGET","YES");
    IupSetAttribute(ffplayText,"DROPFILESTARGET","YES");

    gridbox = IupGridBox(songLabel,songText,scriptLabel,scriptText,ffplayLabel,ffplayText,startButton,NULL);
    IupSetAttribute(gridbox,"ORIENTATION","HORIZONTAL");
    IupSetAttribute(gridbox,"NUMDIV","2");
    IupSetAttribute(gridbox,"GAPLIN","20");
    IupSetAttribute(gridbox,"GAPCOL","20");

    dlg = IupDialog(gridbox);
    IupSetAttribute(dlg,"TITLE","Lua Music Visualizer");
    IupSetAttribute(dlg,"SIZE","300x100");
    IupShowXY(dlg,IUP_CENTER,IUP_CENTER);
    IupMainLoop();

    IupClose();

    /*

    if(video_generator_init(generator,processor,decoder,argv[1],argv[2])) return 1;

    do {
        r = video_generator_loop(generator);
    } while (r == 0);

    video_generator_close(generator);
    */
    return 0;
}
