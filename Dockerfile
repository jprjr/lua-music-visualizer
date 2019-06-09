FROM jprjr/iup:3.27

ENV LUAJIT_VER=2.1.0-beta3

RUN apt-get update && apt-get install -y gcc-multilib upx

RUN mkdir -p /src && \
    cd /src && \
    curl -R -L -O http://luajit.org/download/LuaJIT-${LUAJIT_VER}.tar.gz && \
    tar xf LuaJIT-${LUAJIT_VER}.tar.gz && \
    cd LuaJIT-${LUAJIT_VER}/src && \
    make TARGET_SYS=Windows HOST_CC="gcc -m32" CROSS="i686-w64-mingw32-" libluajit.a && \
    cp lauxlib.h /usr/i686-w64-mingw32/include && \
    cp lua*.h /usr/i686-w64-mingw32/include && \
    cp libluajit.a /usr/i686-w64-mingw32/lib && \
    make clean && \
    make TARGET_SYS=Windows HOST_CC="gcc" CROSS="x86_64-w64-mingw32-" libluajit.a && \
    cp lauxlib.h /usr/x86_64-w64-mingw32/include && \
    cp lua*.h /usr/x86_64-w64-mingw32/include && \
    cp libluajit.a /usr/x86_64-w64-mingw32/lib && \
    make clean && \
    cd / && \
    rm -rf /src/LuaJIT-${LUAJIT_VER}

COPY . /src/lua-music-visualizer
WORKDIR /src/lua-music-visualizer

RUN \
    make -f Makefile.windows.docker TARGET=i686-w64-mingw32 clean && \
    make -f Makefile.windows.docker TARGET=i686-w64-mingw32 && \
    mv lua-music-visualizer.exe lua-music-visualizer-luajit32.exe && \
    make -f Makefile.windows.docker TARGET=i686-w64-mingw32 clean && \
    make -f Makefile.windows.docker TARGET=x86_64-w64-mingw32 && \
    mv lua-music-visualizer.exe lua-music-visualizer-luajit64.exe && \
    make -f Makefile.windows.docker TARGET=x86_64-w64-mingw32 clean
