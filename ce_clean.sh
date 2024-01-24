#!/bin/bash

rm -rf pyconfig.tmp pyconfig.pre.tmp zip.list wince_build make.log

if test -e Makefile; then
    make distclean;
fi

echo "Clean up finished."
