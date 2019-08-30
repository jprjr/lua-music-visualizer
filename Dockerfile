FROM ubuntu:18.04 as windows-builder

ENV LUAJIT_VER=2.1.0-beta3

RUN apt-get update && apt-get install -y gcc-multilib upx \
    mingw-w64 curl make patch build-essential git


RUN mkdir -p /src && \
    cd /src && \
    git clone https://github.com/jprjr/iup-build.git iup
WORKDIR /src/iup

RUN \
    make TARGET=i686-w64-mingw32 clean && \
    make TARGET=x86_64-w64-mingw32 clean && \
    make TARGET=i686-w64-mingw32 && \
    make TARGET=i686-w64-mingw32 dist && \
    make TARGET=x86_64-w64-mingw32 && \
    make TARGET=x86_64-w64-mingw32 dist && \
    tar xzf output/lua-i686-w64-mingw32.tar.gz -C /usr/i686-w64-mingw32 && \
    tar xzf output/ftgl-i686-w64-mingw32.tar.gz -C /usr/i686-w64-mingw32 && \
    tar xzf output/freetype-i686-w64-mingw32.tar.gz -C /usr/i686-w64-mingw32 && \
    tar xzf output/zlib-i686-w64-mingw32.tar.gz -C /usr/i686-w64-mingw32 && \
    tar xzf output/im-i686-w64-mingw32.tar.gz -C /usr/i686-w64-mingw32 && \
    tar xzf output/cd-i686-w64-mingw32.tar.gz -C /usr/i686-w64-mingw32 && \
    tar xzf output/iup-i686-w64-mingw32.tar.gz -C /usr/i686-w64-mingw32 && \
    tar xzf output/lua-x86_64-w64-mingw32.tar.gz -C /usr/x86_64-w64-mingw32 && \
    tar xzf output/ftgl-x86_64-w64-mingw32.tar.gz -C /usr/x86_64-w64-mingw32 && \
    tar xzf output/freetype-x86_64-w64-mingw32.tar.gz -C /usr/x86_64-w64-mingw32 && \
    tar xzf output/zlib-x86_64-w64-mingw32.tar.gz -C /usr/x86_64-w64-mingw32 && \
    tar xzf output/im-x86_64-w64-mingw32.tar.gz -C /usr/x86_64-w64-mingw32 && \
    tar xzf output/cd-x86_64-w64-mingw32.tar.gz -C /usr/x86_64-w64-mingw32 && \
    tar xzf output/iup-x86_64-w64-mingw32.tar.gz -C /usr/x86_64-w64-mingw32 && \
    make TARGET=i686-w64-mingw32 clean && \
    make TARGET=x86_64-w64-mingw32 clean && \
    cd / && \
    rm -rf /src/iup

WORKDIR /

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
    mkdir -p /dist/win32 && \
    mkdir -p /dist/win64 && \
    make -f Makefile.windows.docker TARGET=i686-w64-mingw32 clean && \
    make -f Makefile.windows.docker TARGET=i686-w64-mingw32 -j4 && \
    cp *.exe /dist/win32/ && \
    make -f Makefile.windows.docker TARGET=i686-w64-mingw32 clean && \
    make -f Makefile.windows.docker TARGET=x86_64-w64-mingw32 -j4 && \
    cp *.exe /dist/win64/ && \
    make -f Makefile.windows.docker TARGET=x86_64-w64-mingw32 clean

FROM alpine:3.10 as linux-builder

RUN apk add file pkgconfig make gcc musl-dev luajit-dev linux-headers && \
    mkdir -p /src

COPY . /src/lua-music-visualizer
WORKDIR /src/lua-music-visualizer

RUN make clean && make CC="gcc -static -fPIE" PKGCONFIG="pkg-config --static" && strip lua-music-visualizer
RUN mkdir -p /dist && \
    cp lua-music-visualizer /dist

FROM multiarch/crossbuild as osx-builder

ENV CROSS_TRIPLE=x86_64-apple-darwin14
ENV LUAJIT_VER=2.1.0-beta3

RUN mkdir -p /src && \
    cd /src && \
    curl -R -L -O http://luajit.org/download/LuaJIT-${LUAJIT_VER}.tar.gz && \
    tar xf LuaJIT-${LUAJIT_VER}.tar.gz

RUN cd /src/LuaJIT-${LUAJIT_VER}/src && \
    make TARGET_SYS=Darwin HOST_SYS=Linux HOST_CC="gcc" CC=/usr/osxcross/bin/o64-clang libluajit.a && \
    /usr/osxcross/bin/${CROSS_TRIPLE}-ranlib libluajit.a

COPY . /src/lua-music-visualizer
WORKDIR /src/lua-music-visualizer

RUN make clean
RUN make CC=/usr/osxcross/bin/o64-clang CFLAGS="-I/src/LuaJIT-${LUAJIT_VER}/src -Wall -Wextra -DNDEBUG -Os" LDFLAGS="-L/src/LuaJIT-${LUAJIT_VER}/src -lluajit -pagezero_size 10000 -image_base 100000000 -lm -pthread" HOST_CC="gcc"
RUN mkdir -p /dist && cp lua-music-visualizer /dist/

FROM alpine:3.10
RUN apk add upx rsync tar zip gzip && mkdir -p /dist/linux && mkdir -p /dist/osx && mkdir -p /dist/win32 && mkdir -p /dist/win64
COPY --from=windows-builder /dist/win32/* /dist/win32/
COPY --from=windows-builder /dist/win64/* /dist/win64/
COPY --from=linux-builder /dist/* /dist/linux/
COPY --from=osx-builder /dist/* /dist/osx/

RUN cd dist && \
    cd win32 && \
    upx *.exe && \
    zip ../lua-music-visualizer-$(../linux/lua-music-visualizer --version)-win32.zip *.exe && \
    cd ../win64 && \
    upx *.exe && \
    zip ../lua-music-visualizer-$(../linux/lua-music-visualizer --version)-win64.zip *.exe && \
    cd ../osx && \
    upx lua-music-visualizer && \
    tar cvzf ../lua-music-visualizer-$(../linux/lua-music-visualizer --version)-osx.tar.gz * && \
    cd ../linux && \
    upx lua-music-visualizer && \
    tar cvzf ../lua-music-visualizer-$(./lua-music-visualizer --version)-linux.tar.gz * && \
    cd .. && \
    rm -rf win32 win64 linux osx
COPY entrypoint.sh /
ENTRYPOINT ["/entrypoint.sh"]
