import os, re, glob, py_compile

flist = []
base_path = "build/lib/python3.10/"

srcfiles = [*glob.glob(base_path + "**", recursive=True)]

for py in srcfiles:
    if py.endswith(".py") and not py.startswith(base_path + "test/") and not py.startswith(base_path + "lib2to3/"):
        output = py
        try:
            with open(py, mode="r") as f:
                src = re.split(
                        "(?<=\n)#[ \t]+DONT COMPILE START\n(?:.|\s)*?(?<=\n)#[ \t]+DONT COMPILE END\n",
                        f.read(),
                        flags=re.MULTILINE)
            if len(src) > 1:
                py = re.sub("\.py$", ".tmp.py", py)
                with open(py, mode="w") as f:
                    f.write("".join(src))
            py_compile.compile(py, output+"c", optimize=1, doraise=True)
            if len(src) > 1:
                os.remove(py)
        except:
            flist.append(output[len(base_path):])
        else:
            flist.append(output[len(base_path):] + "c")
    elif "__pycache__" not in py and not py.endswith(".pyc"):
        flist.append(py[len(base_path):])


with open("zip.list", mode="w") as f:
    f.write("\n".join(flist))
