/**
 * \file math_utils.c
 * \brief Mathematical utility functions for C Progress Bar library.
 *
 * \author Ching-Yin Ng
 */

#include <stdint.h>

#include "c_progress_bar.h"
#include "internal/math_utils.h"

double calculate_percentage(const CPB_ProgressBar *restrict progress_bar)
{
    const int64_t start = progress_bar->start;
    const int64_t total = progress_bar->total - start;
    const int64_t current = progress_bar->current - start;

    if (total <= 0 || current <= 0)
    {
        return 0.0;
    }
    if (current >= total)
    {
        return 100.0;
    }

    const double percentage = ((double)current / (double)total) * 100.0;
    return percentage;
}

double calculate_overall_rate(const CPB_ProgressBar *restrict progress_bar)
{
    const double elapsed_time =
        (progress_bar->internal.timer_time_last_update -
         progress_bar->internal.time_start);

    if (elapsed_time <= 0.0)
    {
        return 0.0;
    }

    return (double)progress_bar->internal.timer_percentage_last_update / elapsed_time;
}

double calculate_recent_rate(const CPB_ProgressBar *restrict progress_bar)
{
    // We don't need recent rate anyways if we only have a few data points
    if (progress_bar->internal.updates_count <= CPB_TIMER_DATA_POINTS)
    {
        return calculate_overall_rate(progress_bar);
    }

    double sum_time = 0.0;
    double sum_percent = 0.0;

    for (int i = 0; i < CPB_TIMER_DATA_POINTS; i++)
    {
        sum_time += progress_bar->internal.timer_time_diffs[i];
        sum_percent += progress_bar->internal.timer_percentage_diffs[i];
    }

    if (sum_time <= 1e-9)
    {
        return calculate_overall_rate(progress_bar);
    }

    return sum_percent / sum_time;
}