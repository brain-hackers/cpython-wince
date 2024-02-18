/* Minimal main program -- everything is loaded from the library */

#include "Python.h"

#if defined(MS_WINDOWS) && !defined(MS_WINCE)
int
wmain(int argc, wchar_t **argv)
{
    return Py_Main(argc, argv);
}
#elif defined(MS_WINCE)

#ifndef WINCE_HASH
#define WINCE_HASH ""
#endif

int
WinMain(HINSTANCE hCurInst, HINSTANCE hPrevInst, wchar_t *lpsCmdLine, int nCmdShow)
{
    if (!WinCEShell_CheckHash(WINCE_HASH))
        return -1;
    return WinCEShell_WinMain(hCurInst, hPrevInst, lpsCmdLine, nCmdShow);
}
#else
int
main(int argc, char **argv)
{
    return Py_BytesMain(argc, argv);
}
#endif
