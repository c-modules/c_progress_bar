#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "c_progress_bar.h"
#include "internal/math_utils.h"
#include "internal/system_utils.h"

CPB_Config cpb_get_default_config(void)
{
    CPB_Config config = {.min_refresh_time = 0.1};
    return config;
}

void cpb_init(
    CPB_ProgressBar *restrict progress_bar,
    int64_t start,
    int64_t total,
    CPB_Config config
)
{
    if (!progress_bar)
    {
        return;
    }

    // Must call before using timer
    progress_bar->internal._timer_freq_inv = get_timer_freq_inv();

    progress_bar->start = start;
    progress_bar->total = total;
    progress_bar->current = start;

    progress_bar->is_started = false;
    progress_bar->is_finished = false;

    progress_bar->config = config;

    progress_bar->internal.unique_updates_count = 0;
    progress_bar->internal.window_index = 0;
    progress_bar->internal.time_start = 0.0;
    progress_bar->internal.timer_time_last_update = 0.0;
    progress_bar->internal.timer_percent_last_update = 0.0;
    for (int i = 0; i < 5; i++)
    {
        progress_bar->internal.timer_time_diffs[i] = 0.0;
        progress_bar->internal.timer_percent_diffs[i] = 0.0;
    }
}

void cpb_start(CPB_ProgressBar *restrict progress_bar)
{
    if (!progress_bar)
    {
        return;
    }

    progress_bar->is_started = true;
    progress_bar->internal.time_start = get_monotonic_time(progress_bar);
    progress_bar->internal.timer_time_last_update = progress_bar->internal.time_start;
    progress_bar->internal.timer_percent_last_update =
        calculate_percentage(progress_bar);

    printf(
        "\r\033[2K\033[?25lProgress: %zu / %zu %3d%%",
        progress_bar->current,
        progress_bar->total,
        (int)progress_bar->internal.timer_percent_last_update
    );
    fflush(stdout);
}

void cpb_update(CPB_ProgressBar *restrict progress_bar, int64_t current)
{
    if (!progress_bar)
    {
        return;
    }

    progress_bar->current = current;
    double current_time = get_monotonic_time(progress_bar);
    if ((current_time - progress_bar->internal.timer_time_last_update) <
        progress_bar->config.min_refresh_time)
    {
        return;
    }

    const double progress_percent = (int)calculate_percentage(progress_bar);

    // WARNING: zu is wrong, should remove later
    printf(
        "\r\033[2K\033[?25lProgress: %zu / %zu %3d%%",
        progress_bar->current,
        progress_bar->total,
        (int)progress_percent
    );
    fflush(stdout);

    progress_bar->internal.timer_time_last_update = current_time;
    progress_bar->internal.timer_percent_last_update = progress_percent;
}

void cpb_finish(CPB_ProgressBar *restrict progress_bar)
{
    if (!progress_bar)
    {
        return;
    }

    const double progress_percent = (int)calculate_percentage(progress_bar);
    progress_bar->is_finished = true;

    printf(
        "\r\033[2K\033[?25lProgress: %zu / %zu %3d%%\033[?25h\n",
        progress_bar->current,
        progress_bar->total,
        (int)progress_percent
    );
    fflush(stdout);
}

