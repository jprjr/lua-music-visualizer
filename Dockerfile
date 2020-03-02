FROM ubuntu:20.04 as windows-builder

ENV LUAJIT_VER=2.1.0-beta3
ENV LIBSAMPLERATE_VER=0.1.9
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y gcc-multilib upx \
    mingw-w64 nasm curl make patch build-essential git pkg-config

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
    make -j$(nproc) TARGET_SYS=Windows HOST_CC="gcc -m32" CROSS="i686-w64-mingw32-" libluajit.a && \
    cp lauxlib.h /usr/i686-w64-mingw32/include && \
    cp lua*.h /usr/i686-w64-mingw32/include && \
    cp libluajit.a /usr/i686-w64-mingw32/lib && \
    make clean && \
    make -j$(nproc) TARGET_SYS=Windows HOST_CC="gcc" CROSS="x86_64-w64-mingw32-" libluajit.a && \
    cp lauxlib.h /usr/x86_64-w64-mingw32/include && \
    cp lua*.h /usr/x86_64-w64-mingw32/include && \
    cp libluajit.a /usr/x86_64-w64-mingw32/lib && \
    make clean && \
    cd / && \
    rm -rf /src/LuaJIT-${LUAJIT_VER}

RUN rm -f /usr/bin/i686-w64-mingw32-pkg-config && \
    rm -f /usr/bin/x86_64-w64-mingw32-pkg-config && \
    echo "#!/bin/sh" >> /usr/bin/i686-w64-mingw32-pkg-config && \
    echo "export PKG_CONFIG_DIR=" >> /usr/bin/i686-w64-mingw32-pkg-config && \
    echo "export PKG_CONFIG_LIBDIR=/usr/i686-w64-mingw32/lib/pkgconfig" >> /usr/bin/i686-w64-mingw32-pkg-config && \
    echo "export PKG_CONFIG_SYSROOT_DIR=/usr/i686-w64-mingw32" >> /usr/bin/i686-w64-mingw32-pkg-config && \
    echo "exec pkg-config \"\$@\"" >> /usr/bin/i686-w64-mingw32-pkg-config && \
    echo "#!/bin/sh" >> /usr/bin/x86_64-w64-mingw32-pkg-config && \
    echo "export PKG_CONFIG_DIR=" >> /usr/bin/x86_64-w64-mingw32-pkg-config && \
    echo "export PKG_CONFIG_LIBDIR=/usr/x86_64-w64-mingw32/lib/pkgconfig" >> /usr/bin/x86_64-w64-mingw32-pkg-config && \
    echo "export PKG_CONFIG_SYSROOT_DIR=/usr/x86_64-w64-mingw32" >> /usr/bin/x86_64-w64-mingw32-pkg-config && \
    echo "exec pkg-config \"\$@\"" >> /usr/bin/x86_64-w64-mingw32-pkg-config && \
    chmod +x /usr/bin/i686-w64-mingw32-pkg-config && \
    chmod +x /usr/bin/x86_64-w64-mingw32-pkg-config

RUN cd /src && \
    curl -R -L -O http://www.mega-nerd.com/SRC/libsamplerate-${LIBSAMPLERATE_VER}.tar.gz && \
    tar xf libsamplerate-${LIBSAMPLERATE_VER}.tar.gz && \
    cd libsamplerate-${LIBSAMPLERATE_VER} && \
    ./configure \
      --prefix=/usr/i686-w64-mingw32 \
      --disable-shared \
      --enable-static \
      --host=i686-w64-mingw32 && \
    make && \
    make install && \
    find /usr/i686-w64-mingw32/lib -name '*.la' -print -delete  && \
    make clean && \
    ./configure \
      --prefix=/usr/x86_64-w64-mingw32 \
      --disable-shared \
      --enable-static \
      --host=x86_64-w64-mingw32 && \
    make && \
    make install && \
    find /usr/x86_64-w64-mingw32/lib -name '*.la' -print -delete && \
    cd / && \
    rm -rf /src/libsamplerate-${LIBSAMPLERATE_VER}


COPY . /src/lua-music-visualizer
WORKDIR /src/lua-music-visualizer

ENV OPT_CFLAGS="-O3 -DNDEBUG"

RUN \
    mkdir -p /dist/win32 && \
    mkdir -p /dist/win64 && \
    make -f Makefile.windows.docker LDFLAGS_LOCAL="-static-libgcc" TARGET=i686-w64-mingw32 clean && \
    make -f Makefile.windows.docker LDFLAGS_LOCAL="-static-libgcc" TARGET=i686-w64-mingw32 NASM_PLAT=win32 OPT_CFLAGS="$OPT_CFLAGS" -j4 && \
    i686-w64-mingw32-strip lua-music-visualizer.exe && \
    cp lua-music-visualizer.exe /dist/win32/ && \
    make -f Makefile.windows.docker LDFLAGS_LOCAL="-static-libgcc" TARGET=i686-w64-mingw32 clean && \
    make -f Makefile.windows.docker LDFLAGS_LOCAL="-static-libgcc" TARGET=x86_64-w64-mingw32 NASM_PLAT=win64 OPT_CFLAGS="$OPT_CFLAGS" -j4 && \
    x86_64-w64-mingw32-strip lua-music-visualizer.exe && \
    cp lua-music-visualizer.exe /dist/win64/ && \
    make -f Makefile.windows.docker ENABLE_SPC=1 ENABLE_NEZ=1 TARGET=x86_64-w64-mingw32 clean

FROM alpine:3.10 as linux-builder

ENV UPX_BIN=upx
ENV OPT_CFLAGS="-O3 -DNDEBUG"

RUN apk add zlib-dev nasm file pkgconfig make gcc g++ musl-dev luajit-dev libsamplerate-dev linux-headers && \
    mkdir -p /src

COPY --from=0 /src /src

WORKDIR /src/lua-music-visualizer

RUN make clean && \
    make NASM_PLAT=elf64 CC="gcc -static -fPIE" PKGCONFIG="pkg-config --static" OPT_CFLAGS="$OPT_CFLAGS" && \
    strip lua-music-visualizer && \
    mkdir -p /dist && \
    cp lua-music-visualizer /dist

FROM jprjr/osxcross:10.10 as osx-builder

RUN apk update && apk add nasm

ENV OPT_CFLAGS="-O3 -DNDEBUG"

ENV CROSS_TRIPLE=x86_64-apple-darwin14
ENV LUAJIT_VER=2.1.0-beta3
ENV LIBSAMPLERATE_VER=0.1.9

COPY --from=0 /src /src

RUN cd /src && \
    tar xf LuaJIT-${LUAJIT_VER}.tar.gz && \
    cd LuaJIT-${LUAJIT_VER}/src && \
    make TARGET_SYS=Darwin HOST_SYS=Linux HOST_CC="gcc" CC=o64-clang libluajit.a && \
    ${CROSS_TRIPLE}-ranlib libluajit.a

RUN cd /src && \
    tar xf libsamplerate-${LIBSAMPLERATE_VER}.tar.gz && \
    cd libsamplerate-${LIBSAMPLERATE_VER} && \
    CC=o64-clang \
    ./configure \
      --prefix=/opt/libsamplerate \
      --enable-shared \
      --enable-static \
      --host=${CROSS_TRIPLE} && \
    make && \
    make install &&\
    ${CROSS_TRIPLE}-ranlib /opt/libsamplerate/lib/libsamplerate.a && \
    rm -f /opt/libsamplerate/lib/libsamplerate.la && \
    rm -f /opt/libsamplerate/lib/libsamplerate.so

WORKDIR /src/lua-music-visualizer

RUN make clean
RUN make CC=o64-clang SAMPLERATE_CFLAGS="-I/opt/libsamplerate/include" LUA_CFLAGS="-I/src/LuaJIT-${LUAJIT_VER}/src" SAMPLERATE_LDFLAGS="-L/opt/libsamplerate/lib -lsamplerate" LUA_LDFLAGS="-L/src/LuaJIT-${LUAJIT_VER}/src -lluajit -pagezero_size 10000 -image_base 100000000" HOST_CC="gcc" OPT_CFLAGS="$OPT_CFLAGS" && \
    ${CROSS_TRIPLE}-strip lua-music-visualizer && \
    mkdir -p /dist && cp lua-music-visualizer /dist/

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
    true lua-music-visualizer && \
    tar cvzf ../lua-music-visualizer-$(../linux/lua-music-visualizer --version)-osx.tar.gz * && \
    cd ../linux && \
    true lua-music-visualizer && \
    tar cvzf ../lua-music-visualizer-$(./lua-music-visualizer --version)-linux.tar.gz * && \
    cd .. && \
    rm -rf win32 win64 linux osx

COPY entrypoint.sh /
ENTRYPOINT ["/entrypoint.sh"]
