#!/bin/bash

export HOST_ARCH=arm-none-linux-gnu
export TOOL_PREFIX=${TOOL_PREFIX:-/usr/bin/arm-mingw32ce}
export CC=$TOOL_PREFIX-gcc
export CXX=$TOOL_PREFIX-g++
export CPP="$TOOL_PREFIX-g++ -E"
export AR=$TOOL_PREFIX-ar
export RANLIB=$TOOL_PREFIX-ranlib
export LD=$TOOL_PREFIX-ld
export READELF=$TOOL_PREFIX-readelf
export WINDRES=$TOOL_PREFIX-windres
export LIBS="-lcoredll6 -lcoredll -lm -laygshell -lws2 -lcommctrl"
export CFLAGS="-march=armv5tej -mcpu=arm926ej-s -Wno-attributes -DWC_NO_BEST_FIT_CHARS -D_WIN32_WCE=0x0600 -D_MAX_PATH=260 -D_UNICODE -DUNICODE -DPy_HAVE_ZLIB=1 -DLACK_OF_CRYPT_API -fvisibility=hidden -fno-pic -I./zlib-src -I./Modules/_ctypes/libffi_arm_wince"
export LDFLAGS="-fno-strict-aliasing"
export CPPFLAGS="-fvisibility=hidden"
export LIBFFI_INCLUDEDIR="Modules/_ctypes/libffi-arm-wince"
export ZLIBDIR="./zlib-src"

PY_DEBUG='no';

while (( $# > 0 ))
do
    case $1 in
        -d|--debug)
            PY_DEBUG='yes';
            ;;
        -*)
            echo "unknown option: "$1;
            exit 1;
            ;;
        *)
            echo "unexpected argument: "$1;
            exit 1;
            ;;
    esac
    shift
done


build_tkinter='yes' # set 'no' or '' if you do not need tkinter being built.

TCLTK_INCS=""
TCLTK_LIBS=""

if [ "x$build_tkinter" != 'x' ]; then
    TCLTK_INCS="-IPC/WinCE/_tkinter" && \
    TCLTK_LIBS="-L. -ltk84 -ltcl84"
fi


function err() {
    echo "error!" 1>&2
    exit 1
}


if [ ! -d build ]; then
    mkdir build
fi

touch make.log

ac_cv_pthread_is_default=yes ac_cv_cxx_thread=yes ac_cv_file__dev_ptmx=no ac_cv_file__dev_ptc=no ac_cv_have_long_long_format=yes \
ac_cv_enable_implicit_function_declaration_error=no \
./configure --prefix=$PWD/build \
--build=`$SHELL ./config.guess || echo 'unknown'` --host=mingw32ce --target=mingw32ce \
--disable-ipv6 \
--enable-shared \
--with-ensurepip=no \
--with-builtin-hashlib-hashes \
--without-c-locale-coercion \
--includedir="$PWD/PC" \
--with-pydebug=$PY_DEBUG \
--with-tcltk-includes="$TCLTK_INCS" \
--with-tcltk-libs="$TCLTK_LIBS" \
--enable-optimizations |& tee make.log -a || err

cat PC/pyconfig.h.org | grep -v "#endif /\* \!Py_CONFIG_H \*/" > PC/pyconfig.h
cat pyconfig.h PC/pyconfig.h | grep "^#\s*define [A-Z0-9_]*" | sed "s/#\s*define/#define/" | sort | awk '{printf $1" "$2"\n"}' | uniq -c | awk '$1=="1"{printf "#define "$3"\n"}' > pyconfig.pre.tmp
cat pyconfig.pre.tmp pyconfig.h | grep "^#\s*define [A-Z0-9_]*" | sed "s/#\s*define/#define/" | awk '{printf "#define "$2"\n"}' | sort | uniq -c | awk '$1=="2"{printf $3"\n"}' > pyconfig.tmp

cat pyconfig.h | grep "^#define PY_BUILTIN_HASHLIB_HASHES" >> PC/pyconfig.h
echo >> PC/pyconfig.h

#for i in `cat pyconfig.tmp`; do cat pyconfig.h | grep "^#\s*define $i " | sed "s/#\s*define/#define/" | uniq >> PC/pyconfig.h; done

echo "#endif /* !Py_CONFIG_H */" >> PC/pyconfig.h

#rm pyconfig.tmp pyconfig.pre.tmp
cp PC/pyconfig.h Modules/

make -j $(nproc) \
BLDSHARED="$TOOL_PREFIX-gcc -shared" \
CROSS-COMPILE=$TOOL_PREFIX- CROSS_COMPILE_TARGET=yes |& tee make.log -a || err

make install \
BLDSHARED="$TOOL_PREFIX-gcc -shared" \
CROSS-COMPILE=$TOOL_PREFIX- CROSS_COMPILE_TARGET=yes |& tee make.log -a || err

