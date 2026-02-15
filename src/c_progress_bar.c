#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "c_progress_bar.h"
#include "internal/math_utils.h"
#include "internal/system_utils.h"

typedef struct
{
    const bool is_utf8;

    const char *reset;
    const char *erase_current_line;
    const char *disable_cursor;

    const char *bar_fill;
    const char *bar_empty;
    const char *bar_fill_head;
    const char *bar_empty_head;
    const char *separator;

    const char *color_spinner;
    const char *color_fill;
    const char *color_fill_after_ended;
    const char *color_empty;
    const char *color_percentage;
    const char *color_remaining_time;
    const char *color_elapsed_time;

    const int spinner_animation_length;
    const char *spinner[9];
} UTF8Codes;

static bool update_timer_data(CPB_ProgressBar *restrict progress_bar);
static void print_elapsed_time(const CPB_ProgressBar *restrict progress_bar);
static void print_remaining_time(const CPB_ProgressBar *restrict progress_bar);
static UTF8Codes get_utf8_codes(const CPB_ProgressBar *restrict progress_bar);
static void print_progress_bar(const CPB_ProgressBar *restrict progress_bar);

CPB_Config cpb_get_default_config(void)
{
    CPB_Config config = {
        .description = "",
        .min_refresh_time = 0.1,
        .timer_remaining_time_recent_weight = 0.3
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
    if (update_timer_data(progress_bar))
    {
        print_progress_bar(progress_bar);
    }
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
    if (update_timer_data(progress_bar))
    {
        print_progress_bar(progress_bar);
    }
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

static UTF8Codes get_utf8_codes(const CPB_ProgressBar *restrict progress_bar)
{
    (void)progress_bar; // Reserved for future use
    if (should_use_utf8(stdout) && should_use_color(stdout))
    {
        return (UTF8Codes){
            .is_utf8 = true,

            .reset = "\033[0m",
            .erase_current_line = "\033[2K",
            .disable_cursor = "\033[?25l",

            .bar_fill = "\u2501",
            .bar_empty = "\u2501",
            .bar_fill_head = "\u2578",
            .bar_empty_head = "\u257A",
            .separator = "\u2022",

            .color_spinner = "\033[0;32m",
            .color_fill = "\033[38;5;197m",
            .color_fill_after_ended = "\033[38;5;106m",
            .color_empty = "\033[0;90m",
            .color_percentage = "\033[0;35m",
            .color_remaining_time = "\033[0;36m",
            .color_elapsed_time = "\033[0;33m",

            .spinner_animation_length = 9,
            .spinner =
                {
                    "\u280B",
                    "\u2819",
                    "\u2839",
                    "\u2838",
                    "\u283C",
                    "\u2834",
                    "\u2826",
                    "\u2827",
                    "\u2807",
                },
        };
    }
    else
    {
        return (UTF8Codes){
            .is_utf8 = false,

            .reset = "",
            .erase_current_line = "",
            .disable_cursor = "",

            .bar_fill = "=",
            .bar_empty = " ",
            .bar_fill_head = ">",
            .bar_empty_head = ">",
            .separator = "*",

            .color_spinner = "",
            .color_fill = "",
            .color_fill_after_ended = "",
            .color_empty = "",
            .color_percentage = "",
            .color_remaining_time = "",
            .color_elapsed_time = "",

            .spinner_animation_length = -1,
            .spinner = {NULL},
        };
    }
}

static void print_progress_bar(const CPB_ProgressBar *restrict progress_bar)
{
    const UTF8Codes utf8_codes = get_utf8_codes(progress_bar);
    fputs("\r", stdout);
    if (utf8_codes.is_utf8)
    {
        fputs(utf8_codes.reset, stdout);
        fputs(utf8_codes.disable_cursor, stdout);
        fputs(utf8_codes.erase_current_line, stdout);
    }

    const double percentage = progress_bar->internal.timer_percentage_last_update;
    const double clamped =
        percentage < 0.0 ? 0.0 : (percentage > 100.0 ? 100.0 : percentage);

    const int total_half_cells = CPB_PROGRESS_BAR_DEFAULT_WIDTH * 2;
    const int filled_half_cells = (int)(clamped / 100.0 * total_half_cells);
    const int full_cells = filled_half_cells / 2;
    const bool has_left_half_cell = filled_half_cells % 2 > 0;
    const int empty_cells = CPB_PROGRESS_BAR_DEFAULT_WIDTH - full_cells;
    const bool has_right_half_cell = !has_left_half_cell && empty_cells > 0;

    const char *fill_color = progress_bar->is_finished
                                 ? utf8_codes.color_fill_after_ended
                                 : utf8_codes.color_fill;

    // Spinner
    if (utf8_codes.spinner_animation_length > 0)
    {
        const int spinner_index =
            progress_bar->internal.updates_count % utf8_codes.spinner_animation_length;
        fputs(utf8_codes.color_spinner, stdout);
        fputs(utf8_codes.spinner[spinner_index], stdout);
        fputs(utf8_codes.reset, stdout);
        fputs(" ", stdout);
    }

    // Description
    const char *description = progress_bar->config.description;
    if (description[0] != '\0')
    {
        fputs(description, stdout);
        fputs(" ", stdout);
    }

    // Filled cells
    if (filled_half_cells > 0)
    {
        fputs(fill_color, stdout);
        for (int i = 0; i < full_cells; i++)
        {
            fputs(utf8_codes.bar_fill, stdout);
        }

        if (has_left_half_cell)
        {
            fputs(utf8_codes.bar_fill_head, stdout);
        }
        fputs(utf8_codes.reset, stdout);
    }

    // Unfilled cells
    if (empty_cells > 0)
    {
        fputs(utf8_codes.color_empty, stdout);

        if (has_right_half_cell)
        {
            fputs(utf8_codes.bar_empty_head, stdout);
        }

        int i = (has_left_half_cell || has_right_half_cell) ? 1 : 0;
        for (; i < empty_cells; i++)
        {
            fputs(utf8_codes.bar_empty, stdout);
        }
    }
    fputs(utf8_codes.reset, stdout);

    // Extra Info
    printf(
        " %s%3d%%%s %s ",
        utf8_codes.color_percentage,
        (int)clamped,
        utf8_codes.reset,
        utf8_codes.separator
    );
    fputs(utf8_codes.color_elapsed_time, stdout);
    print_elapsed_time(progress_bar);
    fputs(utf8_codes.reset, stdout);
    fputs(" ", stdout);
    fputs(utf8_codes.separator, stdout);
    fputs(" ", stdout);
    fputs(utf8_codes.color_remaining_time, stdout);
    print_remaining_time(progress_bar);
    fputs(utf8_codes.reset, stdout);

    // Reset cursor
    if (progress_bar->is_finished)
    {
        fputs("\033[?25h\n", stdout);
    }

    fflush(stdout);
}
