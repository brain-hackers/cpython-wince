#!/bin/bash

rm -rf pyconfig.tmp pyconfig.pre.tmp zip.list wince_build make.log
rm -rf libcrypto-3.dll libssl-3.dll

if test -e Makefile; then
    make distclean;
fi

rm -rf WinCE/bzip2
rm -rf WinCE/lzma

echo "Clean up finished."
