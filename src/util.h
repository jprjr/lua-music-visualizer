#ifndef JPR_UTIL_H
#define JPR_UTIL_H

#ifdef NDEBUG
#define DEBUGLOG(s)
#else

#if defined(_WIN32) || defined(_WIN64)
#define DEBUGLOG(s) \
    WriteFile(GetStdHandle(STD_ERROR_HANDLE),s,lstrlenA(s),NULL,0); \
    WriteFile(GetStdHandle(STD_ERROR_HANDLE),"\n",1,NULL,0);
#else
#define DEBUGLOG(s) \
    (void)write(2,s,strlen(s)); \
    (void)write(2,"\n",1);
#endif
#endif
#endif
