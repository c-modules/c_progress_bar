/**
 * \file c_progress_bar.h
 * \brief Library header for C Progress Bar library
 *
 * \author Ching-Yin Ng
 */

#ifndef C_PROGRESS_BAR_H
#define C_PROGRESS_BAR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef CPB_VERSION
#define CPB_VERSION "Unknown"
#endif

// Terminal width when it cannot be determined
#define CPB_DEFAULT_TERMINAL_WIDTH 80

// Default output width when printing to file
#define CPB_DEFAULT_FILE_WIDTH 120

// Number of data points to keep for timer calculations
#define CPB_TIMER_DATA_POINTS 5

// Progress bar default width
#define CPB_PROGRESS_BAR_DEFAULT_WIDTH 40

typedef struct CPB_Config
{
    char *description;
    double min_refresh_time;
    double timer_remaining_time_recent_weight;
} CPB_Config;

typedef struct CPB_ProgressBar
{
    int64_t start;
    int64_t total;
    int64_t current;

    bool is_started;
    bool is_finished;

    CPB_Config config;

    struct
    {
        int64_t updates_count;
        double time_start;
        double timer_time_last_update;
        double timer_percentage_last_update;
        double timer_time_diffs[CPB_TIMER_DATA_POINTS];
        double timer_percentage_diffs[CPB_TIMER_DATA_POINTS];

        // For monotonic time calculation on Windows
        double _timer_freq_inv;
    } internal;
} CPB_ProgressBar;

/**
 * \brief Print compilation information such as compiler version,
 *        compilation date, and configurations.
 */
// void cpb_print_compilation_info(void);

CPB_Config cpb_get_default_config(void);
void cpb_init(
    CPB_ProgressBar *restrict progress_bar,
    int64_t start,
    int64_t total,
    CPB_Config config
);
void cpb_start(CPB_ProgressBar *restrict progress_bar);
void cpb_update(CPB_ProgressBar *restrict progress_bar, int64_t current);
void cpb_finish(CPB_ProgressBar *restrict progress_bar);

#endif /* C_PROGRESS_BAR_H */
