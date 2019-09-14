#ifndef MPDC_HEADER
#define MPDC_HEADER

#include <inttypes.h>
#include <stdarg.h>

#ifndef MPDC_BUFFER_SIZE
#define MPDC_BUFFER_SIZE 4096
#endif

#ifndef MPDC_OP_QUEUE_SIZE
#define MPDC_OP_QUEUE_SIZE 100
#endif

#ifndef MPDC_ALL_STATIC
#define MPDC_ALL_STATIC 0
#endif

#if MPDC_ALL_STATIC
#define STATIC static
#else
#define STATIC
#endif

typedef struct mpdc_connection_s mpdc_connection;
typedef struct mpdc_ringbuf_s mpdc_ringbuf;

enum MPDC_COMMAND {
    MPDC_COMMAND_CLEARERROR, MPDC_COMMAND_CURRENTSONG, MPDC_COMMAND_IDLE,
    MPDC_COMMAND_STATUS, MPDC_COMMAND_STATS, MPDC_COMMAND_CONSUME,
    MPDC_COMMAND_CROSSFADE, MPDC_COMMAND_MIXRAMPDB, MPDC_COMMAND_MIXRAMPDELAY,
    MPDC_COMMAND_RANDOM, MPDC_COMMAND_REPEAT, MPDC_COMMAND_SETVOL,
    MPDC_COMMAND_SINGLE, MPDC_COMMAND_REPLAY_GAIN_MODE, MPDC_COMMAND_REPLAY_GAIN_STATUS,
    MPDC_COMMAND_VOLUME, MPDC_COMMAND_NEXT, MPDC_COMMAND_PAUSE, MPDC_COMMAND_PLAY,
    MPDC_COMMAND_PLAYID, MPDC_COMMAND_PREVIOUS, MPDC_COMMAND_SEEK, MPDC_COMMAND_SEEKID,
    MPDC_COMMAND_SEEKCUR, MPDC_COMMAND_STOP, MPDC_COMMAND_ADD, MPDC_COMMAND_ADDID,
    MPDC_COMMAND_CLEAR, MPDC_COMMAND_DELETE, MPDC_COMMAND_DELETEID, MPDC_COMMAND_MOVE,
    MPDC_COMMAND_MOVEID, MPDC_COMMAND_PLAYLIST, MPDC_COMMAND_PLAYLISTFIND, MPDC_COMMAND_PLAYLISTID,
    MPDC_COMMAND_PLAYLISTINFO, MPDC_COMMAND_PLAYLISTSEARCH, MPDC_COMMAND_PLCHANGES, MPDC_COMMAND_PLCHANGESPOSID,
    MPDC_COMMAND_PRIO, MPDC_COMMAND_PRIOID, MPDC_COMMAND_RANGEID, MPDC_COMMAND_SHUFFLE,
    MPDC_COMMAND_SWAP, MPDC_COMMAND_SWAPID, MPDC_COMMAND_ADDTAGID, MPDC_COMMAND_CLEARTAGID,
    MPDC_COMMAND_LISTPLAYLIST, MPDC_COMMAND_LISTPLAYLISTINFO, MPDC_COMMAND_LISTPLAYLISTS, MPDC_COMMAND_LOAD,
    MPDC_COMMAND_PLAYLISTADD, MPDC_COMMAND_PLAYLISTCLEAR, MPDC_COMMAND_PLAYLISTDELETE, MPDC_COMMAND_PLAYLISTMOVE,
    MPDC_COMMAND_RENAME, MPDC_COMMAND_RM, MPDC_COMMAND_SAVE, MPDC_COMMAND_ALBUMART, MPDC_COMMAND_COUNT,
    MPDC_COMMAND_GETFINGERPRINT, MPDC_COMMAND_FIND, MPDC_COMMAND_FINDADD, MPDC_COMMAND_LIST, MPDC_COMMAND_LISTALL,
    MPDC_COMMAND_LISTALLINFO, MPDC_COMMAND_LISTFILES, MPDC_COMMAND_LSINFO, MPDC_COMMAND_READCOMMENTS,
    MPDC_COMMAND_SEARCH, MPDC_COMMAND_SEARCHADD, MPDC_COMMAND_SEARCHADDPL, MPDC_COMMAND_UPDATE,
    MPDC_COMMAND_RESCAN, MPDC_COMMAND_MOUNT, MPDC_COMMAND_UNMOUNT, MPDC_COMMAND_LISTMOUNTS,
    MPDC_COMMAND_LISTNEIGHBORS, MPDC_COMMAND_STICKER, MPDC_COMMAND_CLOSE, MPDC_COMMAND_KILL,
    MPDC_COMMAND_PASSWORD, MPDC_COMMAND_PING, MPDC_COMMAND_TAGTYPES, MPDC_COMMAND_PARTITION,
    MPDC_COMMAND_LISTPARTITIONS, MPDC_COMMAND_NEWPARTITION, MPDC_COMMAND_DISABLEOUTPUT, MPDC_COMMAND_ENABLEOUTPUT,
    MPDC_COMMAND_OUTPUTS, MPDC_COMMAND_OUTPUTSET, MPDC_COMMAND_CONFIG, MPDC_COMMAND_COMMANDS,
    MPDC_COMMAND_NOTCOMMANDS, MPDC_COMMAND_URLHANDLERS, MPDC_COMMAND_DECODERS, MPDC_COMMAND_SUBSCRIBE,
    MPDC_COMMAND_UNSUBSCRIBE, MPDC_COMMAND_CHANNELS, MPDC_COMMAND_READMESSAGES, MPDC_COMMAND_SENDMESSAGE
};

enum MPDC_EVENT {
    MPDC_EVENT_DATABASE          = 1,
    MPDC_EVENT_UPDATE            = 2,
    MPDC_EVENT_STORED_PLAYLIST   = 4,
    MPDC_EVENT_PLAYLIST          = 8,
    MPDC_EVENT_PLAYER            = 16,
    MPDC_EVENT_MIXER             = 32,
    MPDC_EVENT_OUTPUT            = 64,
    MPDC_EVENT_OPTIONS           = 128,
    MPDC_EVENT_PARTITION         = 256,
    MPDC_EVENT_STICKER           = 512,
    MPDC_EVENT_SUBSCRIPTION      = 1024,
    MPDC_EVENT_MESSAGE           = 2048
};


/* all functions:
 * return 1+ on success
 * 0 on nothing (ie, if a file call would block)
 * -1 on error/EOF */

/* read/write are responsible for getting data in/out */
/* required */
/* should return -1 on error or eof, 0 if no data available, 1+ for data read/written */
typedef int (*mpdc_write_func)(void *ctx, const uint8_t *buf, unsigned int);
typedef int (*mpdc_read_func)(void *ctx, uint8_t *buf, unsigned int);

/* function called to trigger that the client is ready to read */
/* if client is blocking, set this to NULL */
/* returns <= 0 on errors, 1+ on success */
typedef int (*mpdc_read_notify_func)(mpdc_connection *);

/* function called to trigger that the client is ready to write */
/* if client is blocking, set this to NULL */
/* returns <= 0 on errors, 1+ on success */
typedef int (*mpdc_write_notify_func)(mpdc_connection *);

/* resolve is called at the end of mpdc_setup */
/* optional */
/* return <=0 on errors, 1+ on success */
typedef int (*mpdc_resolve_func)(mpdc_connection *, const char *hostname);

/* connect is called at the end of mpdc_connect */
/* required */
/* return <=0 on errors, 1+ on success */
typedef int (*mpdc_connect_func)(mpdc_connection *, const char *hostname, uint16_t port);

/* called when disconnected from MPD */
typedef void (*mpdc_disconnect_func)(mpdc_connection *);


/* called when MPD starts reading a response from a command */
typedef void (*mpdc_response_begin_func)(mpdc_connection *, const char *cmd);

/* called when MPD finishes reading a response from a command */
/* `ok` indicates if the command finished in OK or error */
typedef void (*mpdc_response_end_func)(mpdc_connection *, const char *cmd, int ok, const char *err);

/* called when receiving a response from a command */
/* key will always be null-terminated, value may not (ie,
 * in the case of the albumart command */
typedef void (*mpdc_response_func)(mpdc_connection *,const char *cmd, const char *key, const uint8_t *value, unsigned int length);

/* "private" method for receiving data */
typedef int (*_mpdc_receive_func)(mpdc_connection *);


struct mpdc_ringbuf_s {
    uint8_t autofill;
    uint8_t autoflush;
    uint8_t *buf;
    uint8_t *head;
    uint8_t *tail;
    unsigned int size;
    void *read_ctx;
    void *write_ctx;
    mpdc_read_func read;
    mpdc_write_func write;
};

struct mpdc_connection_s {
    void *ctx;
    uint8_t op_buf[MPDC_OP_QUEUE_SIZE];
    uint8_t out_buf[MPDC_BUFFER_SIZE];
    uint8_t in_buf[MPDC_BUFFER_SIZE];
    uint8_t scratch[MPDC_BUFFER_SIZE];
    mpdc_ringbuf op;
    mpdc_ringbuf out;
    mpdc_ringbuf in;
    char *host;
    char *password;
    uint8_t cb_level;
    uint8_t state;
    uint16_t port;
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    int _mode;
    unsigned int _bytes;
    mpdc_write_func write;
    mpdc_read_func read;
    mpdc_resolve_func resolve;
    mpdc_response_begin_func response_begin;
    mpdc_response_func response;
    mpdc_response_end_func response_end;
    mpdc_connect_func connect;
    mpdc_disconnect_func disconnect;
    mpdc_write_notify_func write_notify;
    mpdc_read_notify_func read_notify;
    _mpdc_receive_func _receive;
};


/* returns 0 on success, anything else on failure */
STATIC
int mpdc_init(mpdc_connection *);

/* calls the resolve callback */
/* setup looks in host/port strings to find MPD password, if needed */
/* if you already have the port in an integer format, set "port" to
 * NULL and use uport instead.
 * Setting host to NULL will use the default host "127.0.0.1" */
/* returns 1 on success, anything else on failure */
STATIC
int mpdc_setup(mpdc_connection *, char *host, char *port, uint16_t uport);

/* switch between the block/noblock modes, usually auto-detected */
STATIC
void mpdc_block(mpdc_connection *, int status);

/* calls the connect callback */
STATIC
int mpdc_connect(mpdc_connection *connection);

/* should be called to send the next queued command */
/* returns 1 on succesfully sending the whole line, 0
 * on a partial send, -1 on error */
/* automatically calls read_notify if all data was sent */
STATIC
int mpdc_send(mpdc_connection *connection);

/* should be called to read and process data */
/* returns 1 on finishing a command, 0 on partial data, -1 on error */
STATIC
int mpdc_receive(mpdc_connection *connection);

STATIC
void mpdc_disconnect(mpdc_connection *connection);

/* mpdc commands return 1 on success */
STATIC
int mpdc_password(mpdc_connection *connection, const char *password);

STATIC
int mpdc_idle(mpdc_connection *connection, uint_least16_t events);

STATIC
int mpdc__put(mpdc_connection *connection, uint8_t cmd, const char *fmt, ...);

#define mpdc__zero_arg(conn,cmd) mpdc__put((conn),cmd,"")

/* the following commands take no arguments, besides the
 * connection object.
 * they all have a function signature like:
 * int mpdc_clearerror(mpdc_connection *connection) */
#define mpdc_clearerror(conn) mpdc__zero_arg((conn),MPDC_COMMAND_CLEARERROR)
#define mpdc_currentsong(conn) mpdc__zero_arg((conn),MPDC_COMMAND_CURRENTSONG)
#define mpdc_status(conn) mpdc__zero_arg((conn),    MPDC_COMMAND_STATUS)
#define mpdc_stats(conn) mpdc__zero_arg((conn),        MPDC_COMMAND_STATS)
#define mpdc_replay_gain_status(conn) mpdc__zero_arg((conn), MPDC_COMMAND_REPLAY_GAIN_STATUS)
#define mpdc_next(conn) mpdc__zero_arg((conn), MPDC_COMMAND_NEXT)
#define mpdc_previous(conn) mpdc__zero_arg((conn), MPDC_COMMAND_PREVIOUS)
#define mpdc_stop(conn) mpdc__zero_arg((conn), MPDC_COMMAND_STOP)
#define mpdc_clear(conn) mpdc__zero_arg((conn), MPDC_COMMAND_CLEAR)
#define mpdc_playlist(conn) mpdc__zero_arg((conn), MPDC_COMMAND_PLAYLIST)
#define mpdc_listplaylists(conn) mpdc__zero_arg((conn), MPDC_COMMAND_LISTPLAYLISTS)
#define mpdc_listmounts(conn) mpdc__zero_arg((conn), MPDC_COMMAND_LISTMOUNTS)
#define mpdc_listneighbors(conn) mpdc__zero_arg((conn), MPDC_COMMAND_LISTNEIGHBORS)
#define mpdc_close(conn) mpdc__zero_arg((conn), MPDC_COMMAND_CLOSE)
#define mpdc_kill(conn) mpdc__zero_arg((conn), MPDC_COMMAND_KILL)
#define mpdc_ping(conn) mpdc__zero_arg((conn), MPDC_COMMAND_PING)
#define mpdc_tagtypes(conn) mpdc__zero_arg((conn), MPDC_COMMAND_TAGTYPES)
#define mpdc_listpartitions(conn) mpdc__zero_arg((conn), MPDC_COMMAND_LISTPARTITIONS)
#define mpdc_outputs(conn) mpdc__zero_arg((conn),MPDC_COMMAND_OUTPUTS)
#define mpdc_config(conn) mpdc__zero_arg((conn), MPDC_COMMAND_CONFIG)
#define mpdc_commands(conn) mpdc__zero_arg((conn),MPDC_COMMAND_COMMANDS)
#define mpdc_notcommands(conn) mpdc__zero_arg((conn),MPDC_COMMAND_NOTCOMMANDS)
#define mpdc_urlhandlers(conn) mpdc__zero_arg((conn), MPDC_COMMAND_URLHANDLERS)
#define mpdc_decoders(conn) mpdc__zero_arg((conn), MPDC_COMMAND_DECODERS)
#define mpdc_channels(conn) mpdc__zero_arg((conn), MPDC_COMMAND_CHANNELS)
#define mpdc_readmessages(conn) mpdc__zero_arg((conn), MPDC_COMMAND_READMESSAGES)

/* used to provide "overloading" */
#define GET_MACRO(_1,_2,_3,_4,NAME,...) NAME

#define mpdc_albumart(conn,path,offset) mpdc__put((conn), MPDC_COMMAND_ALBUMART,"su",path,offset)

#define mpdc_subscribe(conn,chan) mpdc__put((conn), MPDC_COMMAND_SUBSCRIBE,"s",chan)

#define mpdc_consume(conn,state) mpdc__put((conn),MPDC_COMMAND_CONSUME,"u",( state == 0 ? 0 : 1 ))

#define mpdc_crossfade(conn,seconds) mpdc__put((conn),MPDC_COMMAND_CONSUME,"u",seconds)

#define mpdc_mixrampdb(conn,thresh) mpdc__put((conn),MPDC_COMMAND_MIXRAMPDB,"d",thresh)

#define mpdc_mixrampdelay(conn,seconds) mpdc__put((conn),MPDC_COMMAND_MIXRAMPDELAY,"d",seconds)

#define mpdc_random(conn,state) mpdc__put((conn),MPDC_COMMAND_RANDOM,"u",( state == 0 ? 0 : 1 ))

#define mpdc_repeat(conn,state) mpdc__put((conn),MPDC_COMMAND_REPEAT,"u",( state == 0 ? 0 : 1 ))

#define mpdc_setvol(conn,volume) mpdc__put((conn),MPDC_COMMAND_SETVOL,"u",( volume >= 0 && volume <= 100 ? volume : 0))

#define mpdc_single(conn,state) mpdc__put((conn),MPDC_COMMAND_SINGLE,"u",( state == 0 ? 0 : 1 ))

#define mpdc_replay_gain_mode(conn,mode) mpdc__put((conn),MPDC_COMMAND_REPLAY_GAIN_MODE,"s",mode)

#define mpdc_volume(conn,change) mpdc__put((conn),MPDC_COMMAND_VOLUME,"d",change)

#define mpdc_pause(conn,state) mpdc__put((conn),MPDC_COMMAND_PAUSE,"u",( state == 0 ? 0 : 1 ))

#define mpdc_play1(conn) mpdc__zero_arg((conn), MPDC_COMMAND_PLAY)
#define mpdc_play2(conn,songpos) mpdc__put((conn),MPDC_COMMAND_PLAY,"u",songpos)
#define mpdc_play(...) GET_MACRO(__VA_ARGS__,void,void,mpdc_play2,mpdc_play1)(__VA_ARGS__)

#define mpdc_playid1(conn) mpdc__zero_arg((conn), MPDC_COMMAND_PLAYID)
#define mpdc_playid2(conn,songpos) mpdc__put((conn),MPDC_COMMAND_PLAYID,"u",songpos)
#define mpdc_playid(...) GET_MACRO(__VA_ARGS__,void,void,mpdc_playid2,mpdc_playid1)(__VA_ARGS__)

#define mpdc_seek(conn,songpos,time) mpdc__put((conn),MPDC_COMMAND_SEEK,"uu",songpos,time)

#define mpdc_seekid(conn,songid,time) mpdc__put((conn),MPDC_COMMAND_SEEKID,"uu",seekid,time)

#define mpdc_seekcur(conn,time) mpdc__put((conn),MPDC_COMMAND_SEEKCUR,"u",seekcur)

#define mpdc_add(conn,uri) mpdc__put((conn),MPDC_COMMAND_ADD,"s",uri)

#define mpdc_addid2(conn,uri) mpdc__put((conn),MPDC_COMMAND_ADDID,"s",uri)
#define mpdc_addid3(conn,uri,pos) mpdc__put((conn),MPDC_COMMAND_ADDID,"su",uri,pos)
#define mpdc_addid(...) GET_MACRO(__VA_ARGS__,void,mpdc_addid3,mpdc_addid2,void)(__VA_ARGS__)

#define mpdc_delete2(conn,pos) mpdc__put((conn),MPDC_COMMAND_DELETE,"u",pos)
#define mpdc_delete3(conn,start,end) mpdc__put((conn),MPDC_COMMAND_DELETE,"u:U",start,end)
#define mpdc_delete(...) GET_MACRO(__VA_ARGS__,void,mpdc_delete3,mpdc_delete2,void)(__VA_ARGS__)

#define mpdc_deleteid(conn,id) mpdc__put((conn),MPDC_COMMAND_DELETEID,"u",id)

#define mpdc_move4(conn,start,end,to) mpdc__put((conn),MPDC_COMMAND_MOVE,"u:Uu",start,end,to)
#define mpdc_move3(conn,from,to) mpdc__put((conn),MPDC_COMMAND_MOVE,"uu", from, to)
#define mpdc_move(...) GET_MACRO(__VA_ARGS__,mpdc_move4,mpdc_move3,void,void)(__VA_ARGS__)

#define mpdc_moveid(conn,from,to) mpdc__put((conn),MPDC_COMMAND_MOVEID,"uu",from,to)

#define mpdc_playlistfind(conn,tag,needle) mpdc__put((conn),MPDC_COMMANDPLAYLISTFIND,"ss",tag,needle)

#define mpdc_playlistid1(conn) mpdc__zero_arg((conn),MPDC_COMMAND_PLAYLISTID)
#define mpdc_playlistid2(conn,id) mpdc__put((conn),MPDC_COMMAND_PLAYLISTID,"u",id)
#define mpdc_playlistid(...) GET_MACRO(__VA_ARGS__,void,void,mpdc_playlistid2,mpdc_playlistid1)(__VA_ARGS__)

#define mpdc_playlistinfo1(conn) mpdc__zero_arg((conn),MPDC_COMMAND_PLAYLISTINFO)
#define mpdc_playlistinfo2(conn,pos) mpdc__put((conn),MPDC_COMMAND_PLAYLISTINFO,"u",pos)
#define mpdc_playlistinfo3(conn,start,end) mpdc__put((conn),MPDC_COMMAND_PLAYLISTINFO,"u:U",start,end)
#define mpdc_playlistinfo(...) GET_MACRO(__VA_ARGS__,void,mpdc_playlistinfo3,mpdc_playlistinfo2,mpdc_playlistinfo1)(__VA_ARGS__)

#define mpdc_playlistsearch(conn,tag,needle) mpdc__put((conn),MPDC_COMMAND_PLAYLISTSEARCH,"ss",tag,needle)

#define mpdc_plchanges2(conn,version) mpdc__put((conn),MPDC_COMMAND_PLCHANGES,"u",version)
#define mpdc_plchanges4(conn,version,start,end) mpdc__put((conn),MPDC_COMMAND_PLCHANGES,"uu:U",version,start,end)
#define mpdc_plchanges(...) GET_MACRO(__VA_ARGS__,mpdc_plchanges4,void,void,mpdc_plchanges1)(__VA_ARGS__)

#define mpdc_plchangesposid2(conn,version) mpdc__put((conn),MPDC_COMMAND_PLCHANGESPOSID,"u",version)
#define mpdc_plchangesposid4(conn,version,start,end) mpdc__put((conn),MPDC_COMMAND_PLCHANGESPOSID,"uu:U",version,start,end)
#define mpdc_plchangesposid(...) GET_MACRO(__VA_ARGS__,mpdc_plchangesposid4,void,void,mpdc_plchangesposid1)(__VA_ARGS__)

#define mpdc_prio(conn,prio,start,end) mpdc__put((conn),MPDC_COMMAND_PRIO,"uu:U",prio,start,end)
#define mpdc_priod(conn,prio,id) mpdc__put((conn),MPDC_COMMAND_PRIOD,"uu",prio,id)

#define mpdc_sendmessage(conn,channel,text) mpdc__put((conn),MPDC_COMMAND_SENDMESSAGE,"ss",channel,text)

#if 0
#define mpdc_consume1(conn) mpdc__zero_arg((conn),MPDC_COMMAND_CONSUME)
#define mpdc_consume2(conn,state) mpdc__put((conn),MPDC_COMMAND_CONSUME,"u",state)

#define mpdc_consume(...) GET_MACRO(__VA_ARGS__,void,void,mpdc_consume2,mpdc_consume1)(__VA_ARGS__)
#endif


#endif
