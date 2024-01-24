/*
 * Provides functions that are available on Windows generally but
 * not on Windows CE.
 * Some of the declarations are taken from the public domain MinGW headers.
 */

#ifndef WINCE_COMPATIBILITY_H
#define WINCE_COMPATIBILITY_H

//#include <windows.h>
#ifndef _HRESULT_DEFINED
typedef long HRESULT;
#define _HRESULT_DEFINED
#endif
#include <windows.h>
#include <winerror.h>

#ifndef va_list
#include <stdarg.h>
#endif

#include <fcntl.h>

#define PYTHON_DLL_NAME "libpython3.10.dll"
#define PY3_DLLNAME L"libpython3.10.dll"

/* locale emulation */
#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MONETARY 3
#define LC_NUMERIC 4
#define LC_TIME 5
#define LC_MIN LC_ALL
#define LC_MAX LC_TIME

/* FIXME: this file is included before pyport.h so we can't use PyAPI_FUNC() */
#ifdef Py_BUILD_CORE
#define WINCE_PyAPI_FUNC(RTYPE) __declspec(dllexport) RTYPE
#define WINCE_PyAPI_DATA(RTYPE) extern __declspec(dllexport) RTYPE
#else
#define WINCE_PyAPI_FUNC(RTYPE) __declspec(dllimport) RTYPE
#define WINCE_PyAPI_DATA(RTYPE) extern __declspec(dllimport) RTYPE
#endif

/* errno emulation */

#define EPERM 1   /* Operation not permitted */
#define ENOFILE 2 /* No such file or directory */
#define ENOENT 2
#define ESRCH 3   /* No such process */
#define EINTR 4   /* Interrupted function call */
#define EIO 5     /* Input/output error */
#define ENXIO 6   /* No such device or address */
#define E2BIG 7   /* Arg list too long */
#define ENOEXEC 8 /* Exec format error */
#define EBADF 9   /* Bad file descriptor */
#define ECHILD 10 /* No child processes */
#define EAGAIN 11 /* Resource temporarily unavailable */
#define ENOMEM 12 /* Not enough space */
#define EACCES 13 /* Permission denied */
#define EFAULT 14 /* Bad address */
/* 15 - Unknown Error */
#define EBUSY 16   /* strerror reports "Resource device" */
#define EEXIST 17  /* File exists */
#define EXDEV 18   /* Improper link (cross-device link?) */
#define ENODEV 19  /* No such device */
#define ENOTDIR 20 /* Not a directory */
#define EISDIR 21  /* Is a directory */
#define EINVAL 22  /* Invalid argument */
#define ENFILE 23  /* Too many open files in system */
#define EMFILE 24  /* Too many open files */
#define ENOTTY 25  /* Inappropriate I/O control operation */
/* 26 - Unknown Error */
#define EFBIG 27  /* File too large */
#define ENOSPC 28 /* No space left on device */
#define ESPIPE 29 /* Invalid seek (seek on a pipe?) */
#define EROFS 30  /* Read-only file system */
#define EMLINK 31 /* Too many links */
#define EPIPE 32  /* Broken pipe */
#define EDOM 33   /* Domain error (math functions) */
#define ERANGE 34 /* Result too large (possibly too small) */
/* 35 - Unknown Error */
#define EDEADLOCK 36 /* Resource deadlock avoided (non-Cyg) */
#define EDEADLK 36
/* 37 - Unknown Error */
#define ENAMETOOLONG 38 /* Filename too long (91 in Cyg?) */
#define ENOLCK 39       /* No locks available (46 in Cyg?) */
#define ENOSYS 40       /* Function not implemented (88 in Cyg?) */
#define ENOTEMPTY 41    /* Directory not empty (90 in Cyg?) */
#define EILSEQ 42       /* Illegal byte sequence */

#define ECONVERT 43 /* Convert error (FIXME-WINCE: Icouldn't find the correct value */

#define EWOULDBLOCK EAGAIN /* Operation would block */

/*
 * Because we need a per-thread errno, we define a function
 * pointer that we can call to return a pointer to the errno
 * for the current thread.  Then we define a macro for errno
 * that dereferences this function's result.
 *
 * This makes it syntactically just like the "real" errno.
 *
 * Using a function pointer allows us to use a very fast
 * function when there are no threads running and a slower
 * function when there are multiple threads running.
 */
#ifdef errno
#undef errno
#endif
void wince_errno_new_thread(int *errno_pointer);
void wince_errno_thread_exit(void);
WINCE_PyAPI_DATA(int) * (*wince_errno_pointer_function)(void);
#define errno (*(*wince_errno_pointer_function)())

#define _errno errno

extern int _sys_nerr;
extern const char *_sys_errlist[];

/* signal.h */
#define SIGINT 2    /* Interactive attention */
#define SIGILL 4    /* Illegal instruction */
#define SIGFPE 8    /* Floating point error */
#define SIGSEGV 11  /* Segmentation violation */
#define SIGTERM 15  /* Termination request */
#define SIGBREAK 21 /* Control-break */
#define SIGABRT 22  /* Abnormal termination (abort) */

#define NSIG 23 /* maximum signal number + 1 */

#define SetErrorMode(m) (0)
#define GetErrorMode() (0)

/* winbase.h */

/*
 * Windows CE doesn't have following things:
 *
 * we need to define these as mush as possible.
 */

unsigned long getVersion();

void *SecureZeroMemory(void *ptr, size_t cnt);

double copysign(double x, double y);

// Modules/_io/winconsoleio.c
#define C3_HIGHSURROGATE 0x0800
#define C3_LOWSURROGATE 0x1000

#define FILE_FLAG_FIRST_PIPE_INSTANCE 0x00080000
#define FILE_MAP_EXECUTE 0x00000020
#define PAGE_WRITECOMBINE 0x00000400
#define SEC_LARGE_PAGES 0x80000000
#define SEC_WRITECOMBINE 0x40000000

#define PROC_THREAD_ATTRIBUTE_HANDLE_LIST 0x00020002
#define CREATE_NEW_PROCESS_GROUP 0x00000200
#define CREATE_UNICODE_ENVIRONMENT 0x00000400
#define DETACHED_PROCESS 0x00000008
#define EXTENDED_STARTUPINFO_PRESENT 0x00080000

#define _SC_TTY_NAME_MAX 0x0000023F

#define VOLUME_NAME_DOS 0x00
#define VOLUME_NAME_GUID 0x01
#define VOLUME_NAME_NONE 0x04 /* Only this can be used? */
#define VOLUME_NAME_NT 0x02

#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS 0x00001000

typedef struct _PROC_THREAD_ATTRIBUTE_ENTRY {
    unsigned long *Attribute;
    size_t cbSize;
    void *lpValue;
} PROC_THREAD_ATTRIBUTE_ENTRY, *LPPROC_THREAD_ATTRIBUTE_ENTRY;

typedef struct _LPPROC_THREAD_ATTRIBUTE_LIST {
    unsigned long dwFlags;
    unsigned long Size;
    unsigned long Count;
    unsigned long Reserved;
    unsigned long *Unknown;
    PROC_THREAD_ATTRIBUTE_ENTRY Entries[1];
} PROC_THREAD_ATTRIBUTE_LIST, *LPPROC_THREAD_ATTRIBUTE_LIST;

typedef struct _STARTUPINFOEXW {
    STARTUPINFOW StartupInfo;
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList;
} STARTUPINFOEXW, *LPSTARTUPINFOEXW;

typedef struct _FILE_ATTRIBUTE_TAG_INFO {
    unsigned long FileAttributes;
    unsigned long ReparseTag;
} FILE_ATTRIBUTE_TAG_INFO, *PFILE_ATTRIBUTE_TAG_INFO;

int InitializeProcThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
                                      unsigned long dwAttributeCount, unsigned long dwFlags,
                                      size_t *lpSize);
int UpdateProcThreadAttribute(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList, unsigned long dwFlags,
                              unsigned long *Attribute, void *lpValue, size_t cbSize,
                              void *lpPreviousValue, size_t *lpReturnSize);
void DeleteProcThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList);

#ifndef wcsnlen /* FIXME-WINCE: CEGCC does not define wcsnlen. I wonder why. */
size_t wcsnlen(const wchar_t *str, size_t numberOfElements);
#endif
/* FIXME-WINCE: CEGCC does not define wcscat_s. I wonder why. */
int wcscat_s(wchar_t *strDestination, size_t numberOfElements, const wchar_t *strSource);

#define _chsize_s _chsize
#define wcscpy_s(dest, size, src) (wcscpy(dest, src))
#define wcsncpy_s(d, n, s, c) (wcsncpy(d, s, c))
#define wcsnlen_s(str, num) str == NULL ? 0 : wcsnlen(str, num)
#define wcsncasecmp _wcsnicmp
#define wcstok_s wcstok
#define memcpy_s(d, ds, s, c) (memcpy(d, s, c))

int _heapmin(void);

double copysign(double x, double y);

#define _LK_UNLCK 0  /* Unlock */
#define _LK_LOCK 1   /* Lock */
#define _LK_NBLCK 2  /* Non-blocking lock */
#define _LK_RLCK 3   /* Lock for read only */
#define _LK_NBRLCK 4 /* Non-blocking lock for read only */

int _locking(int fd, int mode, long nbytes);

#define GetHandleInformation(h, f) (0)
#define SetHandleInformation(h, m, f) (0)

#define RemoveVectoredExceptionHandler(h) (0)

#define WaitForSingleObjectEx(h, m, a) ((a && 0xffffffff || WaitForSingleObject(h, m)))

#define GetProcessTimes(h, c, e, k, u) (0)
#define OpenProcessToken(p, d, t) (0)
#define AdjustTokenPrivileges(t, d, n, b, p, r) (0)

#define GetSystemTimeAdjustment(t, i, d) (0)

#define GetFileInformationByHandleEx(h, c, i, s) (GetFileInformationByHandle(h, i))
#define OpenFileMapping(d, i, n) (NULL)

#define OpenFileMappingW OpenFileMapping

#define GetFinalPathNameByHandle(h, p, c, f) (0)
#define GetFinalPathNameByHandleW GetFinalPathNameByHandle
#define GetVolumePathName(f, b, c) (0)
#define GetVolumePathNameW GetVolumePathName

#define GetOverlappedResult(f, o, n, w) (0)

#define SecureZeroMemory ZeroMemory

#define RegDeleteKeyExW(k, s, sam, rsv) (RegDeleteKeyW(k, s))
#define RegLoadKeyW(k, s, f) (ERROR_NOT_SUPPORTED)
#define RegSaveKeyW(k, f, sec) (ERROR_NOT_SUPPORTED)
#define RegConnectRegistryW(m, k, r) (ERROR_NOT_SUPPORTED)
#define CancelIo(h) (0)

#define AdjustPrivileges(h, d, n, l, p, r) (0)

wchar_t **CommandLineToArgvW(const wchar_t *lpCmdLine, int *pNumArgs);

/* pathcch.h emulation */

#define PATHCCH_NONE 0
#define PATHCCH_ALLOW_LONG_PATHS 1

#define PATHCCH_E_FILENAME_TOO_LONG \
    ((HRESULT)0x8000FFFFL) /* FIXME-WINCE: could not find its actual value */

WINCE_PyAPI_FUNC(HRESULT) PathCchCanonicalizeEx(wchar_t *pszPathOut, size_t cchPathOut,
                                                wchar_t *pszPathIn, unsigned long dwFlags);
WINCE_PyAPI_FUNC(HRESULT)
    PathCchCombineEx(wchar_t *pszPathOut, size_t cchPathOut, wchar_t *pszPathIn, wchar_t *pszMore,
                     unsigned long dwFlags);
WINCE_PyAPI_FUNC(HRESULT) PathCchSkipRoot(wchar_t *pszPath, wchar_t **ppszRootEnd);

/* namedpipe emulation ( not emulation though ) */

#define ConnectNamedPipe(h, o) (0)
#define CreateNamedPipe(n, o, p, m, ob, ib, t, s) (INVALID_HANDLE_VALUE)
#define CreatePipe(r, w, a, s) (0)
#define PeekNamedPipe(h, buf, s, bytes, tm, l) (0)
#define WaitNamedPipe(l, t) (0)
#define SetNamedPipeHandleState(h, m, mc, t) (0)

/* Math */

/*
 * ldexp() DOES NOT return HUGE_VAL on overflow.  It void*);s 1.#INF,
 * so we need to define Py_HUGE_VAL in a way that allows us to correctly
 * detect overflow (this is tested by test.test_builtin)
 */
extern unsigned char wince_positive_double_infinity[];
#define Py_HUGE_VAL (*(double *)wince_positive_double_infinity)

/* Time functions */

#define _TM_DEFINED

struct tm {
    int tm_sec;   /* Seconds: 0-59 */
    int tm_min;   /* Minutes: 0-59 */
    int tm_hour;  /* Hours since midnight: 0-23 */
    int tm_mday;  /* Day of the month: 1-31 */
    int tm_mon;   /* Months *since* january: 0-11 */
    int tm_year;  /* Years since 1900 */
    int tm_wday;  /* Days since Sunday (0-6) */
    int tm_yday;  /* Days since Jan. 1: 0-365 */
    int tm_isdst; /* +1 Daylight Savings Time, 0 No DST,
                   * -1 don't know */
};
typedef long clock_t;

clock_t clock(void);

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);
char *asctime(const struct tm *tm);
char *ctime(const time_t *timep);
time_t mktime(struct tm *tm);
time_t time(time_t *TimeP);

extern char *tzname[2];
extern long timezone;
extern int daylight;

void tzset(void);

/* File & directory APIs */

#define FILE_TYPE_UNKNOWN 0
#define FILE_TYPE_DISK 1
#define FILE_TYPE_CHAR 2
#define FILE_TYPE_PIPE 3
#define FILE_TYPE_REMOTE 0x8000

#undef GetFileType

int wince_GetFileType(void *handle);

#define GetFileType wince_GetFileType

#define _O_RDWR (1 << 0)
#define _O_RDONLY (2 << 0)
#define _O_WRONLY (3 << 0)
#define _O_MODE_MASK (3 << 0)
#define _O_TRUNC (1 << 2)
#define _O_EXCL (1 << 3)
#define _O_CREAT (1 << 4)

#define O_RDWR _O_RDWR
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_MODE_MASK _O_MODE_MASK
#define O_TRUNC _O_TRUNC
#define O_EXCL _O_EXCL
#define O_CREAT _O_CREAT
#define O_BINARY _O_BINARY

#define fopen wince_fopen
WINCE_PyAPI_FUNC(FILE *) wince_fopen(const char *filename, const char *mode);
#define _wfopen wince_wfopen
WINCE_PyAPI_FUNC(FILE *) wince_wfopen(const wchar_t *filename, const wchar_t *mode);

int _wopen(const wchar_t *filename, int oflag, ...);
int _open(const char *filename, int oflag, ...);
FILE *_fdopen(int handle, const char *mode);
int _close(int handle);
int _write(int handle, const void *buffer, unsigned int count);
int _read(int handle, void *buffer, unsigned int count);
long _lseek(int handle, long offset, int origin);
__int64 _lseeki64(int handle, __int64 offset, int origin);
int _commit(int handle);
int _dup(int fd);
#define dup _dup
#define _open_osfhandle(h, m) (h)

#define open _open
#undef fdopen /* defined in Modules/zlib/zutil.h */
#define fdopen _fdopen
#define close _close
#define write _write
#define read _read
#define lseek _lseek

#define rewind(f) fseek((f), 0, SEEK_SET)

/* Windows CE has no CreateHard/SymbolicLink functions.
 * It seems true that no hard links in WinCE,
 * but special symbolic links exist. */

#define CreateHardLink CreateHardLinkW
#define CreateSymbolicLink CreateSymbolicLinkW

/* These STDIO internal constants are used by fileobject */
#define _IOFBF 0
#define _IONBF 4
#define _IOLBF 64

#define _get_osfhandle(fd) ((HANDLE)fd)

int _link(const char *from, const char *to);
#define _link(from, to) (-1)
#define link _link
int _unlink(const char *path);
#define unlink _unlink

#define _S_IFIFO 0x1000 /* FIFO */
#define _S_IFCHR 0x2000 /* Character */
#define _S_IFBLK 0x3000 /* Block */
#define _S_IFDIR 0x4000 /* Directory */
#define _S_IFREG 0x8000 /* Regular */

#define _S_IFMT 0xF000 /* File type mask */

#define _S_IEXEC 0x0040
#define _S_IWRITE 0x0080
#define _S_IREAD 0x0100

#define _S_IRWXU (_S_IREAD | _S_IWRITE | _S_IEXEC)
#define _S_IXUSR _S_IEXEC
#define _S_IWUSR _S_IWRITE
#define _S_IRUSR _S_IREAD

#define _S_ISDIR(m) (((m)&_S_IFMT) == _S_IFDIR)
#define _S_ISFIFO(m) (((m)&_S_IFMT) == _S_IFIFO)
#define _S_ISCHR(m) (((m)&_S_IFMT) == _S_IFCHR)
#define _S_ISBLK(m) (((m)&_S_IFMT) == _S_IFBLK)
#define _S_ISREG(m) (((m)&_S_IFMT) == _S_IFREG)

#define S_IFIFO _S_IFIFO
#define S_IFCHR _S_IFCHR
#define S_IFBLK _S_IFBLK
#define S_IFDIR _S_IFDIR
#define S_IFREG _S_IFREG
#define S_IFMT _S_IFMT
#define S_IEXEC _S_IEXEC
#define S_IWRITE _S_IWRITE
#define S_IREAD _S_IREAD
#define S_IRWXU _S_IRWXU
#define S_IXUSR _S_IXUSR
#define S_IWUSR _S_IWUSR
#define S_IRUSR _S_IRUSR

#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#define S_ISFIFO(m) (((m)&S_IFMT) == S_IFIFO)
#define S_ISCHR(m) (((m)&S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m)&S_IFMT) == S_IFBLK)
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)

// typedef unsigned int _dev_t;
typedef short _dev_t;
typedef short _ino_t;
typedef unsigned short _mode_t;
typedef long _off_t;

#undef stat
#undef fstat
#undef _stat
#undef _fstat

struct _stat {
    _dev_t st_dev;   /* Equivalent to drive number 0=A 1=B ... */
    _ino_t st_ino;   /* Always zero ? */
    _mode_t st_mode; /* See above constants */
    short st_nlink;  /* Number of links. */
    short st_uid;    /* User: Maybe significant on NT ? */
    short st_gid;    /* Group: Ditto */
    _dev_t st_rdev;  /* Seems useless (not even filled in) */
    _off_t st_size;  /* File size in bytes */
    time_t st_atime; /* Accessed date (always 00:00 hrs local
                      * on FAT) */
    time_t st_mtime; /* Modified time */
    time_t st_ctime; /* Creation time */
};

int _fstat(int handle, struct _stat *buffer);
int _stat(const char *path, struct _stat *buffer);
int _wstat(const wchar_t *path, struct _stat *buffer);

#define fstat _fstat
#define stat _stat

/* Current directory APIs */

char *_getcwd(char *, int);
#define getcwd _getcwd
wchar_t *_wgetcwd(wchar_t *buffer, int maxlen);

typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
#ifndef _PICKLE_COMPILING
typedef long LONG;
#endif

DWORD GetCurrentDirectoryA(DWORD numbuf, char *buffer);
DWORD GetCurrentDirectoryW(DWORD numbuf, wchar_t *buffer);
WINCE_PyAPI_FUNC(int) SetCurrentDirectoryW(const wchar_t *path);
int SetCurrentDirectoryA(const char *path);

DWORD GetFullPathNameA(const char *path, DWORD num_buf, char *buf, char **file_part);
DWORD GetFullPathNameW(const wchar_t *path, DWORD num_buf, wchar_t *buf, wchar_t **file_part);

/* String functions */

size_t strxfrm(char *dest, char *src, size_t n);

#define wcscoll(s1, s2) wcscmp(s1, s2)
#define strcoll(s1, s2) strcmp(s1, s2)
#define stricmp _stricmp

#define strnicmp _strnicmp

/* Locale functions */

#define _LCONV_DEFINED

struct lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
};

char *setlocale(int category, const char *locale);
struct lconv *localeconv(void);
int GetLocaleInfoA(DWORD lcid, DWORD lctype, char *buf, int buf_size);

// TODO: how to LCMapString -> LCMapStringEx? (LocaleId -> LocaleName)
#define LCMapStringEx(l, f, ss, cs, dc, cd, v, r, s) (0)

/* Miscellaneous APIs */

#define abort() _exit(3)

/*
 * Make the character classification function call different so we
 * can compensate for Windows CE misclassification of EOF
 */
WINCE_PyAPI_FUNC(int) wince_isctype(int ch, int classification);
#define _isctype wince_isctype

/*
 * These two functions are only used in sysmodule.c but will never
 * be called because isatty() always returns false
 */
#define GetConsoleCP() (0)
#define GetConsoleOutputCP() (0)

#define SetConsoleCtrlHandler(h, a) /* dummy */
#define GenerateConsoleCtrlEvent(e, g) (0)

#define _kbhit() WinCEShell_kbhit()

// used in winapi.c
#define GetStdHandle(s) (INVALID_HANDLE_VALUE)

/* Signals are not supported, so do nothing and succeed */
#define raise(num) (0)
#define signal(num, handler) (0)
#define SIG_ERR ((void *)(-1))
#define SIG_DFL ((void *)(0))
#define SIG_IGN ((void *)(1))

#define SIGINT 0

/* Environment variables are not supported */
WINCE_PyAPI_FUNC(DWORD) wince_GetEnvironmentVariable();
#define GetEnvironmentVariable(name, buf, size) wince_GetEnvironmentVariable()
#define GetEnvironmentVariableW GetEnvironmentVariable
/* When setting environment variables, do nothing and succeed */
#define SetEnvironmentVariableA(n, v) (TRUE)
#define SetEnvironmentVariableW(n, v) (TRUE)

#define ExpandEnvironmentStringsW(s, d, n) (0)

int _getpid(void);
#define getpid _getpid

typedef struct HINSTANCE__ *HINSTANCE;
typedef struct HWND__ *HWND;
typedef struct HKEY__ *HKEY;

HINSTANCE ShellExecuteA(HWND hwnd, const char *operation, const char *file, const char *params,
                        const char *dir, int show_cmd);
HINSTANCE ShellExecuteW(HWND hwnd, const wchar_t *operation, const wchar_t *file,
                        const wchar_t *params, const wchar_t *dir, int show_cmd);

typedef HINSTANCE HMODULE;

HMODULE LoadLibraryExA(const char *filename, void *reserved, DWORD flags);

DWORD FormatMessageA(DWORD flags, const void *source, DWORD msg, DWORD lang, char *buf,
                     DWORD buf_size, va_list *args);

void OutputDebugStringA(const char *message);

#ifndef _PICKLE_COMPILING
LONG RegQueryValueExA(HKEY hkey, const char *value_name, DWORD *reserved, DWORD *type, BYTE *data,
                      DWORD *num_data);
#else
long RegQueryValueExA(HKEY hkey, const char *value_name, DWORD *reserved, DWORD *type, BYTE *data,
                      DWORD *num_data);
#endif

#define RegCreateKey(hKey, lpSubKey, phkResult) \
    RegCreateKeyEx(hKey, lpSubKey, 0, NULL, 0, 0, NULL, phkResult, NULL)
#define RegQueryValueA(hKey, lpSubKey, lpValue, lpcbValue) \
    RegQueryValueExA(hKey, lpSubKey, NULL, NULL, lpValue, lpcbValue)
#define RegQueryValueW(hKey, lpSubKey, lpValue, lpcbValue) \
    RegQueryValueExW(hKey, lpSubKey, NULL, NULL, lpValue, lpcbValue)
#define RegQueryValue RegQueryValueW
#define RegSetValue(hKey, lpSubKey, dwType, lpData, cbData) \
    RegSetValueEx(hKey, lpSubKey, 0, dwType, lpData, cbData)

#define RegCreateKeyW RegCreateKey
#define RegSetValueW RegSetValue

typedef struct _WIN32_FIND_DATAA WIN32_FIND_DATAA;
typedef struct _WIN32_FIND_DATAW WIN32_FIND_DATAW;
typedef void *HANDLE;

HANDLE wince_FindFirstFileW(const wchar_t *filename, WIN32_FIND_DATAW *data);
#define FindFirstFileW wince_FindFirstFileW
HANDLE FindFirstFileA(const char *filename, WIN32_FIND_DATAA *data);

int FindNextFileA(HANDLE handle, WIN32_FIND_DATAA *data);

HANDLE wince_CreateFileW(const wchar_t *filename, DWORD dwDesiredAccess, DWORD dwShareMode,
                         struct _SECURITY_ATTRIBUTES *lpSecurityAttributes,
                         DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                         HANDLE hTemplateFile);
#define CreateFileW wince_CreateFileW
HANDLE CreateFileA(const char *filename, DWORD dwDesiredAccess, DWORD dwShareMode,
                   struct _SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

DWORD wince_GetFileAttributesW(const wchar_t *filename);
#define GetFileAttributesW wince_GetFileAttributesW
DWORD GetFileAttributesA(const char *filename);

BOOL wince_GetFileAttributesExW(const wchar_t *filename, enum _GET_FILEEX_INFO_LEVELS fInfoLevelId,
                                void *lpFileInformation);
#define GetFileAttributesExW wince_GetFileAttributesExW
BOOL GetFileAttributesExA(const char *filename, enum _GET_FILEEX_INFO_LEVELS fInfoLevelId,
                          void *lpFileInformation);

BOOL wince_SetFileAttributesW(const wchar_t *filename, DWORD attr);
#define SetFileAttributesW wince_SetFileAttributesW
BOOL SetFileAttributesA(const char *filename, DWORD attr);

BOOL wince_CreateDirectoryW(const wchar_t *path, struct _SECURITY_ATTRIBUTES *security);
#define CreateDirectoryW wince_CreateDirectoryW
BOOL CreateDirectoryA(const char *path, struct _SECURITY_ATTRIBUTES *security);

BOOL wince_RemoveDirectoryW(const wchar_t *path);
#define RemoveDirectoryW wince_RemoveDirectoryW
BOOL RemoveDirectoryA(const char *path);

BOOL wince_MoveFileW(const wchar_t *oldpath, const wchar_t *newpath);
#define MoveFileW wince_MoveFileW
BOOL MoveFileA(const char *oldpath, const char *newpath);

BOOL wince_DeleteFileW(const wchar_t *path);
#define DeleteFileW wince_DeleteFileW
BOOL DeleteFileA(const char *path);

/* CreateHardLink is not supported, and CreateSymbolicLink works differently */
#define CreateHardLinkW(fn, existfn, secattr) (0)
BOOL CreateSymbolicLinkW(wchar_t *lpSymlinkFileName, wchar_t *lpTargetFileName, DWORD dwFlags);
#define CreateHardLink CreateHardLinkW
#define CreateSymbolicLink CreateSymbolicLinkW

char *CharPrevA(const char *start, const char *current);

typedef struct _object PyObject;

DWORD GetActiveProcessorCount(WORD GroupNumber);

#define GetTickCount64() ((unsigned __int64)GetTickCount())

#define _umask(p) (-1)
#define umask _umask

#endif /* WINCE_COMPATIBILITY_H */
