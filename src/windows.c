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
    char cmdLine[1000];
    HANDLE pipe = INVALID_HANDLE_VALUE;
    HANDLE childStdInRd = INVALID_HANDLE_VALUE;
    HANDLE childStdInWr = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    unsigned int r = 0;

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

    cmdLine[0] = 0;

    memset(&sa,0,sizeof(SECURITY_ATTRIBUTES));
    memset(&pi,0,sizeof(PROCESS_INFORMATION));
    memset(&si,0,sizeof(STARTUPINFO));

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;


    if(!CreatePipe(&childStdInRd, &childStdInWr, &sa, 0)) {
        goto cleanshitup;
    }

    if(!SetHandleInformation(childStdInWr, HANDLE_FLAG_INHERIT, 0)) {
        goto cleanshitup;
    }

    si.cb = sizeof(STARTUPINFO);
    si.hStdError = NULL;
    si.hStdOutput = NULL;
    si.hStdInput = childStdInRd;
    si.dwFlags |= STARTF_USESTDHANDLES;

    str_cat(cmdLine,videoplayerfile);
    if(strstr(videoplayerfile,"vlc") != NULL) {
        str_cat(cmdLine," --file-caching 1500 --network-caching 1500");
    }
    str_cat(cmdLine," -");
    fprintf(stderr,"spawning: %s\n",cmdLine);


    if(!CreateProcess(NULL,
        cmdLine,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi)) goto cleanshitup;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(childStdInRd);
    childStdInRd = INVALID_HANDLE_VALUE;


    if(video_generator_init(generator,processor,decoder,songfile,scriptfile,childStdInWr)) {
        fprintf(stderr,"error starting the video generator\n");
        goto cleanshitup;
    }
    fprintf(stderr,"sending video...\n");

    do {
        r = video_generator_loop(generator);
    } while (r == 0);

    video_generator_close(generator);

cleanshitup:
    if(pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe);
    }
    if(childStdInRd != INVALID_HANDLE_VALUE) {
        CloseHandle(childStdInRd);
    }
    if(childStdInWr != INVALID_HANDLE_VALUE) {
        CloseHandle(childStdInWr);
    }

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
