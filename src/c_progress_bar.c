#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "c_progress_bar.h"
#include "internal/math_utils.h"
#include "internal/system_utils.h"

static bool update_timer_data(CPB_ProgressBar *restrict progress_bar);
static void print_elapsed_time(const CPB_ProgressBar *restrict progress_bar);
static void print_remaining_time(const CPB_ProgressBar *restrict progress_bar);
static void print_progress_bar(const CPB_ProgressBar *restrict progress_bar);

CPB_Config cpb_get_default_config(void)
{
    CPB_Config config = {
        .min_refresh_time = 0.1, .timer_remaining_time_recent_weight = 0.3
    };
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

    progress_bar->internal.updates_count = -1;
    progress_bar->internal.time_start = 0.0;
    progress_bar->internal.timer_time_last_update = 0.0;
    progress_bar->internal.timer_percentage_last_update = 0.0;
    for (int i = 0; i < CPB_TIMER_DATA_POINTS; i++)
    {
        progress_bar->internal.timer_time_diffs[i] = 0.0;
        progress_bar->internal.timer_percentage_diffs[i] = 0.0;
    }
}

void cpb_start(CPB_ProgressBar *restrict progress_bar)
{
    if (!progress_bar)
    {
        return;
    }

    progress_bar->is_started = true;
    (void)update_timer_data(progress_bar);
    print_progress_bar(progress_bar);
}

void cpb_update(CPB_ProgressBar *restrict progress_bar, int64_t current)
{
    if (!progress_bar)
    {
        return;
    }

    progress_bar->current = current;
    if (!update_timer_data(progress_bar))
    {
        return;
    }

    print_progress_bar(progress_bar);
}

void cpb_finish(CPB_ProgressBar *restrict progress_bar)
{
    if (!progress_bar)
    {
        return;
    }

    progress_bar->is_finished = true;
    (void)update_timer_data(progress_bar);
    print_progress_bar(progress_bar);
}

static bool update_timer_data(CPB_ProgressBar *restrict progress_bar)
{
    if (!progress_bar)
    {
        return false;
    }

    if (progress_bar->is_finished)
    {
        progress_bar->internal.timer_time_last_update =
            get_monotonic_time(progress_bar);
        progress_bar->internal.timer_percentage_last_update = 100.0;
        return true;
    }

    if (progress_bar->internal.updates_count < 0)
    {
        const double current_time = get_monotonic_time(progress_bar);
        progress_bar->internal.time_start = current_time;
        progress_bar->internal.timer_time_last_update = current_time;
        progress_bar->internal.timer_percentage_last_update =
            calculate_percentage(progress_bar);
        progress_bar->internal.updates_count = 0;
        return true;
    }

    const double current_time = get_monotonic_time(progress_bar);
    const double diff_time =
        current_time - progress_bar->internal.timer_time_last_update;
    if (diff_time < progress_bar->config.min_refresh_time)
    {
        return false;
    }

    const double current_percentage = calculate_percentage(progress_bar);
    const double diff_percentage =
        current_percentage - progress_bar->internal.timer_percentage_last_update;
    progress_bar->internal.timer_time_diffs
        [progress_bar->internal.updates_count % CPB_TIMER_DATA_POINTS] = diff_time;
    progress_bar->internal.timer_percentage_diffs
        [progress_bar->internal.updates_count % CPB_TIMER_DATA_POINTS] =
        diff_percentage;

    progress_bar->internal.timer_time_last_update = current_time;
    progress_bar->internal.timer_percentage_last_update = current_percentage;
    progress_bar->internal.updates_count++;

    return true;
}

static void print_elapsed_time(const CPB_ProgressBar *restrict progress_bar)
{
    const double elapsed_time =
        (progress_bar->internal.timer_time_last_update -
         progress_bar->internal.time_start);

    // Calculate elapsed hours, minutes and seconds
    const int hours = ((int)elapsed_time) / 3600;
    const int minutes = ((int)elapsed_time % 3600) / 60;
    const int seconds = ((int)elapsed_time % 60);

    if (hours < 0 || minutes < 0 || seconds < 0)
    {
        fputs("--:--:--", stdout);
    }
    else
    {
        printf("%02d:%02d:%02d", hours, minutes, seconds);
    }
}

static void print_remaining_time(const CPB_ProgressBar *restrict progress_bar)
{
    const double overall_rate = calculate_overall_rate(progress_bar);
    const double recent_rate = calculate_recent_rate(progress_bar);
    const double blended_rate =
        progress_bar->config.timer_remaining_time_recent_weight * recent_rate +
        (1.0 - progress_bar->config.timer_remaining_time_recent_weight) * overall_rate;

    if (blended_rate <= 0.0)
    {
        fputs("--:--:--", stdout);
        return;
    }

    const double remaining_percentage =
        100.0 - progress_bar->internal.timer_percentage_last_update;
    const double estimated_remaining_time = remaining_percentage / blended_rate;

    // Calculate remaining hours, minutes and seconds
    const int hours = ((int)estimated_remaining_time) / 3600;
    const int minutes = ((int)estimated_remaining_time % 3600) / 60;
    const int seconds = ((int)estimated_remaining_time % 60);

    if (hours < 0 || minutes < 0 || seconds < 0)
    {
        fputs("--:--:--", stdout);
    }
    else
    {
        printf("%02d:%02d:%02d", hours, minutes, seconds);
    }
}

static void print_progress_bar(const CPB_ProgressBar *restrict progress_bar)
{
    printf(
        "\r\033[2K\033[?25lProgress: %zu / %zu %3d%% ",
        progress_bar->current,
        progress_bar->total,
        (int)progress_bar->internal.timer_percentage_last_update
    );
    print_elapsed_time(progress_bar);
    fputs(" ", stdout);
    print_remaining_time(progress_bar);
    if (progress_bar->is_finished)
    {
        fputs("\033[?25h\n", stdout);
    }
    fflush(stdout);
}