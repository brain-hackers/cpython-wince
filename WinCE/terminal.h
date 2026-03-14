#ifndef Py_TERMINAL_H
#define Py_TERMINAL_H

char *wince_getenv(const char *);
int WinCEShell_fileno(FILE *);
int WinCEShell_isatty(int);
int WinCEShell_kbhit();

WINCE_PyAPI_FUNC(char *) wince_getenv(const char *);
WINCE_PyAPI_FUNC(int) WinCEShell_WinMain(HINSTANCE, HINSTANCE, LPWSTR, int);
WINCE_PyAPI_FUNC(int) WinCEShell_CheckHash(char *);
WINCE_PyAPI_FUNC(int) WinCEShell_kbhit();

#endif /* !Py_TERMINAL_H */
