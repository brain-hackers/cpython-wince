#!/bin/bash

rm -rf wince_build
mkdir wince_build
mkdir wince_build/Lib

BUILD_MACHINE=`$SHELL ./config.guess`

if test ! -d build/bin; then echo "not built yet!"; exit 1; fi

cp build/bin/python3.10.exe wince_build/
cp build/bin/*.dll wince_build/
cp build/lib.*/_sysconfigdata*.py wince_build/Lib/

mv build/lib/python3.10/os.py wince_build/Lib/
python3.10 ce_mkpyc.py
cd build/lib/python3.10
zip -0 -@ python310.zip < ../../../zip.list
cp ../../../wince_build/Lib/os.py ./

mv python310.zip ../../../wince_build/

cd ../../../

ls build/lib.wince-arm-3.10/*.so -d | sed 's/\.cpython.*//' | sed 's/^.*\///' | awk '{printf "cp build/lib.wince-arm-3.10/"$1".cpython-310-*.so  wince_build/"$1".pyd\n"}' | bash

echo -ne "Lib\npython310.zip\n.\nimport site" > wince_build/libpython3.10._pth

if test -a wince_build/libpython3.10d.dll; then mv wince_build/libpython3.10._pth wince_build/libpython3.10d._pth; fi

cp -r tk84.dll tcl84.dll celib.dll tcl8.4.3 wince_build/

echo "Done."
