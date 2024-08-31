#!/bin/bash

rm -rf wince_build
mkdir wince_build

BUILD_MACHINE=`$SHELL ./config.guess`

if test ! -d build/bin; then echo "not built yet!"; exit 1; fi

cp build/bin/python3.10.exe wince_build/
cp build/bin/*.dll wince_build/
cp WinCE/openssl/lib/*.dll wince_build/
cp WinCE/sqlite/lib/libsqlite3.dll wince_build/
cp WinCE/init.py wince_build/

python3.10 ce_mkpyc.py
cd build/lib/python3.10
zip -0 -@ python310.zip < ../../../zip.list

mv python310.zip ../../../wince_build/

cd ../../../

ls build/lib/python3.10/lib-dynload/*.so -d | sed 's/\.cpython.*//' | sed 's/^.*\///' | awk '{printf "cp build/lib/python3.10/lib-dynload/"$1".*.so  wince_build/"$1".cp310-wince_arm.pyd\n"}' | bash

if test -a wince_build/python310d.dll; then mv wince_build/python310.zip wince_build/python310d.zip; fi

cp -r tk84.dll tcl84.dll celib.dll zlib1.dll tcl8.4.3 wince_build/

echo "Done."
