/*
 * Provides functions that are available on Windows generally but
 * not on Windows CE.
 */

#include "wince_compatibility.h"

#include <limits.h>
#include <malloc.h>
#include <stdlib.h>
#include <windows.h>

/********************************************************/
/*							*/
/* Errno emulation:					*/
/*	There is no errno on Windows CE and we need	*/
/*	to make it per-thread.  So we have a function	*/
/*	that returns a pointer to the errno for the	*/
/*	current thread.					*/
/*							*/
/*	If there is ONLY the main thread then we can	*/
/* 	quickly return some static storage.		*/
/*							*/
/*	If we have multiple threads running, we use	*/
/*	Thread-Local Storage to hold the pointer	*/
/*							*/
/********************************************************/

/* _sys_nerr and _sys_errlist */
static char undefined_error[] = "Undefined error";

const char *_sys_errlist[] = {
    undefined_error,                       /*  0 - ENOERROR */
    undefined_error,                       /*  1 - EPERM */
    "No such file or directory",           /*  2 - ENOENT */
    "No such process",                     /*  3 - ESRCH */
    "Interrupted system call",             /*  4 - EINTR */
    undefined_error,                       /*  5 - EIO */
    "Device not configured",               /*  6 - ENXIO */
    undefined_error,                       /*  7 - E2BIG */
    undefined_error,                       /*  8 - ENOEXEC */
    undefined_error,                       /*  9 - EBADF */
    undefined_error,                       /* 10 - ECHILD */
    undefined_error,                       /* 11 - EDEADLK */
    undefined_error,                       /* 12 - ENOMEM */
    "Permission denied",                   /* 13 - EACCES */
    "Bad access",                          /* 14 - EFAULT */
    undefined_error,                       /* 15 - ENOTBLK */
    "File is busy",                        /* 16 - EBUSY */
    "File exists",                         /* 17 - EEXIST */
    undefined_error,                       /* 18 - EXDEV */
    undefined_error,                       /* 19 - ENODEV */
    "Is not a directory",                  /* 20 - ENOTDIR */
    "Is a directory",                      /* 21 - EISDIR */
    "Invalid argument",                    /* 22 - EINVAL */
    "Too many open files in system",       /* 23 - ENFILE */
    "Too many open files",                 /* 24 - EMFILE */
    "Inappropriate I/O control operation", /* 25 - NOTTY */
    undefined_error,                       /* 26 - unknown */
    "File too large",                      /* 27 - EFBIG */
    "No space left on device",             /* 28 - ENOSPC */
    "Invalid seek",                        /* 29 - ESPIPE */
    "Read-only file systems",              /* 30 - EROFS */
    "Too many links",                      /* 31 - EMLINK */
    "Broken pipe",                         /* 32 - EPIPE */
    "Domain error",                        /* 33 - EDOM */
    "Result too large",                    /* 34 - ERANGE */
    undefined_error,                       /* 35 - unknown */
    "Resource deadlock avoided",           /* 36 - EDEADLK */
    undefined_error,                       /* 37 - unknown */
    "File name too long",                  /* 38 - ENAMETOOLONG */
    "No locks available",                  /* 39 - ENOLCK */
    "Functions not implemented",           /* 40 - ENOSYS */
    "Directory not empty",                 /* 41 - ENOTEMPTY */
    "Illegal byte sequence",               /* 42 - EILSEQ */
    "Convert failed"                       /* 43 - ECONVERT */
};

/* This is set to zero for now because it is only used in Python/errors.c,
   and setting it to zero will cause Python to call FormatMessage instead */
int _sys_nerr = 0; /*sizeof(_sys_errlist)/sizeof(_sys_errlist[0]);*/

/* Function pointer for returning errno pointer */
static int *initialize_errno(void);
int *(*wince_errno_pointer_function)(void) = initialize_errno;

/* Static errno storage for the main thread */
static int errno_storage = 0;

/* Thread-Local storage slot for errno */
static int tls_errno_slot = 0xffffffff;

/*
 * Number of threads we have running and critical section protection
 * for manipulating it
 */
static int number_of_threads = 0;
static CRITICAL_SECTION number_of_threads_critical_section;

/* For the main thread only -- return the errno pointer */
static int *
get_main_thread_errno(void)
{
    return &errno_storage;
}

/* When there is more than one thread -- return the errno pointer */
static int *
get_thread_errno(void)
{
    return (int *)TlsGetValue(tls_errno_slot);
}

/* Initialize a thread's errno */
static void
initialize_thread_errno(int *errno_pointer)
{
    /* Make sure we have a slot */
    if (tls_errno_slot == 0xffffffff) {
        /* No: Get one */
        tls_errno_slot = (int)TlsAlloc();
        if (tls_errno_slot == 0xffffffff)
            abort();
    }
    /*
     * We can safely check for 0 threads, because
     * only the main thread will be initializing
     * at this point.  Make sure the critical
     * section that protects the number of threads
     * is initialized
     */
    if (number_of_threads == 0)
        InitializeCriticalSection(&number_of_threads_critical_section);
    /* Store the errno pointer */
    if (TlsSetValue(tls_errno_slot, (LPVOID)errno_pointer) == 0)
        abort();
    /* Bump the number of threads */
    EnterCriticalSection(&number_of_threads_critical_section);
    number_of_threads++;
    if (number_of_threads > 1) {
        /*
         * We have threads other than the main thread:
         *   Use thread-local storage
         */
        wince_errno_pointer_function = get_thread_errno;
    }
    LeaveCriticalSection(&number_of_threads_critical_section);
}

/* Initialize errno emulation on Windows CE (Main thread) */
static int *
initialize_errno(void)
{
    /* Initialize the main thread's errno in thread-local storage */
    initialize_thread_errno(&errno_storage);
    /*
     * Set the errno function to be the one that returns the
     * main thread's errno
     */
    wince_errno_pointer_function = get_main_thread_errno;
    /*
     *	Return the main thread's errno
     */
    return &errno_storage;
}

/* Initialize errno emulation on Windows CE (New thread) */
void
wince_errno_new_thread(int *errno_pointer)
{
    initialize_thread_errno(errno_pointer);
}

/* Note that a thread has exited */
void
wince_errno_thread_exit(void)
{
    /* Decrease the number of threads */
    EnterCriticalSection(&number_of_threads_critical_section);
    number_of_threads--;
    if (number_of_threads <= 1) {
        /* We only have the main thread */
        wince_errno_pointer_function = get_main_thread_errno;
    }
    LeaveCriticalSection(&number_of_threads_critical_section);
}

char *
strerror(int errnum)
{
    if (errnum >= sizeof(_sys_errlist)) {
        errno = ECONVERT;
        errnum = errno;
    }
    return _sys_errlist[errnum];
}

#ifdef _M_ARM
/*
 * ldexp() DOES NOT return HUGE_VAL on overflow.  It returns 1.#INF,
 * which, on StrongARM are the following bytes:
 */
unsigned char wince_positive_double_infinity[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf0, 0x7f};
#else /* not _M_ARM */
/* For other architectures this needs to be defined */
#error Need to define the bytes for + infinity for a double on non-arm architectures
#endif /* not _M_ARM */

/*** System functions Emulation? ***/
size_t
wcsnlen(const wchar_t *str, size_t numberOfElements)
{
    int i = 0;
    int result = 0;
    while (i < numberOfElements) {
        if (*(str + i) == L'\0') {
            result = 1;
            break;
        }
        i++;
    }
    if (!result) {
        return numberOfElements;
    }
    else {
        return (size_t)i;
    }
}

int
wcscat_s(wchar_t *strDestination, size_t numberOfElements, const wchar_t *strSource)
{
    if (strDestination == NULL || wcsnlen(strDestination, numberOfElements) == numberOfElements)
        return EINVAL;
    if (strSource == NULL) {
        strDestination[0] = '\0';
        return EINVAL;
    }
    if (numberOfElements <= 0) {
        strDestination[0] = '\0';
        return ERANGE;
    }
    wcscat(strDestination, strSource);
    return 0;
}

DWORD
GetActiveProcessorCount(WORD GroupNumber)
{
    LPSYSTEM_INFO sinfo;
    GetSystemInfo(sinfo);
    return sinfo->dwNumberOfProcessors;
}

/*** Current Directory Emulation ***/

#define ISSLASH(c) ((c) == L'\\' || (c) == L'/')

/*
 * The emulated current working directory
 */
static WCHAR default_current_directory[] = L"\\Temp";
static WCHAR *current_directory = default_current_directory;

/*
 * WinCE has more restrictive path handling than Windows NT so we
 * need to do some conversions for compatibility
 */
static void
wince_canonicalize_path(WCHAR *path)
{
    WCHAR *p = path;

    /* Replace forward slashes with backslashes */
    while (*p) {
        if (*p == L'/')
            *p = L'\\';
        p++;
    }
    /* Strip \. and \.. from the beginning of absolute paths */
    p = path;
    while (p[0] == L'\\' && p[1] == L'.') {
        if (p[2] == L'.') {
            /* Skip \.. */
            p += 3;
        }
        else {
            /* Skip \. */
            p += 2;
        }
    }
    if (!*p) {
        /* If we stripped everything then return the root \
         * instead of an empty path
         */
        wcscpy(path, L"\\");
    }
    else if (p != path) {
        /* We skipped something so we need to delete it */
        int size = (wcslen(p) + 1) * sizeof(WCHAR);
        memmove(path, p, size);
    }
}

/* Converts a wide relative path to a wide absolute path */
void
wince_absolute_path_wide(const WCHAR *path, WCHAR *buffer, int buffer_size)
{
    int path_length, directory_length;
    WCHAR *cp;
    WCHAR *cp1;

    /* Check for a path that is already absolute */
    if (ISSLASH(*path)) {
        /* Yes: Just return it */
        path_length = wcslen(path);
        if (path_length >= buffer_size)
            path_length = buffer_size - 1;
        memcpy(buffer, path, path_length * sizeof(WCHAR));
        buffer[path_length] = L'\0';
        wince_canonicalize_path(buffer);
        return;
    }

    /* Need to turn it into an absolute path: strip "." and ".." */
    cp = (WCHAR *)path;
    /* cp1 points to the null terminator */
    cp1 = current_directory;
    while (*cp1) cp1++;
    while (*cp == L'.') {
        if (ISSLASH(cp[1])) {
            /* Strip ".\\" from beginning */
            cp += 2;
            continue;
        }
        if (cp[1] == L'\0') {
            /* "." is the same as no path */
            cp++;
            break;
        }
        /* Handle filenames like ".abc" */
        if (cp[1] != L'.')
            break;
        /* Handle filenames like "..abc" */
        if (!ISSLASH(cp[2]) && (cp[2] != L'\0'))
            break;
        /* Skip ".." */
        cp += 2;
        /* Skip backslash following ".." */
        if (*cp)
            cp++;
        /* Find the final backslash in the current directory path */
        while (cp1 > current_directory)
            if (ISSLASH(*--cp1))
                break;
    }

    /*
     * Now look for device specifications (and get the length of the path)
     */
    path = cp;
    while (*cp) {
        if (*cp++ == L':') {
            /* Device: Just return it */
            path_length = wcslen(path);
            if (path_length >= buffer_size)
                path_length = buffer_size - 1;
            memcpy(buffer, path, path_length * sizeof(WCHAR));
            buffer[path_length] = L'\0';
            wince_canonicalize_path(buffer);
            return;
        }
    }
    path_length = cp - path;
    /* Trim off trailing backslash */
    if ((path_length > 0) && (ISSLASH(cp[-1])))
        path_length--;
    /* If we backed up past the root, we are at the root */
    directory_length = cp1 - current_directory;
    if (directory_length == 0)
        directory_length = 1;
    /*
     * Output the directory and the path separator (truncated as necessary)
     */
    buffer_size -= 2; /* Account for the null terminator and the path separator */
    if (directory_length >= buffer_size)
        directory_length = buffer_size;
    memcpy(buffer, current_directory, directory_length * sizeof(WCHAR));
    buffer[directory_length] = L'\\';
    buffer_size -= directory_length;
    /* If there is no path, make sure we remove the path separator */
    if (path_length == 0)
        directory_length--;
    /* Output the path (truncated as necessary) */
    if (path_length >= buffer_size)
        path_length = buffer_size;
    memcpy(buffer + directory_length + 1, path, path_length * sizeof(WCHAR));
    buffer[directory_length + 1 + path_length] = '\0';
    wince_canonicalize_path(buffer);
}

/* Converts an ANSI relative path to a wide absolute path */
void
wince_absolute_path_to_wide(const char *path, WCHAR *buffer, int buffer_size)
{
    WCHAR wide_path[MAX_PATH + 1];

    /* Convert path to wide */
    MultiByteToWideChar(CP_ACP, 0, path, -1, wide_path, (sizeof(wide_path) / sizeof(wide_path[0])));
    /* Get the absolute path */
    wince_absolute_path_wide(wide_path, buffer, buffer_size);
}

char *
_getcwd(char *buffer, int maxlen)
{
    int n = WideCharToMultiByte(CP_ACP, 0, current_directory, -1, buffer, maxlen - 1, NULL, NULL);
    buffer[n] = '\0';
    return buffer;
}

wchar_t *
_wgetcwd(wchar_t *buffer, int maxlen)
{
    int n = wcslen(current_directory);
    if (n >= maxlen)
        n = maxlen - 1;
    memcpy(buffer, current_directory, n * sizeof(wchar_t));
    buffer[n] = L'\0';
    return buffer;
}

DWORD
GetCurrentDirectoryA(DWORD numbuf, char *buffer)
{
    int n = WideCharToMultiByte(CP_ACP, 0, current_directory, -1, buffer, numbuf, NULL, NULL);
    return n;
}

DWORD
GetCurrentDirectoryW(DWORD numbuf, wchar_t *buffer)
{
    DWORD n = wcslen(current_directory) + 1;
    if (n > numbuf)
        return n;
    memcpy(buffer, current_directory, n * sizeof(wchar_t));
    return n - 1;
}

BOOL
SetCurrentDirectoryW(const WCHAR *path)
{
    int length;
    WCHAR *cp;
    WCHAR absolute_path[MAX_PATH + 1];
    DWORD attr;

    /* Get the absolute path */
    wince_absolute_path_wide(path, absolute_path, MAX_PATH + 1);
    /* Make sure it exists */
    attr = GetFileAttributesW(absolute_path);
    if (attr == 0xFFFFFFFF)
        return FALSE;
    if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* Put it into dynamic memory */
    length = wcslen(absolute_path);
    cp = (WCHAR *)LocalAlloc(0, (length + 1) * sizeof(WCHAR));
    if (!cp)
        return FALSE;
    memcpy(cp, absolute_path, length * sizeof(WCHAR));
    cp[length] = L'\0';
    /* Free up any old allocation and store the new current directory */
    if (current_directory != default_current_directory)
        LocalFree(current_directory);
    current_directory = cp;
    return TRUE;
}

BOOL
SetCurrentDirectoryA(const char *path)
{
    WCHAR wide_path[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wide_path, (sizeof(wide_path) / sizeof(wide_path[0])));
    return SetCurrentDirectoryW(wide_path);
}

DWORD
GetFullPathNameA(const char *path, DWORD num_buf, char *buf, char **file_part)
{
    WCHAR wbuf[MAX_PATH + 1];
    DWORD n;

    wince_absolute_path_to_wide(path, wbuf, MAX_PATH);
    n = WideCharToMultiByte(CP_ACP, 0, wbuf, -1, buf, num_buf, NULL, NULL);
    if (file_part) {
        *file_part = strrchr(buf, '\\');
        if (*file_part)
            (*file_part)++;
        else
            *file_part = buf;
    }
    return n;
}

DWORD
GetFullPathNameW(const wchar_t *path, DWORD num_buf, wchar_t *buf, wchar_t **file_part)
{
    wince_absolute_path_wide(path, buf, num_buf);
    if (file_part) {
        *file_part = wcsrchr(buf, '\\');
        if (*file_part)
            (*file_part)++;
        else
            *file_part = buf;
    }
    return wcslen(buf);
}

#define FT_EPOCH (116444736000000000LL)
#define FT_TICKS (10000000LL)

/* Convert a Windows FILETIME to a time_t */
static time_t
convert_FILETIME_to_time_t(FILETIME *ft)
{
    __int64 temp;

    /*
     * Convert the FILETIME structure to 100nSecs since 1601 (as a 64-bit value)
     */
    temp = (((__int64)ft->dwHighDateTime) << 32) + (__int64)ft->dwLowDateTime;
    /* Convert to seconds from 1970 */
    return ((time_t)((temp - FT_EPOCH) / FT_TICKS));
}

static void
convert_time_t_to_FILETIME(time_t t, FILETIME *ft)
{
    __int64 temp;

    /*
     *	Use 64-bit calculation to convert seconds since 1970 to
     *	100nSecs since 1601
     */
    temp = ((__int64)t * FT_TICKS) + FT_EPOCH;
    /*
     *	Put it into the FILETIME structure
     */
    ft->dwLowDateTime = (DWORD)temp;
    ft->dwHighDateTime = (DWORD)(temp >> 32);
}

static int
Zeller(int d, int m, int y)
{
    if (m <= 2) {
        y--;
        m += 12;
    }
    return (d + (m * 2) + (int)((m + 1) * 3.0 / 5.0) + y + y / 4 - y / 100 + y / 400 + 1) % 7 + 1;
}

static int
GetSundayOfMonth(const SYSTEMTIME *t, int nYear)
{
    if (t->wDay == 5) {
        // Warning does not handle leap years
        int Months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        return Months[t->wMonth - 1] - (Zeller(Months[t->wMonth - 1], t->wMonth, nYear) - 1);
    }
    else {
        return (9 - Zeller(1, t->wMonth, nYear)) % 9 + (7 * (t->wDay - 1));
    }
}

static short
IsDST(const SYSTEMTIME *systime)
{
    TIME_ZONE_INFORMATION tzinfo;
    SYSTEMTIME stDaylightWeekInMonth;
    SYSTEMTIME stStandardWeekInMonth;
    int nDaylightSunday;
    int nStandardSunday;
    SYSTEMTIME stDaylightAbsolute;
    FILETIME ftDaylight;
    SYSTEMTIME stStandardAbsolute;
    FILETIME ftStandard;
    FILETIME ftTime;

    GetTimeZoneInformation(&tzinfo);
    stDaylightWeekInMonth = tzinfo.DaylightDate;
    stStandardWeekInMonth = tzinfo.StandardDate;

    nDaylightSunday = GetSundayOfMonth(&stDaylightWeekInMonth, systime->wYear);
    nStandardSunday = GetSundayOfMonth(&stStandardWeekInMonth, systime->wYear);

    stDaylightAbsolute.wYear = systime->wYear;
    stDaylightAbsolute.wMonth = stDaylightWeekInMonth.wMonth;
    stDaylightAbsolute.wDay = nDaylightSunday;
    stDaylightAbsolute.wHour = stDaylightWeekInMonth.wHour;
    stDaylightAbsolute.wMinute = stDaylightWeekInMonth.wMinute;
    stDaylightAbsolute.wSecond = stDaylightWeekInMonth.wSecond;
    stDaylightAbsolute.wMilliseconds = 0;
    SystemTimeToFileTime(&stDaylightAbsolute, &ftDaylight);

    stStandardAbsolute.wYear = systime->wYear;
    stStandardAbsolute.wMonth = stStandardWeekInMonth.wMonth;
    stStandardAbsolute.wDay = nStandardSunday;
    stStandardAbsolute.wHour = stStandardWeekInMonth.wHour;
    stStandardAbsolute.wMinute = stStandardWeekInMonth.wMinute;
    stStandardAbsolute.wSecond = stStandardWeekInMonth.wSecond;
    stStandardAbsolute.wMilliseconds = 0;
    SystemTimeToFileTime(&stStandardAbsolute, &ftStandard);

    SystemTimeToFileTime(systime, &ftTime);

    return ((CompareFileTime(&ftTime, &ftDaylight) == 1) &&
            (CompareFileTime(&ftTime, &ftStandard) != 1));
}

static struct tm *
convert_FILETIME_to_tm(FILETIME *ft)
{
    SYSTEMTIME systime;
    static struct tm tm = {0};
    static const short day_of_year_by_month[12] = {
        (short)(0),
        (short)(31),
        (short)(31 + 28),
        (short)(31 + 28 + 31),
        (short)(31 + 28 + 31 + 30),
        (short)(31 + 28 + 31 + 30 + 31),
        (short)(31 + 28 + 31 + 30 + 31 + 30),
        (short)(31 + 28 + 31 + 30 + 31 + 30 + 31),
        (short)(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31),
        (short)(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
        (short)(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
        (short)(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)};

    /* Turn the FILETIME into a SYSTEMTIME */
    FileTimeToSystemTime(ft, &systime);
    /* Use SYSTEMTIME to fill in the tm structure */
    tm.tm_sec = systime.wSecond;
    tm.tm_min = systime.wMinute;
    tm.tm_hour = systime.wHour;
    tm.tm_mday = systime.wDay;
    tm.tm_mon = systime.wMonth - 1;
    tm.tm_year = systime.wYear - 1900;
    tm.tm_wday = systime.wDayOfWeek;
    tm.tm_yday = day_of_year_by_month[tm.tm_mon] + tm.tm_mday - 1;
    tm.tm_isdst = IsDST(&systime);
    if (tm.tm_mon >= 2) {
        /*
         *	Check for leap year (every 4 years but not every 100 years but every 400 years)
         */
        if ((systime.wYear % 4) == 0) {
            /* It is a 4th year */
            if ((systime.wYear % 100) == 0) {
                /* It is a 100th year */
                if ((systime.wYear % 400) == 0) {
                    /* It is a 400th year: It is a leap year */
                    tm.tm_yday++;
                }
            }
            else {
                /* It is not a 100th year: It is a leap year */
                tm.tm_yday++;
            }
        }
    }
    return &tm;
}

static void
convert_tm_to_FILETIME(struct tm *tm, FILETIME *ft)
{
    SYSTEMTIME systime;

    /* Use the tm structure to fill in a SYSTEM */
    systime.wYear = tm->tm_year + 1900;
    systime.wMonth = tm->tm_mon + 1;
    systime.wDayOfWeek = tm->tm_wday;
    systime.wDay = tm->tm_mday;
    systime.wHour = tm->tm_hour;
    systime.wMinute = tm->tm_min;
    systime.wSecond = tm->tm_sec;
    systime.wMilliseconds = 0;
    /* Convert it to a FILETIME and return it */
    SystemTimeToFileTime(&systime, ft);
}

clock_t
clock(void)
{
    /* Not supported */
    return (clock_t)-1;
}

struct tm *
gmtime(const time_t *timep)
{
    FILETIME ft;

    if (!timep)
        return NULL;
    /* time_t -> FILETIME -> tm */
    convert_time_t_to_FILETIME(*timep, &ft);
    return convert_FILETIME_to_tm(&ft);
}

int
gmtime_s(struct tm *tmDest, const time_t *timep)
{
    struct tm *result;
    result = gmtime(timep);
    if (result == NULL)
        return 1;
    *tmDest = *result;
    return 0;
}

struct tm *
localtime(const time_t *timep)
{
    FILETIME ft, local_ft;

    if (!timep)
        return NULL;
    /* time_t -> FILETIME -> Local FILETIME -> tm */
    convert_time_t_to_FILETIME(*timep, &ft);
    FileTimeToLocalFileTime(&ft, &local_ft);
    return convert_FILETIME_to_tm(&local_ft);
}

int
localtime_s(struct tm *tmDest, const time_t *timep)
{
    struct tm *result;
    result = localtime(timep);
    if (result == NULL)
        return 1;
    *tmDest = *result;
    return 0;
}

char *
asctime(const struct tm *tm)
{
    WCHAR temp[26];
    static const char *Days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static char buffer[26];

    /* Format the string */
    wsprintfW(temp, TEXT("%hs %hs %2d %02d:%02d:%02d %4d\n"), Days[tm->tm_wday], Months[tm->tm_mon],
              tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year + 1900);
    /* Convert to ascii and return it */
    WideCharToMultiByte(CP_ACP, 0, temp, sizeof(temp) / sizeof(temp[0]), buffer,
                        sizeof(buffer) / sizeof(buffer[0]), NULL, NULL);
    return buffer;
}

char *
ctime(const time_t *timep)
{
    FILETIME ft;

    /*
     *	time_t -> FILETIME -> Local FILETIME -> tm then we can use asctime()
     */
    convert_time_t_to_FILETIME(*timep, &ft);
    FileTimeToLocalFileTime(&ft, &ft);
    return asctime(convert_FILETIME_to_tm(&ft));
}

time_t
mktime(struct tm *tm)
{
    FILETIME local_ft;
    FILETIME ft;

    /* tm -> Local FILETIME -> FILETIME -> time_t */
    convert_tm_to_FILETIME(tm, &local_ft);
    LocalFileTimeToFileTime(&local_ft, &ft);
    return convert_FILETIME_to_time_t(&ft);
}

time_t
time(time_t *timep)
{
    SYSTEMTIME systime;
    FILETIME ft;
    time_t result;

    /* Get the current system time */
    GetSystemTime(&systime);
    /* SYSTEMTIME -> FILETIME -> time_t */
    SystemTimeToFileTime(&systime, &ft);
    result = convert_FILETIME_to_time_t(&ft);
    /* Return the time_t */
    if (timep)
        *timep = result;
    return result;
}

void
GetSystemTimeAsFileTime(FILETIME *lpSystemTimeAsFileTime)
{
    SYSTEMTIME systime;

    GetSystemTime(&systime);

    SystemTimeToFileTime(&systime, lpSystemTimeAsFileTime);
}

static char standard_name[32] = "GMT";
static char daylight_name[32] = "GMT";
char *tzname[2] = {standard_name, daylight_name};
long timezone = 0;
int daylight = 0;

void
tzset(void)
{
    TIME_ZONE_INFORMATION info;
    int result;

    /*
     *	Get our current timezone information
     */
    result = GetTimeZoneInformation(&info);
    switch (result) {
        /*
         *	We are on standard time
         */
        case TIME_ZONE_ID_STANDARD:
            daylight = 0;
            break;
        /*
         *	We are on daylight savings time
         */
        case TIME_ZONE_ID_DAYLIGHT:
            daylight = 1;
            break;
        /*
         *	We don't know the timezone information (leave it GMT)
         */
        default:
            return;
    }
    /*
     *	Extract the timezone information
     */
    timezone = info.Bias * 60;
    if (info.StandardName[0])
        WideCharToMultiByte(CP_ACP, 0, info.StandardName, -1, standard_name,
                            sizeof(standard_name) - 1, NULL, NULL);
    if (info.DaylightName[0])
        WideCharToMultiByte(CP_ACP, 0, info.DaylightName, -1, daylight_name,
                            sizeof(daylight_name) - 1, NULL, NULL);
}

int
wince_GetFileType(HANDLE handle)
{
    SetLastError(0);
    return FILE_TYPE_DISK;
}

int
_wstat(const wchar_t *path, struct _stat *buffer)
{
    WIN32_FIND_DATA data;
    HANDLE hFile;
    WCHAR *p;
    unsigned attr;
    WCHAR absolute_path[MAX_PATH + 1];

    /* Get the absolute path */
    wince_absolute_path_wide(path, absolute_path, MAX_PATH + 1);

    /* Get the file attributes */
    attr = GetFileAttributesW(absolute_path);
    if (attr == 0xFFFFFFFF) {
        errno = GetLastError();
        return -1;
    }

    /* Check for stuff we can't deal with */
    p = absolute_path;
    while (*p) {
        if ((*p == L'?') || (*p == L'*'))
            return -1;
        p++;
    }
    hFile = FindFirstFile(absolute_path, &data);
    if (hFile == INVALID_HANDLE_VALUE) {
        errno = GetLastError();
        return -1;
    }
    FindClose(hFile);

    /* Found: Convert the file times */
    buffer->st_mtime = convert_FILETIME_to_time_t(&data.ftLastWriteTime);
    if (data.ftLastAccessTime.dwLowDateTime || data.ftLastAccessTime.dwHighDateTime)
        buffer->st_atime = convert_FILETIME_to_time_t(&data.ftLastAccessTime);
    else
        buffer->st_atime = buffer->st_mtime;
    if (data.ftCreationTime.dwLowDateTime || data.ftCreationTime.dwHighDateTime)
        buffer->st_ctime = convert_FILETIME_to_time_t(&data.ftCreationTime);
    else
        buffer->st_ctime = buffer->st_mtime;

    /* Convert the file attributes */
    if (absolute_path[1] == L':')
        p += 2;
    buffer->st_mode = (unsigned short)(((ISSLASH(*p) && !absolute_path[1]) ||
                                        (attr & FILE_ATTRIBUTE_DIRECTORY) || *p)
                                           ? S_IFDIR | S_IEXEC
                                           : S_IFREG);
    buffer->st_mode |= (attr & FILE_ATTRIBUTE_READONLY) ? S_IREAD : (S_IREAD | S_IWRITE);
    p = absolute_path + wcslen(absolute_path);
    while (p >= absolute_path) {
        if (*--p == L'.') {
            if (p[1] && ((p[1] == L'e') || (p[1] == L'c') || (p[1] == L'b')) && p[2] &&
                ((p[2] == L'x') || (p[2] == L'm') || (p[2] == L'a') || (p[2] == L'o')) && p[3] &&
                ((p[3] == L'e') || (p[3] == L'd') || (p[3] == L't') || (p[3] == L'm')))
                buffer->st_mode |= S_IEXEC;
            break;
        }
    }
    buffer->st_mode |= (buffer->st_mode & 0700) >> 3;
    buffer->st_mode |= (buffer->st_mode & 0700) >> 6;
    /* Set the other information */
    buffer->st_nlink = 1;
    buffer->st_size = (unsigned long int)data.nFileSizeLow;
    buffer->st_uid = 0;
    buffer->st_gid = 0;
    buffer->st_ino = 0;
    buffer->st_dev = 0;
    /* Return success */
    return 0;
}

/*static int Check_For_ZIP_Resource(const char *path, HRSRC *res);*/

int
wince_stat(const char *path, struct stat *st)
{
    WCHAR wide_path[MAX_PATH + 1];

    /* Turn it into a wide string and do the _wstat */
    MultiByteToWideChar(CP_ACP, 0, path, -1, wide_path, sizeof(wide_path) / sizeof(wide_path[0]));
    return _wstat(wide_path, st);
}

#define stat wince_stat

int
wince__fstat(int handle, struct _stat *buffer)
{
    BY_HANDLE_FILE_INFORMATION data;

    /* Get the file information */
    if (!GetFileInformationByHandle((HANDLE)handle, &data)) {
        /* Return error */
        errno = GetLastError();
        return -1;
    }

    /* Found: Convert the file times */
    buffer->st_mtime = convert_FILETIME_to_time_t(&data.ftLastWriteTime);
    if (data.ftLastAccessTime.dwLowDateTime || data.ftLastAccessTime.dwHighDateTime)
        buffer->st_atime = convert_FILETIME_to_time_t(&data.ftLastAccessTime);
    else
        buffer->st_atime = buffer->st_mtime;
    if (data.ftCreationTime.dwLowDateTime || data.ftCreationTime.dwHighDateTime)
        buffer->st_ctime = convert_FILETIME_to_time_t(&data.ftCreationTime);
    else
        buffer->st_ctime = buffer->st_mtime;

    /* Convert the file attributes */
    buffer->st_mode =
        (unsigned short)((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR | S_IEXEC
                                                                            : S_IFREG);
    buffer->st_mode |=
        (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? S_IREAD : (S_IREAD | S_IWRITE);
    buffer->st_mode |= (buffer->st_mode & 0700) >> 3;
    buffer->st_mode |= (buffer->st_mode & 0700) >> 6;
    /* Set the other information */
    buffer->st_nlink = 1;
    buffer->st_size = (unsigned long int)data.nFileSizeLow;
    buffer->st_uid = 0;
    buffer->st_gid = 0;
    buffer->st_ino = 0;
    buffer->st_dev = 0;
    /* Return success */
    return 0;
}

#define _fstat wince__fstat

static find_data_wide_to_ansi(WIN32_FIND_DATAA *data, const WIN32_FIND_DATAW *wdata)
{
    data->dwFileAttributes = wdata->dwFileAttributes;
    data->ftCreationTime = wdata->ftCreationTime;
    data->ftLastAccessTime = wdata->ftLastAccessTime;
    data->ftLastWriteTime = wdata->ftLastWriteTime;
    data->nFileSizeHigh = wdata->nFileSizeHigh;
    data->nFileSizeLow = wdata->nFileSizeLow;
    data->dwOID = wdata->dwOID;
    // data->dwReserved0 = 0;
    // data->dwReserved1 = 0;
    WideCharToMultiByte(CP_ACP, 0, wdata->cFileName, -1, data->cFileName, sizeof(data->cFileName),
                        NULL, NULL);
    // data->cAlternateFileName[0] = '\0';
}

int
_wopen(const wchar_t *filename, int oflag, ...)
{
    DWORD access, share_mode, creation_disposition;
    HANDLE handle;
    static int modes[4] = {0, (GENERIC_READ | GENERIC_WRITE), GENERIC_READ, GENERIC_WRITE};
    WCHAR absolute_path[MAX_PATH + 1];

    /* Get the absolute path */
    wince_absolute_path_wide(filename, absolute_path, MAX_PATH + 1);

    /* Calculate the CreateFile arguments */
    access = modes[oflag & _O_MODE_MASK];
    share_mode = (oflag & _O_EXCL) ? 0 : (FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (oflag & _O_TRUNC)
        creation_disposition = (oflag & _O_CREAT) ? CREATE_ALWAYS : TRUNCATE_EXISTING;
    else {
        if (oflag & _O_APPEND)
            creation_disposition = (oflag & _O_CREAT) ? OPEN_ALWAYS : OPEN_EXISTING;
        else
            creation_disposition = (oflag & _O_CREAT) ? CREATE_NEW : OPEN_EXISTING;
    }

    handle = CreateFileW(absolute_path, access, share_mode, NULL, creation_disposition,
                         FILE_ATTRIBUTE_NORMAL, NULL);

    /* Deal with errors */
    if (handle == INVALID_HANDLE_VALUE) {
        errno = GetLastError();
        if (errno == ERROR_FILE_EXISTS)
            errno = ERROR_ALREADY_EXISTS;
        return -1;
    }
    /* Return the handle */
    return (int)handle;
}

int
_open(const char *filename, int oflag, ...)
{
    WCHAR wfilename[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wfilename,
                        (sizeof(wfilename) / sizeof(wfilename[0])));
    return _wopen(wfilename, oflag);
}

FILE *
_fdopen(int handle, const char *mode)
{
    WCHAR wmode[10];

    if (MultiByteToWideChar(CP_ACP, 0, mode, -1, wmode, 10) == 10) {
        errno = GetLastError();
        return NULL;
    }

    return _wfdopen((void *)handle, wmode);
}

/* Check an fopen()/_wfopen() mode string for validity */
static int
check_fopen_mode(const WCHAR *mode)
{
    /* Check each character in the mode string against the allowed characters */
    while (*mode) {
        switch (*mode++) {
            /* These are the valid mode characters */
            case L'r':
            case L'w':
            case L'a':
            case L'+':
            case L't':
            case L'b':
            case L'c':
            case L'n':
                break;
            /* Everything else is invalid */
            default:
                return 0;
        }
    }
    return 1;
}

/* Local version of fopen() that handles relative paths */
#undef _wfopen
FILE *
wince_fopen(const char *filename, const char *Mode)
{
    FILE *fp;
    WCHAR abs_path[MAX_PATH + 1];
    WCHAR wmode[8];

    /* Convert mode to wide string and check mode validity */
    MultiByteToWideChar(CP_ACP, 0, Mode, -1, wmode, (sizeof(wmode) / sizeof(wmode[0])));
    if (!check_fopen_mode(wmode)) {
        /* Return an error that will provoke an "invalid mode" error */
        errno = 0;
        return 0;
    }
    /* Try to do the normal fopen() */
    wince_absolute_path_to_wide(filename, abs_path, (sizeof(abs_path) / sizeof(abs_path[0])));
    fp = _wfopen(abs_path, wmode);
    if (!fp)
        errno = ENOENT;
    return fp;
}

/* Local version of _wfopen() that deals with relative paths */
FILE *
wince_wfopen(const WCHAR *filename, const WCHAR *Mode)
{
    FILE *fp;
    WCHAR abs_path[MAX_PATH + 1];

    /* Check mode validity */
    if (!check_fopen_mode(Mode)) {
        /* Return an error that will provoke an "invalid mode" error */
        errno = 0;
        return 0;
    }
    /* Call _wfopen with the absolute path */
    wince_absolute_path_wide(filename, abs_path, sizeof(abs_path) / sizeof(abs_path[0]));
    fp = _wfopen(abs_path, Mode);
    if (!fp)
        errno = ENOENT;
    return fp;
}

int
_close(int handle)
{
    if (CloseHandle((HANDLE)handle))
        return 0;
    errno = GetLastError();
    return -1;
}

int
_write(int handle, const void *buffer, unsigned int count)
{
    DWORD numwritten = 0;
    if (!WriteFile((HANDLE)handle, buffer, count, &numwritten, NULL)) {
        errno = GetLastError();
        return -1;
    }
    return numwritten;
}

int
_read(int handle, void *buffer, unsigned int count)
{
    DWORD numread = 0;
    if (!ReadFile((HANDLE)handle, buffer, count, &numread, NULL)) {
        errno = GetLastError();
        return -1;
    }
    return numread;
}

long
_lseek(int handle, long offset, int origin)
{
    DWORD move_method;
    DWORD result;

    switch (origin) {
        default:
            errno = EINVAL;
            return -1L;
        case SEEK_SET:
            move_method = FILE_BEGIN;
            break;
        case SEEK_CUR:
            move_method = FILE_CURRENT;
            break;
        case SEEK_END:
            move_method = FILE_END;
            break;
    }
    result = SetFilePointer((HANDLE)handle, offset, NULL, move_method);
    if (result == 0xFFFFFFFF) {
        errno = GetLastError();
        return -1;
    }
    if (result > 0x7FFFFFFF) {
        errno = EINVAL;
        return -1;
    }
    return (long)result;
}

__int64
_lseeki64(int handle, __int64 offset, int origin)
{
    DWORD move_method;
    DWORD result;
    DWORD high = (DWORD)(offset >> 32);

    switch (origin) {
        default:
            errno = EINVAL;
            return -1L;
        case SEEK_SET:
            move_method = FILE_BEGIN;
            break;
        case SEEK_CUR:
            move_method = FILE_CURRENT;
            break;
        case SEEK_END:
            move_method = FILE_END;
            break;
    }
    result = SetFilePointer((HANDLE)handle, (long)offset, &high, move_method);
    if (result == 0xFFFFFFFF) {
        errno = GetLastError();
        if (errno != NO_ERROR)
            return -1;
    }
    return result | ((__int64)high << 32);
}

int
_unlink(const char *path)
{
    WCHAR wide_path[MAX_PATH + 1];

    /*
     *	Delete the file
     */
    wince_absolute_path_to_wide(path, wide_path, MAX_PATH);
    if (!DeleteFile(wide_path)) {
        errno = GetLastError();
        return -1;
    }
    return 0;
}

int
_commit(int handle)
{
    if (!FlushFileBuffers((HANDLE)handle)) {
        errno = EBADF;
        return -1;
    }
    return 0;
}

int
_dup(int fd)
{
    HANDLE orgHandle;
    HANDLE newHandle;
    int fd2;
    orgHandle = _get_osfhandle(fd);
    DuplicateHandle(GetCurrentProcess(), &orgHandle, GetCurrentProcess(), &newHandle, 0, FALSE,
                    DUPLICATE_SAME_ACCESS);
    fd2 = (int)newHandle;
    return fd2;
}

size_t
strxfrm(char *dest, char *src, size_t n)
{
    size_t len = strlen(src);

    /* Just copy the string */
    if (n > 0)
        memcpy(dest, src, min(n, len + 1));
    return len;
}

int
_getpid(void)
{
    return (int)GetCurrentProcessId();
}

static HINSTANCE
shell_execute(HWND hwnd, const wchar_t *operation, const wchar_t *file, const wchar_t *params,
              const wchar_t *dir, int show_cmd)
{
    SHELLEXECUTEINFO info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.lpVerb = operation;
    info.lpFile = file;
    info.lpParameters = params;
    info.lpDirectory = dir;
    info.nShow = show_cmd;
    if (!ShellExecuteEx(&info))
        return info.hInstApp;
    /* From what I've seen, hInstApp is zero when successful */
    return (HINSTANCE)33; /* Success */
}

HINSTANCE
ShellExecuteA(HWND hwnd, const char *operation, const char *file, const char *params,
              const char *dir, int show_cmd)
{
    WCHAR wide_path[MAX_PATH + 1];
    WCHAR *woperation = NULL;
    WCHAR *wparams = NULL;
    WCHAR absolute_wdir[MAX_PATH + 1];
    WCHAR *wdir = NULL;

    if (operation) {
        int n = MultiByteToWideChar(CP_ACP, 0, operation, -1, NULL, 0);
        woperation = _alloca(n * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, operation, -1, woperation, n);
    }
    if (strstr(file, "://") != NULL)
        /* Don't do path conversions if it appears to be a URL */
        MultiByteToWideChar(CP_ACP, 0, file, -1, wide_path, MAX_PATH);
    else
        wince_absolute_path_to_wide(file, wide_path, MAX_PATH);
    if (params) {
        int n = MultiByteToWideChar(CP_ACP, 0, params, -1, NULL, 0);
        wparams = _alloca(n * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, params, -1, wparams, n);
    }
    if (dir) {
        wince_absolute_path_to_wide(file, wide_path, MAX_PATH);
        wdir = absolute_wdir;
    }

    return shell_execute(hwnd, woperation, wide_path, wparams, wdir, show_cmd);
}

HINSTANCE
ShellExecuteW(HWND hwnd, const wchar_t *operation, const wchar_t *file, const wchar_t *params,
              const wchar_t *dir, int show_cmd)
{
    WCHAR absolute_path[MAX_PATH + 1];
    LPCWSTR ppath;

    if (wcsstr(file, L"://") != NULL)
        /* Don't do path conversions if it appears to be a URL */
        ppath = file;
    else {
        wince_absolute_path_wide(file, absolute_path, MAX_PATH + 1);
        ppath = absolute_path;
    }

    return shell_execute(hwnd, operation, ppath, params, dir, show_cmd);
}

static char C_LOCALE[] = "C";
static char C_DECIMAL_POINT[] = ".";
static char EMPTY_STRING[] = "";

static struct lconv C_LOCALE_INFO = {
    C_DECIMAL_POINT, /* decimal_point */
    EMPTY_STRING,    /* thousands_sep */
    EMPTY_STRING,    /* grouping */
    EMPTY_STRING,    /* int_curr_symbol */
    EMPTY_STRING,    /* currency_symbol */
    EMPTY_STRING,    /* mon_decimal_point */
    EMPTY_STRING,    /* mon_thousands_sep */
    EMPTY_STRING,    /* mon_grouping */
    EMPTY_STRING,    /* positive_sign */
    EMPTY_STRING,    /* negative_sign */
    CHAR_MAX,        /* int_frac_digits */
    CHAR_MAX,        /* frac_digits */
    CHAR_MAX,        /* p_cs_precedes */
    CHAR_MAX,        /* p_sep_by_space */
    CHAR_MAX,        /* n_cs_precedes */
    CHAR_MAX,        /* n_sep_by_space */
    CHAR_MAX,        /* p_sign_posn */
    CHAR_MAX         /* n_sign_posn */
};

/* NOTE: Windows CE only supports the "C" locale */
char *
setlocale(int category, const char *locale)
{
    if (locale == NULL) {
        /* Get the current locale */
        return C_LOCALE;
    }
    /* Set the locale */
    if (strcmp(locale, "C") != 0) {
        return NULL; /* Locale not supported */
    }
    return C_LOCALE;
}

struct lconv *
localeconv(void)
{
    return &C_LOCALE_INFO;
}

int
GetLocaleInfoA(LCID lcid, LCTYPE lctype, char *buf, int buf_size)
{
    if (buf_size == 0) {
        return GetLocaleInfoW(lcid, lctype, NULL, 0) * 4;
    }
    else {
        WCHAR wbuf[256];
        GetLocaleInfoW(lcid, lctype, wbuf, sizeof(wbuf) / sizeof(wbuf[0]));
        return WideCharToMultiByte(CP_ACP, 0, wbuf, -1, buf, buf_size, NULL, NULL);
    }
}

HMODULE
LoadLibraryExA(const char *filename, void *reserved, DWORD flags)
{
    WCHAR wide_path[260];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wide_path,
                        sizeof(wide_path) / sizeof(wide_path[0]));
    return LoadLibraryW(wide_path);
}

DWORD
FormatMessageA(DWORD flags, const void *source, DWORD msg, DWORD lang, char *buf, DWORD buf_size,
               va_list *args)
{
    if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        void *pwbuf;
        void **ppbuf = (void **)buf;
        DWORD n;
        DWORD ret = FormatMessageW(flags, source, msg, lang, (WCHAR *)&pwbuf, buf_size, args);
        if (ret == 0)
            return 0;
        n = WideCharToMultiByte(CP_ACP, 0, pwbuf, -1, NULL, 0, NULL, NULL);
        *ppbuf = LocalAlloc(LMEM_FIXED, n);
        if (*ppbuf == NULL)
            return 0;
        ret = WideCharToMultiByte(CP_ACP, 0, pwbuf, -1, *ppbuf, n, NULL, NULL);
        LocalFree(pwbuf);
        return ret;
    }
    else {
        WCHAR wbuf[256];
        DWORD ret =
            FormatMessageW(flags, source, msg, lang, wbuf, sizeof(wbuf) / sizeof(wbuf[0]), NULL);
        return WideCharToMultiByte(CP_ACP, 0, wbuf, -1, buf, buf_size, NULL, NULL);
    }
}

void
OutputDebugStringA(const char *message)
{
    int n = MultiByteToWideChar(CP_ACP, 0, message, -1, NULL, 0);
    WCHAR *wmessage = (WCHAR *)_alloca(n * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, message, -1, wmessage, n);
    OutputDebugStringW(wmessage);
}

LONG
RegQueryValueExA(HKEY hkey, const char *value_name, DWORD *reserved, DWORD *type, BYTE *data,
                 DWORD *num_data)
{
    LONG r;
    int len = MultiByteToWideChar(CP_ACP, 0, value_name, -1, NULL, 0);
    WCHAR *wvalue_name = (WCHAR *)_alloca(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, value_name, -1, wvalue_name, len);

    if (data && !num_data)
        return ERROR_INVALID_PARAMETER;
    if (data) {
        DWORD num_wdata = *num_data * 4;
        BYTE *wdata = _alloca(num_wdata);
        r = RegQueryValueExW(hkey, wvalue_name, reserved, type, wdata, &num_wdata);
        if (r == ERROR_SUCCESS)
            WideCharToMultiByte(CP_ACP, 0, (WCHAR *)wdata, -1, data, *num_data, NULL, NULL);
    }
    else {
        r = RegQueryValueExW(hkey, wvalue_name, reserved, type, NULL, num_data);
        if (num_data)
            *num_data *= 4;
    }
    return r;
}

#undef FindFirstFileW
HANDLE
wince_FindFirstFileW(const wchar_t *filename, WIN32_FIND_DATAW *data)
{
    WCHAR absolute_path[MAX_PATH + 1];
    wince_absolute_path_wide(filename, absolute_path, MAX_PATH + 1);
    return FindFirstFileW(absolute_path, data);
}

HANDLE
FindFirstFileA(const char *filename, WIN32_FIND_DATAA *data)
{
    WCHAR absolute_path[MAX_PATH + 1];
    HANDLE handle;
    WIN32_FIND_DATAW wdata;

    /* Get the absolute path */
    wince_absolute_path_to_wide(filename, absolute_path, MAX_PATH);

    handle = wince_FindFirstFileW(absolute_path, &wdata);
    if (handle != INVALID_HANDLE_VALUE)
        find_data_wide_to_ansi(data, &wdata);
    return handle;
}

BOOL
FindNextFileA(HANDLE handle, WIN32_FIND_DATAA *data)
{
    WIN32_FIND_DATAW wdata;
    BOOL ret;

    ret = FindNextFileW(handle, &wdata);
    if (ret)
        find_data_wide_to_ansi(data, &wdata);
    return ret;
}

#undef CreateFileW
HANDLE
wince_CreateFileW(const wchar_t *filename, DWORD dwDesiredAccess, DWORD dwShareMode,
                  struct _SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwCreationDisposition,
                  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    WCHAR absolute_path[MAX_PATH + 1];
    wince_absolute_path_wide(filename, absolute_path, MAX_PATH + 1);
    dwFlagsAndAttributes &=
        FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_HIDDEN |
        FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM |
        FILE_ATTRIBUTE_ROMMODULE | FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_WRITE_THROUGH |
        FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS;
    if (dwDesiredAccess > 0 && dwDesiredAccess & (GENERIC_READ | GENERIC_WRITE) == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    dwDesiredAccess &= GENERIC_READ | GENERIC_WRITE;
    return CreateFileW(absolute_path, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                       dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

HANDLE
CreateFileA(const char *filename, DWORD dwDesiredAccess, DWORD dwShareMode,
            struct _SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwCreationDisposition,
            DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    WCHAR wfilename[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wfilename,
                        sizeof(wfilename) / sizeof(wfilename[0]));
    dwFlagsAndAttributes &=
        FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_HIDDEN |
        FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM |
        FILE_ATTRIBUTE_ROMMODULE | FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_WRITE_THROUGH |
        FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS;
    if (dwDesiredAccess > 0 && dwDesiredAccess & (GENERIC_READ | GENERIC_WRITE) > 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    dwDesiredAccess &= GENERIC_READ | GENERIC_WRITE;
    return wince_CreateFileW(wfilename, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                             dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

#undef GetFileAttributesW
DWORD
wince_GetFileAttributesW(const wchar_t *filename)
{
    WCHAR absolute_path[MAX_PATH + 1];
    wince_absolute_path_wide(filename, absolute_path, MAX_PATH + 1);
    return GetFileAttributesW(absolute_path);
}

DWORD
GetFileAttributesA(const char *filename)
{
    WCHAR wfilename[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wfilename,
                        sizeof(wfilename) / sizeof(wfilename[0]));
    return wince_GetFileAttributesW(wfilename);
}

#undef GetFileAttributesExW
BOOL
wince_GetFileAttributesExW(const wchar_t *filename, enum _GET_FILEEX_INFO_LEVELS fInfoLevelId,
                           void *lpFileInformation)
{
    WCHAR absolute_path[MAX_PATH + 1];
    wince_absolute_path_wide(filename, absolute_path, MAX_PATH + 1);
    return GetFileAttributesExW(absolute_path, fInfoLevelId, lpFileInformation);
}

BOOL
GetFileAttributesExA(const char *filename, enum _GET_FILEEX_INFO_LEVELS fInfoLevelId,
                     void *lpFileInformation)
{
    WCHAR wfilename[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wfilename,
                        sizeof(wfilename) / sizeof(wfilename[0]));
    return wince_GetFileAttributesExW(wfilename, fInfoLevelId, lpFileInformation);
}

#undef SetFileAttributesW
BOOL
wince_SetFileAttributesW(const wchar_t *filename, DWORD attr)
{
    WCHAR absolute_path[MAX_PATH + 1];
    wince_absolute_path_wide(filename, absolute_path, MAX_PATH + 1);
    return SetFileAttributesW(absolute_path, attr);
}

BOOL
SetFileAttributesA(const char *filename, DWORD attr)
{
    WCHAR wfilename[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, wfilename,
                        sizeof(wfilename) / sizeof(wfilename[0]));
    return wince_SetFileAttributesW(wfilename, attr);
}

#undef CreateDirectoryW
BOOL
wince_CreateDirectoryW(const wchar_t *path, struct _SECURITY_ATTRIBUTES *security)
{
    WCHAR absolute_path[MAX_PATH + 1];
    wince_absolute_path_wide(path, absolute_path, MAX_PATH + 1);
    return CreateDirectoryW(absolute_path, security);
}

BOOL
CreateDirectoryA(const char *path, struct _SECURITY_ATTRIBUTES *security)
{
    WCHAR wpath[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, sizeof(wpath) / sizeof(wpath[0]));
    return wince_CreateDirectoryW(wpath, security);
}

#undef RemoveDirectoryW
BOOL
wince_RemoveDirectoryW(const wchar_t *path)
{
    WCHAR absolute_path[MAX_PATH + 1];
    wince_absolute_path_wide(path, absolute_path, MAX_PATH + 1);
    return RemoveDirectoryW(absolute_path);
}

BOOL
RemoveDirectoryA(const char *path)
{
    WCHAR wpath[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, sizeof(wpath) / sizeof(wpath[0]));
    return wince_RemoveDirectoryW(wpath);
}

#undef MoveFileW
BOOL
wince_MoveFileW(const wchar_t *oldpath, const wchar_t *newpath)
{
    WCHAR absolute_path1[MAX_PATH + 1];
    WCHAR absolute_path2[MAX_PATH + 1];
    wince_absolute_path_wide(oldpath, absolute_path1, MAX_PATH + 1);
    wince_absolute_path_wide(newpath, absolute_path2, MAX_PATH + 1);
    return MoveFileW(absolute_path1, absolute_path2);
}

BOOL
MoveFileA(const char *oldpath, const char *newpath)
{
    WCHAR woldpath[MAX_PATH + 1];
    WCHAR wnewpath[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, oldpath, -1, woldpath, sizeof(woldpath) / sizeof(woldpath[0]));
    MultiByteToWideChar(CP_ACP, 0, newpath, -1, wnewpath, sizeof(wnewpath) / sizeof(wnewpath[0]));
    return wince_MoveFileW(woldpath, wnewpath);
}

#undef DeleteFileW
BOOL
wince_DeleteFileW(const wchar_t *path)
{
    WCHAR absolute_path[MAX_PATH + 1];
    wince_absolute_path_wide(path, absolute_path, MAX_PATH + 1);
    return DeleteFileW(absolute_path);
}

BOOL
DeleteFileA(const char *path)
{
    WCHAR wpath[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, sizeof(wpath) / sizeof(wpath[0]));
    return wince_DeleteFileW(wpath);
}

BOOL
CreateSymbolicLinkW(wchar_t *lpSymlinkFileName, wchar_t *lpTargetFileName, DWORD dwFlags)
{
    HANDLE hFile;
    wchar_t buffer[260];
    int result;
    int used = _snwprintf(&buffer[0], 3, L"%d#", wcslen(lpTargetFileName));
    used += _snwprintf(&buffer[used], 255, L"%s", lpTargetFileName);
    hFile = wince_CreateFileW(lpSymlinkFileName, (GENERIC_READ | GENERIC_WRITE), 0, NULL,
                              TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        errno = GetLastError();
        return 0;
    }
    if (!WriteFile(hFile, &buffer, used, NULL, NULL)) {
        errno = GetLastError();
        CloseHandle(hFile);
        return 0;
    }
    result = CloseHandle(hFile);
    if (!result) {
        errno = GetLastError();
        return 0;
    }
    return result;
}

char *
CharPrevA(const char *start, const char *current)
{
    return (char *)(current == start ? start : current - 1);
}

int
snprintf(char *buf, size_t buflen, const char *fmt, ...)
{
    va_list ap;
    int n;

    if (buflen == 0)
        return (0);

    va_start(ap, fmt);
    n = _vsnprintf(buf, buflen, fmt, ap);
    va_end(ap);

    if (n < 0 || (size_t)n >= buflen)
        n = buflen - 1;
    buf[n] = '\0';

    return (n);
}

/* from libc/include/_ansi.h */
#define _CONST const
#define _DEFUN(name, arglist, args) name(args)
#define _AND ,
/* from libc/time/local.h */
#define TZ_LOCK
#define TZ_UNLOCK
#define _tzname tzname
#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)
#define YEAR_BASE 1900
#define SECSPERMIN 60L
#define MINSPERHOUR 60L
#define HOURSPERDAY 24L
#define SECSPERHOUR (SECSPERMIN * MINSPERHOUR)

/*
 * strftime.c
 * Original Author:	G. Haley
 * Additions from:	Eric Blake
 *
 * Places characters into the array pointed to by s as controlled by the string
 * pointed to by format. If the total number of resulting characters including
 * the terminating null character is not more than maxsize, returns the number
 * of characters placed into the array pointed to by s (not including the
 * terminating null character); otherwise zero is returned and the contents of
 * the array indeterminate.
 */

/*
FUNCTION
<<strftime>>---flexible calendar time formatter

INDEX
    strftime

ANSI_SYNOPSIS
    #include <time.h>
    size_t strftime(char *<[s]>, size_t <[maxsize]>,
            const char *<[format]>, const struct tm *<[timp]>);

TRAD_SYNOPSIS
    #include <time.h>
    size_t strftime(<[s]>, <[maxsize]>, <[format]>, <[timp]>)
    char *<[s]>;
    size_t <[maxsize]>;
    char *<[format]>;
    struct tm *<[timp]>;

DESCRIPTION
<<strftime>> converts a <<struct tm>> representation of the time (at
<[timp]>) into a null-terminated string, starting at <[s]> and occupying
no more than <[maxsize]> characters.

You control the format of the output using the string at <[format]>.
<<*<[format]>>> can contain two kinds of specifications: text to be
copied literally into the formatted string, and time conversion
specifications.  Time conversion specifications are two- and
three-character sequences beginning with `<<%>>' (use `<<%%>>' to
include a percent sign in the output).  Each defined conversion
specification selects only the specified field(s) of calendar time
data from <<*<[timp]>>>, and converts it to a string in one of the
following ways:

o+
o %a
A three-letter abbreviation for the day of the week. [tm_wday]

o %A
The full name for the day of the week, one of `<<Sunday>>',
`<<Monday>>', `<<Tuesday>>', `<<Wednesday>>', `<<Thursday>>',
`<<Friday>>', or `<<Saturday>>'. [tm_wday]

o %b
A three-letter abbreviation for the month name. [tm_mon]

o %B
The full name of the month, one of `<<January>>', `<<February>>',
`<<March>>', `<<April>>', `<<May>>', `<<June>>', `<<July>>',
`<<August>>', `<<September>>', `<<October>>', `<<November>>',
`<<December>>'. [tm_mon]

o %c
A string representing the complete date and time, in the form
`<<"%a %b %e %H:%M:%S %Y">>' (example "Mon Apr 01 13:13:13
1992"). [tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday]

o %C
The century, that is, the year divided by 100 then truncated.  For
4-digit years, the result is zero-padded and exactly two characters;
but for other years, there may a negative sign or more digits.  In
this way, `<<%C%y>>' is equivalent to `<<%Y>>'. [tm_year]

o %d
The day of the month, formatted with two digits (from `<<01>>' to
`<<31>>'). [tm_mday]

o %D
A string representing the date, in the form `<<"%m/%d/%y">>'.
[tm_mday, tm_mon, tm_year]

o %e
The day of the month, formatted with leading space if single digit
(from `<<1>>' to `<<31>>'). [tm_mday]

o %E<<x>>
In some locales, the E modifier selects alternative representations of
certain modifiers <<x>>.  But in the "C" locale supported by newlib,
it is ignored, and treated as %<<x>>.

o %F
A string representing the ISO 8601:2000 date format, in the form
`<<"%Y-%m-%d">>'. [tm_mday, tm_mon, tm_year]

o %g
The last two digits of the week-based year, see specifier %G (from
`<<00>>' to `<<99>>'). [tm_year, tm_wday, tm_yday]

o %G
The week-based year. In the ISO 8601:2000 calendar, week 1 of the year
includes January 4th, and begin on Mondays. Therefore, if January 1st,
2nd, or 3rd falls on a Sunday, that day and earlier belong to the last
week of the previous year; and if December 29th, 30th, or 31st falls
on Monday, that day and later belong to week 1 of the next year.  For
consistency with %Y, it always has at least four characters.
Example: "%G" for Saturday 2nd January 1999 gives "1998", and for
Tuesday 30th December 1997 gives "1998". [tm_year, tm_wday, tm_yday]

o %h
A three-letter abbreviation for the month name (synonym for
"%b"). [tm_mon]

o %H
The hour (on a 24-hour clock), formatted with two digits (from
`<<00>>' to `<<23>>'). [tm_hour]

o %I
The hour (on a 12-hour clock), formatted with two digits (from
`<<01>>' to `<<12>>'). [tm_hour]

o %j
The count of days in the year, formatted with three digits
(from `<<001>>' to `<<366>>'). [tm_yday]

o %k
The hour (on a 24-hour clock), formatted with leading space if single
digit (from `<<0>>' to `<<23>>'). Non-POSIX extension. [tm_hour]

o %l
The hour (on a 12-hour clock), formatted with leading space if single
digit (from `<<1>>' to `<<12>>'). Non-POSIX extension. [tm_hour]

o %m
The month number, formatted with two digits (from `<<01>>' to `<<12>>').
[tm_mon]

o %M
The minute, formatted with two digits (from `<<00>>' to `<<59>>'). [tm_min]

o %n
A newline character (`<<\n>>').

o %O<<x>>
In some locales, the O modifier selects alternative digit characters
for certain modifiers <<x>>.  But in the "C" locale supported by newlib, it
is ignored, and treated as %<<x>>.

o %p
Either `<<AM>>' or `<<PM>>' as appropriate. [tm_hour]

o %r
The 12-hour time, to the second.  Equivalent to "%I:%M:%S %p". [tm_sec,
tm_min, tm_hour]

o %R
The 24-hour time, to the minute.  Equivalent to "%H:%M". [tm_min, tm_hour]

o %S
The second, formatted with two digits (from `<<00>>' to `<<60>>').  The
value 60 accounts for the occasional leap second. [tm_sec]

o %t
A tab character (`<<\t>>').

o %T
The 24-hour time, to the second.  Equivalent to "%H:%M:%S". [tm_sec,
tm_min, tm_hour]

o %u
The weekday as a number, 1-based from Monday (from `<<1>>' to
`<<7>>'). [tm_wday]

o %U
The week number, where weeks start on Sunday, week 1 contains the first
Sunday in a year, and earlier days are in week 0.  Formatted with two
digits (from `<<00>>' to `<<53>>').  See also <<%W>>. [tm_wday, tm_yday]

o %V
The week number, where weeks start on Monday, week 1 contains January 4th,
and earlier days are in the previous year.  Formatted with two digits
(from `<<01>>' to `<<53>>').  See also <<%G>>. [tm_year, tm_wday, tm_yday]

o %w
The weekday as a number, 0-based from Sunday (from `<<0>>' to `<<6>>').
[tm_wday]

o %W
The week number, where weeks start on Monday, week 1 contains the first
Monday in a year, and earlier days are in week 0.  Formatted with two
digits (from `<<00>>' to `<<53>>'). [tm_wday, tm_yday]

o %x
A string representing the complete date, equivalent to "%m/%d/%y".
[tm_mon, tm_mday, tm_year]

o %X
A string representing the full time of day (hours, minutes, and
seconds), equivalent to "%H:%M:%S". [tm_sec, tm_min, tm_hour]

o %y
The last two digits of the year (from `<<00>>' to `<<99>>'). [tm_year]

o %Y
The full year, equivalent to <<%C%y>>.  It will always have at least four
characters, but may have more.  The year is accurate even when tm_year
added to the offset of 1900 overflows an int. [tm_year]

o %z
The offset from UTC.  The format consists of a sign (negative is west of
Greewich), two characters for hour, then two characters for minutes
(-hhmm or +hhmm).  If tm_isdst is negative, the offset is unknown and no
output is generated; if it is zero, the offset is the standard offset for
the current time zone; and if it is positive, the offset is the daylight
savings offset for the current timezone. The offset is determined from
the TZ environment variable, as if by calling tzset(). [tm_isdst]

o %Z
The time zone name.  If tm_isdst is negative, no output is generated.
Otherwise, the time zone name is based on the TZ environment variable,
as if by calling tzset(). [tm_isdst]

o %%
A single character, `<<%>>'.
o-

RETURNS
When the formatted time takes up no more than <[maxsize]> characters,
the result is the length of the formatted string.  Otherwise, if the
formatting operation was abandoned due to lack of room, the result is
<<0>>, and the string starting at <[s]> corresponds to just those
parts of <<*<[format]>>> that could be completely filled in within the
<[maxsize]> limit.

PORTABILITY
ANSI C requires <<strftime>>, but does not specify the contents of
<<*<[s]>>> when the formatted string would require more than
<[maxsize]> characters.  Unrecognized specifiers and fields of
<<timp>> that are out of range cause undefined results.  Since some
formats expand to 0 bytes, it is wise to set <<*<[s]>>> to a nonzero
value beforehand to distinguish between failure and an empty string.
This implementation does not support <<s>> being NULL, nor overlapping
<<s>> and <<format>>.

<<strftime>> requires no supporting OS subroutines.
*/

static _CONST int dname_len[7] = {6, 6, 7, 9, 8, 6, 8};

static _CONST char *_CONST dname[7] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                                       "Thursday", "Friday", "Saturday"};

static _CONST int mname_len[12] = {7, 8, 5, 5, 3, 4, 4, 6, 9, 7, 8, 8};

static _CONST char *_CONST mname[12] = {"January",   "February", "March",    "April",
                                        "May",       "June",     "July",     "August",
                                        "September", "October",  "November", "December"};

/* Using the tm_year, tm_wday, and tm_yday components of TIM_P, return
   -1, 0, or 1 as the adjustment to add to the year for the ISO week
   numbering used in "%g%G%V", avoiding overflow.  */
static int
_DEFUN(iso_year_adjust, (tim_p), _CONST struct tm *tim_p)
{
    /* Account for fact that tm_year==0 is year 1900.  */
    int leap = isleap(tim_p->tm_year + (YEAR_BASE - (tim_p->tm_year < 0 ? 0 : 2000)));

    /* Pack the yday, wday, and leap year into a single int since there are so
       many disparate cases.  */
#define PACK(yd, wd, lp) (((yd) << 4) + (wd << 1) + (lp))
    switch (PACK(tim_p->tm_yday, tim_p->tm_wday, leap)) {
        case PACK(0, 5, 0):   /* Jan 1 is Fri, not leap.  */
        case PACK(0, 6, 0):   /* Jan 1 is Sat, not leap.  */
        case PACK(0, 0, 0):   /* Jan 1 is Sun, not leap.  */
        case PACK(0, 5, 1):   /* Jan 1 is Fri, leap year.  */
        case PACK(0, 6, 1):   /* Jan 1 is Sat, leap year.  */
        case PACK(0, 0, 1):   /* Jan 1 is Sun, leap year.  */
        case PACK(1, 6, 0):   /* Jan 2 is Sat, not leap.  */
        case PACK(1, 0, 0):   /* Jan 2 is Sun, not leap.  */
        case PACK(1, 6, 1):   /* Jan 2 is Sat, leap year.  */
        case PACK(1, 0, 1):   /* Jan 2 is Sun, leap year.  */
        case PACK(2, 0, 0):   /* Jan 3 is Sun, not leap.  */
        case PACK(2, 0, 1):   /* Jan 3 is Sun, leap year.  */
            return -1;        /* Belongs to last week of previous year.  */
        case PACK(362, 1, 0): /* Dec 29 is Mon, not leap.  */
        case PACK(363, 1, 1): /* Dec 29 is Mon, leap year.  */
        case PACK(363, 1, 0): /* Dec 30 is Mon, not leap.  */
        case PACK(363, 2, 0): /* Dec 30 is Tue, not leap.  */
        case PACK(364, 1, 1): /* Dec 30 is Mon, leap year.  */
        case PACK(364, 2, 1): /* Dec 30 is Tue, leap year.  */
        case PACK(364, 1, 0): /* Dec 31 is Mon, not leap.  */
        case PACK(364, 2, 0): /* Dec 31 is Tue, not leap.  */
        case PACK(364, 3, 0): /* Dec 31 is Wed, not leap.  */
        case PACK(365, 1, 1): /* Dec 31 is Mon, leap year.  */
        case PACK(365, 2, 1): /* Dec 31 is Tue, leap year.  */
        case PACK(365, 3, 1): /* Dec 31 is Wed, leap year.  */
            return 1;         /* Belongs to first week of next year.  */
    }
    return 0; /* Belongs to specified year.  */
#undef PACK
}

size_t
_DEFUN(strftime, (s, maxsize, format, tim_p),
       char *s _AND size_t maxsize _AND _CONST char *format _AND _CONST struct tm *tim_p)
{
    size_t count = 0;
    int i;

    for (;;) {
        while (*format && *format != '%') {
            if (count < maxsize - 1)
                s[count++] = *format++;
            else
                return 0;
        }

        if (*format == '\0')
            break;

        format++;
        if (*format == 'E' || *format == 'O')
            format++;

        switch (*format) {
            case 'a':
                for (i = 0; i < 3; i++) {
                    if (count < maxsize - 1)
                        s[count++] = dname[tim_p->tm_wday][i];
                    else
                        return 0;
                }
                break;
            case 'A':
                for (i = 0; i < dname_len[tim_p->tm_wday]; i++) {
                    if (count < maxsize - 1)
                        s[count++] = dname[tim_p->tm_wday][i];
                    else
                        return 0;
                }
                break;
            case 'b':
            case 'h':
                for (i = 0; i < 3; i++) {
                    if (count < maxsize - 1)
                        s[count++] = mname[tim_p->tm_mon][i];
                    else
                        return 0;
                }
                break;
            case 'B':
                for (i = 0; i < mname_len[tim_p->tm_mon]; i++) {
                    if (count < maxsize - 1)
                        s[count++] = mname[tim_p->tm_mon][i];
                    else
                        return 0;
                }
                break;
            case 'c': {
                /* Length is not known because of %C%y, so recurse. */
                size_t adjust =
                    strftime(&s[count], maxsize - count, "%a %b %e %H:%M:%S %C%y", tim_p);
                if (adjust > 0)
                    count += adjust;
                else
                    return 0;
            } break;
            case 'C': {
                /* Examples of (tm_year + YEAR_BASE) that show how %Y == %C%y
                   with 32-bit int.
                   %Y		%C		%y
                   2147485547	21474855	47
                   10000		100		00
                   9999		99		99
                   0999		09		99
                   0099		00		99
                   0001		00		01
                   0000		00		00
                   -001		-0		01
                   -099		-0		99
                   -999		-9		99
                   -1000		-10		00
                   -10000		-100		00
                   -2147481748	-21474817	48

                   Be careful of both overflow and sign adjustment due to the
                   asymmetric range of years.
                */
                int neg = tim_p->tm_year < -YEAR_BASE;
                int century = tim_p->tm_year >= 0 ? tim_p->tm_year / 100 + YEAR_BASE / 100
                                                  : abs(tim_p->tm_year + YEAR_BASE) / 100;
                count += snprintf(&s[count], maxsize - count, "%s%.*d", neg ? "-" : "", 2 - neg,
                                  century);
                if (count >= maxsize)
                    return 0;
            } break;
            case 'd':
            case 'e':
                if (count < maxsize - 2) {
                    sprintf(&s[count], *format == 'd' ? "%.2d" : "%2d", tim_p->tm_mday);
                    count += 2;
                }
                else
                    return 0;
                break;
            case 'D':
            case 'x':
                /* %m/%d/%y */
                if (count < maxsize - 8) {
                    sprintf(&s[count], "%.2d/%.2d/%.2d", tim_p->tm_mon + 1, tim_p->tm_mday,
                            tim_p->tm_year >= 0 ? tim_p->tm_year % 100
                                                : abs(tim_p->tm_year + YEAR_BASE) % 100);
                    count += 8;
                }
                else
                    return 0;
                break;
            case 'F': {
                /* Length is not known because of %C%y, so recurse. */
                size_t adjust = strftime(&s[count], maxsize - count, "%C%y-%m-%d", tim_p);
                if (adjust > 0)
                    count += adjust;
                else
                    return 0;
            } break;
            case 'g':
                if (count < maxsize - 2) {
                    /* Be careful of both overflow and negative years, thanks to
                   the asymmetric range of years.  */
                    int adjust = iso_year_adjust(tim_p);
                    int year = tim_p->tm_year >= 0 ? tim_p->tm_year % 100
                                                   : abs(tim_p->tm_year + YEAR_BASE) % 100;
                    if (adjust < 0 && tim_p->tm_year <= -YEAR_BASE)
                        adjust = 1;
                    else if (adjust > 0 && tim_p->tm_year < -YEAR_BASE)
                        adjust = -1;
                    sprintf(&s[count], "%.2d", ((year + adjust) % 100 + 100) % 100);
                    count += 2;
                }
                else
                    return 0;
                break;
            case 'G': {
                /* See the comments for 'C' and 'Y'; this is a variable length
                   field.  Although there is no requirement for a minimum number
                   of digits, we use 4 for consistency with 'Y'.  */
                int neg = tim_p->tm_year < -YEAR_BASE;
                int adjust = iso_year_adjust(tim_p);
                int century = tim_p->tm_year >= 0 ? tim_p->tm_year / 100 + YEAR_BASE / 100
                                                  : abs(tim_p->tm_year + YEAR_BASE) / 100;
                int year = tim_p->tm_year >= 0 ? tim_p->tm_year % 100
                                               : abs(tim_p->tm_year + YEAR_BASE) % 100;
                if (adjust < 0 && tim_p->tm_year <= -YEAR_BASE)
                    neg = adjust = 1;
                else if (adjust > 0 && neg)
                    adjust = -1;
                year += adjust;
                if (year == -1) {
                    year = 99;
                    --century;
                }
                else if (year == 100) {
                    year = 0;
                    ++century;
                }
                count += snprintf(&s[count], maxsize - count, "%s%.*d%.2d", neg ? "-" : "", 2 - neg,
                                  century, year);
                if (count >= maxsize)
                    return 0;
            } break;
            case 'H':
            case 'k':
                if (count < maxsize - 2) {
                    sprintf(&s[count], *format == 'k' ? "%2d" : "%.2d", tim_p->tm_hour);
                    count += 2;
                }
                else
                    return 0;
                break;
            case 'I':
            case 'l':
                if (count < maxsize - 2) {
                    if (tim_p->tm_hour == 0 || tim_p->tm_hour == 12) {
                        s[count++] = '1';
                        s[count++] = '2';
                    }
                    else {
                        sprintf(&s[count], *format == 'I' ? "%.2d" : "%2d", tim_p->tm_hour % 12);
                        count += 2;
                    }
                }
                else
                    return 0;
                break;
            case 'j':
                if (count < maxsize - 3) {
                    sprintf(&s[count], "%.3d", tim_p->tm_yday + 1);
                    count += 3;
                }
                else
                    return 0;
                break;
            case 'm':
                if (count < maxsize - 2) {
                    sprintf(&s[count], "%.2d", tim_p->tm_mon + 1);
                    count += 2;
                }
                else
                    return 0;
                break;
            case 'M':
                if (count < maxsize - 2) {
                    sprintf(&s[count], "%.2d", tim_p->tm_min);
                    count += 2;
                }
                else
                    return 0;
                break;
            case 'n':
                if (count < maxsize - 1)
                    s[count++] = '\n';
                else
                    return 0;
                break;
            case 'p':
                if (count < maxsize - 2) {
                    if (tim_p->tm_hour < 12)
                        s[count++] = 'A';
                    else
                        s[count++] = 'P';

                    s[count++] = 'M';
                }
                else
                    return 0;
                break;
            case 'r':
                if (count < maxsize - 11) {
                    if (tim_p->tm_hour == 0 || tim_p->tm_hour == 12) {
                        s[count++] = '1';
                        s[count++] = '2';
                    }
                    else {
                        sprintf(&s[count], "%.2d", tim_p->tm_hour % 12);
                        count += 2;
                    }
                    s[count++] = ':';
                    sprintf(&s[count], "%.2d", tim_p->tm_min);
                    count += 2;
                    s[count++] = ':';
                    sprintf(&s[count], "%.2d", tim_p->tm_sec);
                    count += 2;
                    s[count++] = ' ';
                    if (tim_p->tm_hour < 12)
                        s[count++] = 'A';
                    else
                        s[count++] = 'P';

                    s[count++] = 'M';
                }
                else
                    return 0;
                break;
            case 'R':
                if (count < maxsize - 5) {
                    sprintf(&s[count], "%.2d:%.2d", tim_p->tm_hour, tim_p->tm_min);
                    count += 5;
                }
                else
                    return 0;
                break;
            case 'S':
                if (count < maxsize - 2) {
                    sprintf(&s[count], "%.2d", tim_p->tm_sec);
                    count += 2;
                }
                else
                    return 0;
                break;
            case 't':
                if (count < maxsize - 1)
                    s[count++] = '\t';
                else
                    return 0;
                break;
            case 'T':
            case 'X':
                if (count < maxsize - 8) {
                    sprintf(&s[count], "%.2d:%.2d:%.2d", tim_p->tm_hour, tim_p->tm_min,
                            tim_p->tm_sec);
                    count += 8;
                }
                else
                    return 0;
                break;
            case 'u':
                if (count < maxsize - 1) {
                    if (tim_p->tm_wday == 0)
                        s[count++] = '7';
                    else
                        s[count++] = '0' + tim_p->tm_wday;
                }
                else
                    return 0;
                break;
            case 'U':
                if (count < maxsize - 2) {
                    sprintf(&s[count], "%.2d", (tim_p->tm_yday + 7 - tim_p->tm_wday) / 7);
                    count += 2;
                }
                else
                    return 0;
                break;
            case 'V':
                if (count < maxsize - 2) {
                    int adjust = iso_year_adjust(tim_p);
                    int wday = (tim_p->tm_wday) ? tim_p->tm_wday - 1 : 6;
                    int week = (tim_p->tm_yday + 10 - wday) / 7;
                    if (adjust > 0)
                        week = 1;
                    else if (adjust < 0)
                        /* Previous year has 53 weeks if current year starts on
                           Fri, and also if current year starts on Sat and
                           previous year was leap year.  */
                        week =
                            52 + (4 >= (wday - tim_p->tm_yday -
                                        isleap(tim_p->tm_year +
                                               (YEAR_BASE - 1 - (tim_p->tm_year < 0 ? 0 : 2000)))));
                    sprintf(&s[count], "%.2d", week);
                    count += 2;
                }
                else
                    return 0;
                break;
            case 'w':
                if (count < maxsize - 1)
                    s[count++] = '0' + tim_p->tm_wday;
                else
                    return 0;
                break;
            case 'W':
                if (count < maxsize - 2) {
                    int wday = (tim_p->tm_wday) ? tim_p->tm_wday - 1 : 6;
                    sprintf(&s[count], "%.2d", (tim_p->tm_yday + 7 - wday) / 7);
                    count += 2;
                }
                else
                    return 0;
                break;
            case 'y':
                if (count < maxsize - 2) {
                    /* Be careful of both overflow and negative years, thanks to
                   the asymmetric range of years.  */
                    int year = tim_p->tm_year >= 0 ? tim_p->tm_year % 100
                                                   : abs(tim_p->tm_year + YEAR_BASE) % 100;
                    sprintf(&s[count], "%.2d", year);
                    count += 2;
                }
                else
                    return 0;
                break;
            case 'Y': {
                /* Length is not known because of %C%y, so recurse. */
                size_t adjust = strftime(&s[count], maxsize - count, "%C%y", tim_p);
                if (adjust > 0)
                    count += adjust;
                else
                    return 0;
            } break;
            case 'z':
#ifndef _WIN32_WCE
                if (tim_p->tm_isdst >= 0) {
                    if (count < maxsize - 5) {
                        long offset;
                        __tzinfo_type *tz = __gettzinfo();
                        TZ_LOCK;
                        /* The sign of this is exactly opposite the envvar TZ.  We
                           could directly use the global _timezone for tm_isdst==0,
                           but have to use __tzrule for daylight savings.  */
                        offset = -tz->__tzrule[tim_p->tm_isdst > 0].offset;
                        TZ_UNLOCK;
                        sprintf(&s[count], "%+03ld%.2ld", offset / SECSPERHOUR,
                                labs(offset / SECSPERMIN) % 60L);
                        count += 5;
                    }
                    else
                        return 0;
                }
                break;
#endif
            case 'Z':
                if (tim_p->tm_isdst >= 0) {
                    int size;
                    TZ_LOCK;
                    size = strlen(_tzname[tim_p->tm_isdst > 0]);
                    for (i = 0; i < size; i++) {
                        if (count < maxsize - 1)
                            s[count++] = _tzname[tim_p->tm_isdst > 0][i];
                        else {
                            TZ_UNLOCK;
                            return 0;
                        }
                    }
                    TZ_UNLOCK;
                }
                break;
            case '%':
                if (count < maxsize - 1)
                    s[count++] = '%';
                else
                    return 0;
                break;
        }
        if (*format)
            format++;
        else
            break;
    }
    if (maxsize)
        s[count] = '\0';

    return count;
}

/* pathcch.h emulation */

HRESULT
PathCchCanonicalizeEx(wchar_t *pszPathOut, size_t cchPathOut, wchar_t *pszPathIn,
                      unsigned long dwFlags)
{
    /* read handed path from the end to the begining so we can make the result without going back */
    if (!dwFlags & PATHCCH_ALLOW_LONG_PATHS && cchPathOut > MAX_PATH + 1)
        return E_INVALIDARG;

    int isCur = 0;
    int isPar = 0;
    int ignores = 0;
    HRESULT result = S_OK;

    wchar_t *resPath = calloc(cchPathOut, sizeof(wchar_t));
    wchar_t *tmpPath = calloc(cchPathOut, sizeof(wchar_t));

    size_t length = 0;
    size_t lenTmp = 0;

    if (pszPathIn == NULL) {
        wmemcpy(pszPathOut, L"\0", 1);
        goto done;
    }

    int index = wcslen(pszPathIn) - 1;
    if (index < 0) {
        wmemcpy(pszPathOut, L"\0", 1);
        goto done;
    }

    while (index >= 0) {
        wcsncpy(tmpPath + (lenTmp + isCur + isPar), pszPathIn + index, 1);
        if ((*(pszPathIn + index) == L'\\' || *(pszPathIn + index) == L'/')) {
            if (isPar)
                ignores++;
            if (!isCur && !ignores) {
                wcsncpy(resPath + length, L"\\", 1);
                length++;
            }
            if (!isCur && ignores) {
                ignores--;
            }
            lenTmp = 0;
            isCur = 0;
            isPar = 0;
            index--;
            continue;
        }
        if (!lenTmp && !isPar && *(pszPathIn + index) == L'.') {
            if (!isCur)
                isCur = 1;
            else
                isPar = 1;
        }
        else if (isCur && (isPar || *(pszPathIn + index) == L'.')) {
            if (isPar) {
                wcsncpy(resPath + length, L"..", 2);
                length++;
                isCur = 0;
                isPar = 0;
                lenTmp = 2;
            }
            else {
                wcsncpy(resPath + length, L".", 1);
                isCur = 0;
                lenTmp = 1;
            }
        }
        if (!(isCur || ignores)) {
            wcsncpy(resPath + length, pszPathIn + index, 1);
            length++;
            lenTmp++;
        }
        index--;
    }
    if (ignores && *resPath == L'\0' && *tmpPath == L'\0') {
        wmemcpy(resPath, tmpPath, wcslen(tmpPath));
        length = lenTmp + isCur + isPar;
    }

    if (!dwFlags & PATHCCH_ALLOW_LONG_PATHS && length > MAX_PATH) {
        result = PATHCCH_E_FILENAME_TOO_LONG;
        goto done;
    }

    index++;
    while (index < length) {
        *(pszPathOut + index) = *(resPath + (length - index - 1));
        index++;
    }

    *(pszPathOut + length) = L'\0';

    goto done;
done:
    free(tmpPath);
    free(resPath);
    return result;
}

HRESULT
PathCchCombineEx(wchar_t *pszPathOut, size_t cchPathOut, wchar_t *pszPathIn, wchar_t *pszMore,
                 unsigned long dwFlags)
{
    HRESULT result;
    size_t combined_length = wcslen(pszPathIn) + 1 + wcslen(pszMore) + 1;
    wchar_t *combined;
    combined = (wchar_t *)calloc(combined_length, sizeof(wchar_t));
    if (combined == NULL)
        return E_OUTOFMEMORY;
    swprintf(combined, L"%ls\\%ls", pszPathIn, pszMore);
    result = PathCchCanonicalizeEx(pszPathOut, cchPathOut, combined, dwFlags);
    free(combined);
    return result;
}

HRESULT
PathCchSkipRoot(wchar_t *pszPath, wchar_t **ppszRootEnd)
{
    if (pszPath == NULL || wcslen(pszPath) == 0 || *pszPath != L'\\' && *pszPath != L'/')
        return E_INVALIDARG;
    *ppszRootEnd = pszPath + 1;
    return S_OK;
}

double
copysign(double x, double y)
{
    if (x >= 0 && y >= 0 || x < 0 && y < 0)
        return x;
    else
        return x * (-1);
}

int
_heapmin()
{
    errno = ENOSYS;
    return -1;
}

int
_locking(int fd, int mode, long nbytes)
{
    int res = -1;
    HANDLE hFile;
    OVERLAPPED overlapped;
    overlapped.Offset = overlapped.OffsetHigh = (DWORD)0;
    overlapped.hEvent = 0;

    hFile = _get_osfhandle(fd);
    if (hFile == -1) {
        errno = EBADF;
        return -1;
    }
    if (mode == _LK_LOCK || mode == _LK_RLCK) {
        int i;
        i = 0;
        while (i++ <= 10) {
            if (LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK || LOCKFILE_FAIL_IMMEDIATELY, 0,
                           LOWORD(nbytes), HIWORD(nbytes), &overlapped)) {
                res = 0;
                break;
            }
            Sleep(1000);
        }
        if (res == -1)
            errno = EDEADLOCK;
        return res;
    }
    else if (mode == _LK_NBLCK || mode == _LK_NBRLCK) {
        if (LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK || LOCKFILE_FAIL_IMMEDIATELY, 0,
                       LOWORD(nbytes), HIWORD(nbytes), &overlapped))
            return 0;
        else {
            errno = EACCES;
            return -1;
        }
    }
    else {
        errno = EINVAL;
    }
    return res;
}

wchar_t **
CommandLineToArgvW(const wchar_t *lpCmdLine, int *pNumArgs)
{
    wchar_t **argv;
    argv = (wchar_t **)calloc(64, sizeof(wchar_t *));
    if (argv == NULL)
        return NULL;

    int argc = 1;
    int i = -1;
    int i2 = 0;
    int quoted = 0;
    int backslash = 0;
    int spaced = 1;
    int error = 0;

    int argTmpSize = 64;

    int cmdlen = (int)wcslen(lpCmdLine);

    wchar_t *curChar = lpCmdLine;

    wchar_t *argTmp;
    wchar_t *argTmpOrg;
    argTmpOrg = (wchar_t *)calloc(64, sizeof(wchar_t));
    argTmp = argTmpOrg;

    if (argTmp == NULL) {
        free(argv);
        return NULL;
    }

    wchar_t exeName[64] = L"";
    if (exeName == NULL) {
        free(argv);
        free(argTmpOrg);
        return NULL;
    }
    GetModuleFileName(NULL, exeName, 64);
    argv[0] = exeName;

    while (i < cmdlen) {
        if (i < 0 && cmdlen == 0)
            break;
        i++;
        if (spaced && *curChar != L' ' && *curChar != L'\t') {
            spaced = 0;
            i2 = 0;
            argc++;
        }
        if (i2 >= argTmpSize) {
            wchar_t *tmp = realloc(argTmpOrg, (argTmpSize += 16) * sizeof(wchar_t));
            if (tmp == NULL) {
                error = 1;
                break;
            }
            argTmp = tmp + (argTmpOrg - argTmp);
            argTmpOrg = tmp;
        }
        if (*curChar == L'\\') {
            backslash++;
            curChar++;
            continue;
        }
        if (*curChar == L'"') {
            if (i2 + (backslash + 1) / 2 >= argTmpSize) {
                wchar_t *tmp = realloc(
                    argTmpOrg, sizeof(wchar_t) * (argTmpSize += 16 * ((backslash + 1) / 2)));
                if (tmp == NULL) {
                    error = 1;
                    break;
                }
                argTmp = tmp + (argTmpOrg - argTmp);
                argTmpOrg = tmp;
            }
            for (int index = 0; index < backslash; index += 2) {
                *argTmp = L'\\';
                backslash -= 2;
                argTmp++;
                i2++;
            }
            if (backslash) {
                *argTmp = L'"';
                argTmp++;
                i2++;
            }
            if (quoted)
                quoted = backslash;
            else
                quoted = 1 - backslash;
            backslash = 0;
            curChar++;
            continue;
        }
        if (backslash) {
            for (int index = 0; index < backslash; index++) {
                *argTmp = L'\\';
                argTmp++;
                i2++;
            }
            backslash = 0;
        }
        if (!spaced && !quoted && (*curChar == L' ' || *curChar == L'\t' || *curChar == L'\0')) {
            argv[argc - 1] = (wchar_t *)calloc(i2 + 1, sizeof(wchar_t));
            if (argv[argc - 1] == NULL) {
                error = 1;
                break;
            }
            wcscpy(argv[argc - 1], argTmpOrg);
            argTmp = argTmpOrg;
            wmemset(argTmpOrg, L'\0', i2);
            spaced = 1;
            curChar++;
            continue;
        }
        *argTmp = *curChar;
        argTmp++;
        if (*curChar == L'\0')
            break;
        curChar++;
        i2++;
    }
    if (error) {
        free(argv);
        free(argTmpOrg);
        return NULL;
    }

    wchar_t **result = realloc(argv, sizeof(wchar_t *) * argc);
    if (result != NULL) {
        *pNumArgs = argc;
    }
    else {
        free(argv);
    }
    free(argTmpOrg);
    return result;
}

DWORD
wince_GetEnvironmentVariable()
{
    SetLastError(ERROR_ENVVAR_NOT_FOUND);
    return 0;
}

/*
 * This function is needed because the default Windows CE implementation
 * does not handle EOF (-1) properly
 */
#undef _isctype
int
wince_isctype(int ch, int classification)
{
    if (ch == EOF)
        return 0;
    return _isctype(ch, classification);
}
