import shutil
import os

srcdir = os.path.dirname(__file__)
filepath = [srcdir + "\\libcrypto.dll", srcdir + "\\libssl.dll", srcdir + "\\libsqlite3.dll"]

for file in filepath:
    shutil.move(file, "\\Windows\\")