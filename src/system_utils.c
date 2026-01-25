/**
 * \file system_utils.c
 * \brief System utility functions for C Progress Bar library.
 *
 * \author Ching-Yin Ng
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c_progress_bar.h"
#include "internal/system_utils.h"

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <langinfo.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

/**
 * \brief Helper function to search for UTF-8 indicators in a string.
 *
 * \param[in] str The string to search.
 * \return true if any UTF-8 indicators are found, false otherwise.
 */
static bool contains_utf8_case_insensitive(const char *str)
{
    if (!str)
    {
        return false;
    }

    while (*str)
    {
        if (tolower((unsigned char)str[0]) == 'u' &&
            tolower((unsigned char)str[1]) == 't' &&
            tolower((unsigned char)str[2]) == 'f')
        {
            // Check for "utf8" or "utf-8"
            if (str[3] == '8')
            {
                return true;
            }
            if (str[3] == '-' && str[4] == '8')
            {
                return true;
            }
        }
        str++;
    }
    return false;
}

/**
 * \brief Helper function to determine if the terminal supports UTF-8 encoding.
 *
 * \return true if the terminal supports UTF-8, false otherwise.
 */
static bool terminal_supports_utf8(void)
{
#ifdef _WIN32
    return GetConsoleOutputCP() == CP_UTF8;
#else
    const char *codeset = nl_langinfo(CODESET);
    if (codeset && contains_utf8_case_insensitive(codeset))
    {
        return true;
    }

    const char *env_vars[] = {"LC_ALL", "LC_CTYPE", "LANG"};
    for (int i = 0; i < 3; i++)
    {
        const char *val = getenv(env_vars[i]);
        if (val && val[0])
        {
            if (contains_utf8_case_insensitive(val))
            {
                return true;
            }

            if (i == 0)
            {
                return false;
            }
        }
    }
    return false;
#endif /* _WIN32 */
}

bool should_use_utf8(FILE *stream)
{
    if (!ISATTY(FILENO(stream)))
    {
        return true;
    }

    return terminal_supports_utf8();
}

bool should_use_color(FILE *stream)
{
    // NO_COLOR set
    // Don’t output ANSI color escape codes, see no-color.org
    const char *no_color = getenv("NO_COLOR");
    if (no_color && no_color[0] != '\0')
    {
        return false;
    }

    // CLICOLOR_FORCE set, but NO_COLOR unset
    // ANSI colors should be enabled no matter what
    const char *force_color = getenv("CLICOLOR_FORCE");
    if (force_color && strcmp(force_color, "0") != 0)
    {
        return true;
    }

    // Check if the stream is a terminal
    int fd = FILENO(stream);
    if (!ISATTY(fd))
    {
        return false;
    }

    // Check TERM variable for "dumb" terminals
    const char *term = getenv("TERM");
    if (term && strcmp(term, "dumb") == 0)
    {
        return false;
    }

// Windows Specific: Enable Virtual Terminal Processing (ANSI support)
#ifdef _WIN32
    HANDLE hOut = (HANDLE)_get_osfhandle(fd);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
    {
        return false;
    }

// ENABLE_VIRTUAL_TERMINAL_PROCESSING might not be defined in older SDKs
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

    if (!(dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    {
        if (!SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        {
            return false; // Failed to enable ANSI support on this Windows console
        }
    }
#endif /* _WIN32 */

    return true;
}

int get_terminal_width(FILE *stream)
{
    if (!stream)
    {
        return CPB_DEFAULT_FILE_WIDTH;
    }

#ifdef _WIN32
    const int fd = FILENO(stream);
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (hFile != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hFile, &csbi))
    {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
#else
    struct winsize w;
    const int fd = FILENO(stream);

    if (ioctl(fd, TIOCGWINSZ, &w) != -1)
    {
        return w.ws_col;
    }
#endif /* _WIN32 */

    /* Fallback */

    // Check COLUMNS environment variable
    const char *env_cols = getenv("COLUMNS");
    if (env_cols)
    {
        return atoi(env_cols);
    }

    return CPB_DEFAULT_TERMINAL_WIDTH;
}

double get_timer_freq_inv(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return 1.0 / (double)freq.QuadPart;
#else
    return 1.0;
#endif /* _WIN32 */
}

double get_monotonic_time(const CPB_ProgressBar *restrict progress_bar)
{
#ifdef _WIN32
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart * progress_bar->internal._timer_freq_inv;

#else
    (void)progress_bar;
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return 0.0;
    }

    return (double)ts.tv_sec + ((double)ts.tv_nsec / 1e9);
#endif /* _WIN32 */
}