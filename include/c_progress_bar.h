/**
 * \file c_progress_bar.h
 * \brief Library header for C Progress Bar library
 *
 * \author Ching-Yin Ng
 */

#ifndef C_PROGRESS_BAR_H
#define C_PROGRESS_BAR_H

#include <stddef.h>

#ifndef CPB_VERSION
#define CPB_VERSION "Unknown"
#endif

// Terminal width when it cannot be determined
#define CPB_DEFAULT_TERMINAL_WIDTH 80

// Default output width when printing to file
#define CPB_DEFAULT_FILE_WIDTH 120

typedef struct CPB_ProgressBar
{
    size_t start;
    size_t total;

    bool is_completed;

    size_t unique_updates_count;
    double time_last_update;
    double time_diff_last_ten_unique_update[10];
    double last_ten_unique_progress_percent[10];
} CPB_ProgressBar;

/**
 * \brief Print compilation information such as compiler version,
 *        compilation date, and configurations.
 */
// void cpb_print_compilation_info(void);

#endif /* C_PROGRESS_BAR_H */
