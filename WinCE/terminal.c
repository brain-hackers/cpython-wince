#include "Python.h"
#include "terminal.h"

#include <windows.h>

#define ID_SHELLLOG 1
#define ID_TEXTINPUT 2
#define ID_PREFIX 3

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

wchar_t **wargv;
int argc;

HWND hConsoleWindow;

HANDLE hTh;
HANDLE ghInitializedEv;
HANDLE ghFinalizeEv;
HANDLE ghFinalizeDoneEv;
HANDLE ghReadlineEv;
HANDLE ghReadlinePopEv;
HANDLE ghWriteConsoleEv;
HANDLE ghWaitForCharEv;

HWND shelllog, prefixBox;

int isPressed = 0;
int isUngetchDone = 0;
int isWaitingForChar = 0;
int doGetchEcho = 0;
long scrolledTop = 0;

long shelllogIndex;

LONG MAX_WIDTH;
LONG W_WIDTH, W_HEIGHT;

LONG fontW, fontH, fontPadX, fontPadY = 0L;
BOOL PaintInitDone = FALSE;
int Exited = 0;

#define BUILD_PYTHONW_FOR_WINCE 1

#ifdef BUILD_PYTHONW_FOR_WINCE
int showConsole = 1;
#else
int showConsole = 0;
#endif

int
WinCEShell_fileno(FILE *stream)
{
    int fd = (int)_fileno(stream);
    if (fd >= 0)
        return fd;
    if ((stream == stdin || stream == stdout || stream == stderr) && showConsole) {
        if (stream == stdin)
            return 0;
        if (stream == stdout)
            return 1;
        if (stream == stderr)
            return 2;
    }
    return fd;
}

int
WinCEShell_isatty(int fd)
{
    if (fd >= 0) {
        if (fd == fileno(stdin) || fd == fileno(stdout) || fd == fileno(stderr))
            return 1;
    }
    return 0;
}

LONG
CalcTextWidth(int num)
{
    if (num == 0)
        return 0L;
    return fontW * num + fontPadX * (num - 1);
}

LONG
CalcTextHeight(int num)
{
    if (num == 0)
        return 0L;
    return fontH * num + fontPadY * (num - 1);
}

int
shift_array(wchar_t *arr, size_t arr_size, size_t count)
{
    if (count == 0)
        return 1;
    if (arr == NULL)
        return 0;

    wchar_t *tmp = (wchar_t *)calloc(arr_size, sizeof(wchar_t));
    if (tmp == NULL)
        return 0;

    memcpy(tmp, arr + count, arr_size - count);
    memcpy(arr, tmp, arr_size);
    free(tmp);
    return 1;
}

int histIndex;
int histLen;
wchar_t **history;
wchar_t *curText;
wchar_t *prefixText;

wchar_t *logBuf;

wchar_t **readlineBuf;
wchar_t readConsoleBuf[BUFSIZ + 1] = {};

wchar_t lastChar[1];

long cmdH, logH, prefixW, logLines, logAdditionalLines, cmdLines, readlineBufLen;

COLORREF fgColor = 0x00ffffff;
COLORREF bgColor = 0x00331100;

HBRUSH fgBrush, bgBrush;

WNDPROC TextInputProc, ShellLogProc;

void
SetCmdline(HWND hWnd)
{
    SetWindowText(hWnd, curText);
    UpdateWindow(hWnd);
}

void
SetPrefix(HWND hWnd)
{
    if (prefixText != NULL)
        prefixW = CalcTextWidth(wcsnlen(prefixText, 64));
    else
        prefixW = 0;
    MoveWindow(prefixBox, 0, logH + scrolledTop, prefixW, cmdH, FALSE);
    SetWindowText(hWnd, prefixText);
    UpdateWindow(hWnd);
}

void
SetShelllog(HWND hWnd)
{
    HDC hdc;
    RECT rc;
    rc.right = W_WIDTH;
    hdc = GetDC(hWnd);
    logH =
        DrawText(hdc, logBuf, -1, &rc, DT_CALCRECT | DT_EDITCONTROL | DT_NOPREFIX | DT_WORDBREAK);
    ReleaseDC(hWnd, hdc);
    logAdditionalLines = (logH + fontPadY) / (fontH + fontPadY) - logLines;
    if (hConsoleWindow != NULL && PaintInitDone) {
        SetScrollRange(hConsoleWindow, SB_VERT, 0, logLines + logAdditionalLines, FALSE);
        SendMessage(
            hConsoleWindow, WM_VSCROLL,
            (DWORD)(
                SB_THUMBPOSITION |
                ((logLines + logAdditionalLines - (W_HEIGHT + fontPadY) / (fontH + fontPadY) - 1)
                 << (sizeof(DWORD) / 2))),
            NULL);
    }
    SetWindowText(hWnd, logBuf);
    UpdateWindow(hWnd);
}

int
GetHistory(wchar_t *output, int index)
{
    if (index < 0 || histLen <= index)
        return 0;
    wcscpy(output, history[index]);
    return 1;
}

int
addReadline(wchar_t *text)
{
    DWORD res = WaitForSingleObject(ghReadlinePopEv, INFINITE);
    switch (res) {
        case WAIT_OBJECT_0:
            break;
        defalut:
            return 0;
    }
    ResetEvent(ghReadlineEv);
    wchar_t **tmpBuf;
    tmpBuf = (wchar_t **)realloc(readlineBuf, (readlineBufLen + 1) * sizeof(wchar_t *));
    if (tmpBuf == NULL) {
        SetEvent(ghReadlineEv);
        return 0;
    }
    tmpBuf[readlineBufLen] = (wchar_t *)calloc(wcslen(text) + 1, sizeof(wchar_t));
    if (tmpBuf[readlineBufLen] == NULL) {
        tmpBuf = realloc(tmpBuf, readlineBufLen * sizeof(wchar_t *));
        SetEvent(ghReadlineEv);
        if (tmpBuf == NULL)
            return -1;
        return 0;
    }
    wcscpy(tmpBuf[readlineBufLen], text);
    readlineBuf = tmpBuf;
    readlineBufLen++;
    wcscat(prefixText, L"");
    SetPrefix(prefixBox);
    SetEvent(ghReadlineEv);
    return 1;
}

int
popReadline(wchar_t *output)
{
    if (output == NULL)
        return 0;
    DWORD res = WaitForSingleObject(ghReadlineEv, INFINITE);
    switch (res) {
        case WAIT_OBJECT_0:
            break;
        defalut:
            return 0;
    }
    ResetEvent(ghReadlinePopEv);
    wchar_t **tmpBuf;
    if (readlineBufLen == 1) {
        ResetEvent(ghReadlineEv);
        SetEvent(ghReadlinePopEv);
        if (!Exited)
            return 0;
        else
            return -1;
    }
    if (readlineBuf[1] == NULL)
        return 0;
    tmpBuf = (wchar_t **)calloc(readlineBufLen - 1, sizeof(wchar_t *));
    if (tmpBuf == NULL) {
        SetEvent(ghReadlinePopEv);
        return 0;
    }
    wcscpy(output, readlineBuf[1]);
    free(readlineBuf[1]);
    int i;
    for (i = 2; i < readlineBufLen; i++) {
        tmpBuf[i - 1] = readlineBuf[i];
    }
    free(readlineBuf);
    readlineBuf = tmpBuf;
    if (readlineBufLen - 1 == 1)
        ResetEvent(ghReadlineEv);
    readlineBufLen--;
    SetEvent(ghReadlinePopEv);
    return 1;
}

char *
WinCEShell_readline(FILE *sys_stdin, FILE *sys_stdout, const char *prefix)
{
    if (!showConsole)
        return NULL;
    int res;
    wchar_t *tmp;
    int prefixLen = 0;
    tmp = (wchar_t *)calloc(65536, sizeof(wchar_t));
    if (tmp == NULL)
        return NULL;
    if (prefixText != NULL)
        free(prefixText);
    if (prefix != NULL)
        prefixText = (wchar_t *)calloc(strlen(prefix) + 1, sizeof(wchar_t));
    else
        prefixText = (wchar_t *)calloc(1, sizeof(wchar_t));
    if (prefixText == NULL) {
        free(tmp);
        return NULL;
    }
    if (prefix != NULL) {
        prefixLen = strlen(prefix);
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, prefix, -1, prefixText, strlen(prefix) + 1);
    }
    wcscpy(tmp, prefixText);
    SetPrefix(prefixBox);
    res = popReadline(tmp + prefixLen);
    if (res == 0) {
        free(tmp);
        return NULL;
    }
    if (res < 0) {
        free(tmp);
        PyErr_SetString(PyExc_SystemExit, "");
        return NULL;
    }
    int c;
    if (tmp != NULL)
        WinCEShell_WriteConsole(NULL, tmp, wcslen(tmp), &c, NULL);
    char *result;
    int n;
    n = WideCharToMultiByte(CP_UTF8, 0, tmp + prefixLen, -1, NULL, 0, NULL, NULL);
    result = (char *)PyMem_RawCalloc(n, sizeof(char));
    if (result == NULL) {
        free(tmp);
        return NULL;
    }
    WideCharToMultiByte(CP_UTF8, 0, tmp + prefixLen, -1, result, n, NULL, NULL);
    free(tmp);
    return result;
}

LRESULT CALLBACK
HandleInput(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    HDC hdc;
    RECT rc;
    PAINTSTRUCT pt = {0};
    void *histTmp;
    int res;
    switch (msg) {
        case WM_CHAR:
            lastChar[0] = (wchar_t)wp;
            if (wcsnlen(readConsoleBuf, BUFSIZ) > BUFSIZ - 1)
                shift_array(readConsoleBuf, sizeof(readConsoleBuf), 1);
            wcscat(readConsoleBuf, lastChar);
            SetEvent(ghWaitForCharEv);
            if (!isWaitingForChar || doGetchEcho)
                return CallWindowProc(TextInputProc, hWnd, msg, wp, lp);
            else
                return 0;
        case WM_KEYDOWN:
            isPressed = 1;
            switch (wp) {
                case VK_RETURN:
                    GetWindowText(hWnd, curText, 77);
                    histTmp = realloc(history, sizeof(history) + sizeof(wchar_t *));
                    if (histTmp == NULL) {
                        // Could not re-allocate buffer for history
                        MessageBox(NULL, L"ReallocFail!", L"error message", MB_OK);
                        free(history[0]);
                        int i;
                        for (i = 1; i < sizeof(history) / sizeof(wchar_t *); i++)
                            history[i - 1] = history[i];
                        histIndex = histLen - 1;
                    }
                    else {
                        history = (wchar_t **)histTmp;
                        histIndex = histLen;
                        histLen++;
                    }
                    history[histIndex] = (wchar_t *)calloc(80, sizeof(wchar_t));
                    wcscpy(history[histIndex - 1], curText);
                    wcscat(curText, L"\r\n");
                    if (WaitForSingleObject(ghReadlineEv, 0) != WAIT_TIMEOUT) {
                        int c;
                        WinCEShell_WriteConsole(NULL, curText, 80, &c, NULL);
                    }
                    res = addReadline(curText);
                    if (res == 0)
                        MessageBox(NULL, L"addReadline Failed (not critical)", L"error occured",
                                   MB_OK);
                    else if (res == -1)
                        MessageBox(NULL, L"addReadline Failed ( CRITICAL! )", L"error occured",
                                   MB_OK);
                    wcscpy(curText, L"\0");
                    SetCmdline(hWnd);
                    // if (!SetCmdline(hWnd))
                    //	MessageBox(NULL, L"SetCmdlineFail!", L"error message", MB_OK);
                    return 0;
                case VK_TAB:
                    return 0;
                case VK_UP:
                    if (histIndex == histLen - 1)
                        wcscpy(history[histIndex], curText);
                    GetWindowText(hWnd, curText, 80);
                    if (GetHistory(curText, histIndex - 1)) {
                        histIndex--;
                        SetCmdline(hWnd);
                    }
                    return 0;
                case VK_DOWN:
                    GetWindowText(hWnd, curText, 80);
                    if (GetHistory(curText, histIndex + 1)) {
                        histIndex++;
                        SetCmdline(hWnd);
                    }
                    return 0;
            }
            return CallWindowProc(TextInputProc, hWnd, msg, wp, lp);
        case WM_KEYUP:
            isPressed = 0;
            ResetEvent(ghWaitForCharEv);
            return CallWindowProc(TextInputProc, hWnd, msg, wp, lp);
        case WM_PAINT:
            rc.right = W_WIDTH;
            hdc = BeginPaint(hWnd, &pt);
            SetTextColor(hdc, fgColor);
            SetBkColor(hdc, bgColor);
            rc.bottom = DrawText(hdc, curText, -1, &rc,
                                 DT_CALCRECT | DT_EDITCONTROL | DT_NOPREFIX | DT_WORDBREAK);
            rc.top += scrolledTop;
            rc.bottom += scrolledTop;
            DrawText(hdc, curText, -1, &rc, DT_EDITCONTROL | DT_NOPREFIX | DT_WORDBREAK);
            EndPaint(hWnd, &pt);
            return CallWindowProc(TextInputProc, hWnd, msg, wp, lp);
        case WM_CTLCOLOREDIT:
            SetTextColor((HDC)wp, bgColor);
            SetBkColor((HDC)wp, fgColor);
            return (LRESULT)bgBrush;
        default:
            return CallWindowProc(TextInputProc, hWnd, msg, wp, lp);
    }
    return CallWindowProc(TextInputProc, hWnd, msg, wp, lp);
}

LRESULT CALLBACK
HandleShellLog(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    HDC hdc;
    SIZE sz;
    RECT rc;
    int c;
    wchar_t *ch, ch2;
    PAINTSTRUCT pt = {0};
    switch (msg) {
        case WM_COMMAND:
            switch (HIWORD(wp)) {
                default:
                    return CallWindowProc(ShellLogProc, hWnd, msg, wp, lp);
            }
            break;
        case WM_PAINT:
            rc.top = scrolledTop;
            rc.bottom = logH + scrolledTop;
            rc.right = W_WIDTH;
            hdc = BeginPaint(hWnd, &pt);
            SetTextColor(hdc, fgColor);
            SetBkColor(hdc, bgColor);
            DrawText(hdc, logBuf, -1, &rc, DT_EDITCONTROL | DT_NOPREFIX | DT_WORDBREAK);
            EndPaint(hWnd, &pt);
            // ch = logBuf;
            /*logAdditionalLines = 0;
            int i = 0;
            while (*ch != L'\0')
            {
                i++;
                ch2 = wcschr(logBuf, L'\n');
                GetTextExtentExPoint(hdc, ch, (WORD)((ch2 != NULL ? ch2 : (logBuf+shelllogIndex)) -
            ch), W_WIDTH, &c, NULL, &sz); if (*(ch+c) == L'\n') ch++; else
                {
                    logAdditionalLines++;
                    MoveWindow(hWnd, 0, scrolledTop, W_WIDTH,
            CalcTextHeight(logLines+logAdditionalLines), FALSE);
                }
                if (
                ExtTextOut(hdc, 0, scrolledTop+CalcTextHeight(logLines+logAdditionalLines)-fontH, 0,
            NULL, ch, c, NULL); ch += c;
            }*/
            return CallWindowProc(ShellLogProc, hWnd, msg, wp, lp);
        default:
            return CallWindowProc(ShellLogProc, hWnd, msg, wp, lp);
    }
    return 0;
}

LRESULT CALLBACK
WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static HWND textinput;
    static int y;
    RECT rc;
    SCROLLINFO scrinfo;
    HDC hdc;
    TEXTMETRIC tm = {0};
    PAINTSTRUCT pt = {0};
    int dy;
    switch (msg) {
        case WM_CREATE:
            y = 0;
            shelllog = CreateWindow(
                L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_READONLY, 0, 0, 0,
                0, hWnd, (HMENU)ID_SHELLLOG, ((LPCREATESTRUCT)(lp))->hInstance, NULL);
            textinput = CreateWindow(
                L"EDIT", L"(text input)", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE, 0, 0, 0,
                0, hWnd, (HMENU)ID_TEXTINPUT, ((LPCREATESTRUCT)(lp))->hInstance, NULL);
            prefixBox = CreateWindow(L"STATIC", L">>> ", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd,
                                     (HMENU)ID_PREFIX, ((LPCREATESTRUCT)(lp))->hInstance, NULL);
            TextInputProc = (WNDPROC)SetWindowLong(textinput, GWL_WNDPROC, (DWORD)HandleInput);
            ShellLogProc = (WNDPROC)SetWindowLong(shelllog, GWL_WNDPROC, (DWORD)HandleShellLog);
            scrinfo.cbSize = sizeof(SCROLLINFO);
            scrinfo.fMask = SIF_RANGE;
            scrinfo.nMin = 0;
            scrinfo.nMax = 100;
            SetScrollInfo(hWnd, SB_VERT, &scrinfo, TRUE);
            wcscpy(curText, L"\0");
            int c;
            SetCmdline(textinput);
            SetShelllog(shelllog);
            break;
        case WM_SIZE:
            if (W_WIDTH != (long)LOWORD(lp)) {
                hdc = GetDC(shelllog);
                logAdditionalLines = 0;
                rc.right = (long)LOWORD(lp);
                logH = DrawText(hdc, logBuf, -1, &rc,
                                DT_CALCRECT | DT_EDITCONTROL | DT_NOPREFIX | DT_WORDBREAK);
                ReleaseDC(shelllog, hdc);
                logAdditionalLines = (logH + fontPadY) / (fontH + fontPadY) - logLines;
            }
            W_WIDTH = (long)LOWORD(lp);
            W_HEIGHT = (long)HIWORD(lp);
            MoveWindow(shelllog, 0, scrolledTop, W_WIDTH, logH, TRUE);
            MoveWindow(prefixBox, 0, logH + scrolledTop, prefixW, cmdH, TRUE);
            MoveWindow(textinput, prefixW, logH + scrolledTop, W_WIDTH - prefixW, cmdH, TRUE);
            break;
        case WM_VSCROLL:
            switch (LOWORD(wp)) {
                case SB_LINEUP:
                    dy = -1;
                    break;
                case SB_LINEDOWN:
                    dy = 1;
                    break;
                case SB_THUMBPOSITION:
                    dy = HIWORD(wp) - y;
                    break;
                case SB_PAGEUP:
                    dy = -10;
                    break;
                case SB_PAGEDOWN:
                    dy = 10;
                    break;
                default:
                    dy = 0;
                    break;
            }
            if (y + dy < 0)
                dy = -y;
            if (logLines + logAdditionalLines - (y + dy) < 0)
                dy = logLines + logAdditionalLines - y;
            if (dy != 0) {
                y += dy;
                scrolledTop = -1 * CalcTextHeight(y);
                ScrollWindowEx(hWnd, 0, (-dy / abs(dy)) * CalcTextHeight(abs(dy)), NULL, NULL, NULL,
                               NULL, SW_SCROLLCHILDREN);
                SetScrollPos(hWnd, SB_VERT, y, TRUE);
                UpdateWindow(hWnd);
            }
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &pt);
            if (!PaintInitDone) {
                PaintInitDone = TRUE;
                GetTextMetrics(hdc, &tm);
                fontW = tm.tmMaxCharWidth;
                fontH = tm.tmHeight;
                fontPadX = tm.tmOverhang;
                fontPadY = tm.tmExternalLeading;
                cmdH = CalcTextHeight(1);
                logH = CalcTextHeight(0);
                cmdLines = 1;
                logLines = 0;
                logAdditionalLines = 0;
                prefixW = CalcTextWidth(wcslen(L""));
                MoveWindow(shelllog, 0, scrolledTop, LOWORD(lp), logH, TRUE);
                MoveWindow(prefixBox, 0, logH + scrolledTop, prefixW, cmdH, TRUE);
                MoveWindow(textinput, prefixW, logH + scrolledTop, LOWORD(lp) - prefixW, cmdH,
                           TRUE);
            }
            SetTextColor(hdc, fgColor);
            SetBkColor(hdc, bgColor);
            EndPaint(hWnd, &pt);
            break;  // return DefWindowProc(hWnd, msg, wp, lp);
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC:
            SetTextColor((HDC)wp, fgColor);
            SetBkColor((HDC)wp, bgColor);
            return (LRESULT)bgBrush;
        case WM_DESTROY:
            if (fgBrush)
                DeleteObject(fgBrush);
            if (bgBrush)
                DeleteObject(bgBrush);
            Exited = 1;
            break;
        default:
            return DefWindowProc(hWnd, msg, wp, lp);
    }
    return 0;
}

typedef struct {
    HINSTANCE hCurInst;
    HINSTANCE hPrevInst;
    LPWSTR lpsCmdLine;
    int nCmdShow;
} WINCE_SHELL_ARGS;

int
WinCEShell(HINSTANCE hCurInst)
{
    if (showConsole) {
        const wchar_t CLASS_NAME[] = L"Python310WinCE";
        WNDCLASS wc = {};

        fgBrush = CreateSolidBrush(fgColor);
        bgBrush = CreateSolidBrush(bgColor);

        prefixText = NULL;
        curText = (wchar_t *)calloc(80, sizeof(wchar_t));
        logBuf = (wchar_t *)calloc(65536, sizeof(wchar_t));
        history = (wchar_t **)calloc(1, sizeof(wchar_t *));
        history[0] = (wchar_t *)calloc(80, sizeof(wchar_t));
        readlineBuf = (wchar_t **)calloc(1, sizeof(wchar_t *));
        readlineBufLen = 1;
        histLen = 1;
        histIndex = 0;

        wc.lpfnWndProc = WndProc;
        wc.hInstance = hCurInst;
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = bgBrush;

        RegisterClass(&wc);
        hConsoleWindow =
            CreateWindow(CLASS_NAME, L"Python 3.10 for WinCE", WS_OVERLAPPEDWINDOW | WS_VSCROLL, 0,
                         0, 480, 288, NULL, NULL, hCurInst, NULL);

        ShowWindow(hConsoleWindow, SW_SHOW);
        UpdateWindow(hConsoleWindow);
    }

    ghReadlineEv = CreateEvent(NULL, TRUE, FALSE, L"readlineEv");
    ghReadlinePopEv = CreateEvent(NULL, TRUE, TRUE, L"readlinePopEv");
    ghWriteConsoleEv = CreateEvent(NULL, TRUE, TRUE, L"WriteConsoleEv");
    ghWaitForCharEv = CreateEvent(NULL, TRUE, FALSE, L"waitForCharEv");
    SetEvent(ghInitializedEv);

    MSG msg;
    BOOL value;
    while (!Exited && showConsole) {
        value = GetMessage(&msg, NULL, 0, 0);
        if (value == 0)
            break;
        if (value == -1)
            return -1;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (Exited) {
            SetEvent(ghReadlineEv);
            WaitForSingleObject(ghFinalizeEv, INFINITE);
            SetEvent(ghFinalizeDoneEv);
            PostQuitMessage(0);
        }
    }

    return 0;
}

int
WinCEShell_close(int fd)
{
    return 0;
}

wint_t
WinCEShell_getwch()
{
    wint_t result;
    isWaitingForChar = 1;
    if (WaitForSingleObject(ghWaitForCharEv, 0) == WAIT_TIMEOUT)
        ResetEvent(ghWaitForCharEv);
    WaitForSingleObject(ghWaitForCharEv, INFINITE);
    isWaitingForChar = 0;
    result = (wint_t)lastChar[0];
    isUngetchDone = 0;
    return result;
}

wint_t
WinCEShell_getwche()
{
    doGetchEcho = 1;
    wint_t result = WinCEShell_getwch();
    doGetchEcho = 0;
    return result;
}

int
WinCEShell_getch()
{
    char *output;
    wchar_t *result;
    isWaitingForChar = 1;
    WaitForSingleObject(ghWaitForCharEv, INFINITE);
    isWaitingForChar = 0;
    result = (wint_t)lastChar[0];
    isUngetchDone = 0;
    WideCharToMultiByte(CP_ACP, MB_USEGLYPHCHARS, result, 1, output, 1, NULL, NULL);
    return (int)(*output);
}

int
WinCEShell_getche()
{
    doGetchEcho = 1;
    int result = WinCEShell_getch();
    doGetchEcho = 0;
    return result;
}

wint_t
WinCEShell_ungetwch(wint_t c)
{
    if (c == WEOF || isUngetchDone)
        return WEOF;
    lastChar[0] = (wchar_t)c;
    isUngetchDone = 1;
    return c;
}

int
WinCEShell_ungetch(int c)
{
    if (c == EOF || isUngetchDone)
        return EOF;
    char ch = (char)c;
    wchar_t *wch;
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &ch, 1, wch, 1);
    lastChar[0] = *wch;
    isUngetchDone = 1;
    return c;
}

int
WinCEShell_kbhit()
{
    return isPressed;
}

wint_t
WinCEShell_putwch(wint_t c)
{
    wchar_t *buf = (wchar_t)c;
    int n;
    if (WinCEShell_WriteConsole(NULL, buf, (DWORD)1, &n, NULL))
        return c;
    return WEOF;
}

int
InitializeProcThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
                                  unsigned long dwAttributeCount, unsigned long dwFlags,
                                  size_t *lpSize)
{
    return 0;  // currently not supported
}

int
UpdateProcThreadAttribute(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList, unsigned long dwFlags,
                          unsigned long *Attribute, void *lpValue, size_t cbSize,
                          void *lpPreviousValue, size_t *lpReturnSize)
{
    return 0;  // currently not supported
}

void
DeleteProcThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList)
{
}

wint_t
WinCEShell_putch(int c)
{
    wchar_t buf[2];
    char ch = (char)c;
    int n;
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &ch, 1, buf, 2);
    if (WinCEShell_WriteConsole(NULL, buf, (DWORD)1, &n, NULL))
        return c;
    return EOF;
}

BOOL
WinCEShell_GetNumberOfConsoleInputEvents(HANDLE handle, LPDWORD *count)
{
    *count = 0;
    return handle == (HANDLE)fileno(stdin);
}

BOOL
WinCEShell_WriteConsole(HANDLE handle, wchar_t *lpBuffer, DWORD nNumberOfCharsToWrite,
                        LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
    long len;
    int redraw = 0;

    if (lpBuffer == NULL)
        return 0;
    /*if (wcsnlen(lpBuffer, nNumberOfCharsToWrite+1) == nNumberOfCharsToWrite+1)
    {
            return 0;
    }*/

    if (nNumberOfCharsToWrite == 0)
        return 1;

    WaitForSingleObject(ghWriteConsoleEv, INFINITE);
    ResetEvent(ghWriteConsoleEv);

    for (len = 0; len < nNumberOfCharsToWrite; len++) {
        if (*(lpBuffer + len) == L'\0')
            break;
        if (*(lpBuffer + len) == L'\n') {
            logLines += 1;
            redraw = 1;
        }
        *(logBuf + (shelllogIndex + len)) = *(lpBuffer + len);
    }

    if (redraw) {
        logH = CalcTextHeight(logLines);
        MoveWindow(shelllog, 0, scrolledTop, W_WIDTH, logH, TRUE);
    }
    SetShelllog(shelllog);
    UpdateWindow(prefixBox);

    *lpNumberOfCharsWritten = len;

    shelllogIndex += len;

    SetEvent(ghWriteConsoleEv);
    return 1;
}

BOOL
WinCEShell_ReadConsole(HANDLE handle, LPVOID lpBuffer, DWORD nNumberOfCharsToRead,
                       LPDWORD lpNumberOfCharsRead, LPVOID pInputControl)
{
    if (Exited || pInputControl != NULL) {
        return 0;
    }
    if (nNumberOfCharsToRead == 0) {
        *lpNumberOfCharsRead = 0;
        memset(lpBuffer, 0, 1);
        return 1;
    }
    int i = wcslen(readConsoleBuf);
    while (i < nNumberOfCharsToRead || i < BUFSIZ) {
        if ((wchar_t)WinCEShell_getwch() == L'\n')
            break;
    }
    wcsncpy(lpBuffer, readConsoleBuf, i);
    shift_array(readConsoleBuf, sizeof(readConsoleBuf), i);
    return 1;
}

void
WinCEShell_Cleanup()
{
    if (wargv != NULL) {
        for (int i = 0; i < argc; i++) free(wargv[i]);
        free(wargv);
    }
    SetEvent(ghFinalizeEv);
    WaitForSingleObject(ghFinalizeDoneEv, 10);
}

int
WinCEShell_WinMain(HINSTANCE hCurInst, HINSTANCE hPrevInst, LPWSTR lpsCmdLine, int nCmdShow)
{
    DWORD dwThId;
    int exitcode;
    char *result;
    char *prefix = "";

    WINCE_SHELL_ARGS shellArgs = {0};

    PyOS_ReadlineFunctionPointer = &WinCEShell_readline;  // defined at Parser/myreadline.c

    ghInitializedEv = CreateEvent(NULL, TRUE, FALSE, L"initializedEv");
    ghFinalizeEv = CreateEvent(NULL, TRUE, FALSE, L"finalizeEv");
    ghFinalizeDoneEv = CreateEvent(NULL, TRUE, FALSE, L"finalizeDoneEv");

    if (showConsole) {
        hTh = CreateThread(NULL, 0, WinCEShell, hCurInst, 0, &dwThId);
    }

    WaitForSingleObject(ghInitializedEv, INFINITE);

    wargv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (Py_AtExit(&WinCEShell_Cleanup) < 0)
        MessageBox(NULL, L"Py_AtExit returned -1 so cleanup will not work correctly.", L"WARNING",
                   MB_OK);

    if (wargv == NULL) {
        MessageBox(NULL, L"Failed to parse the command line", L"ERROR", MB_OK);
        exitcode = -1;
    }
    else {
        exitcode = Py_Main(argc, wargv);
    }

    if (exitcode)
        MessageBox(NULL, L"returned code was not 0", L"ERROR", MB_OK);

    WaitForSingleObject(hTh, INFINITE);
    CloseHandle(hTh);
    free(history);
    free(result);
    return 0;
}
