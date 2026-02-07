/**
 * \file math_utils.h
 * \brief Mathematical utility functions for C Progress Bar library.
 *
 * \author Ching-Yin Ng
 */

#ifndef C_PROGRESS_BAR_INTERNAL_MATH_UTILS_H
#define C_PROGRESS_BAR_INTERNAL_MATH_UTILS_H

#include <stdint.h>

#include "c_progress_bar.h"

/**
 * \brief Calculate the percentage of completion.
 *
 * \param[in] progress_bar Pointer to the progress bar structure.
 *
 * \return The percentage of completion.
 */
double calculate_percentage(const CPB_ProgressBar *restrict progress_bar);

/**
 * \brief Calculate the overall rate of progress (percentage per second).
 *
 * \param[in] progress_bar Pointer to the progress bar structure.
 *
 * \return The overall rate of progress.
 */
double calculate_overall_rate(const CPB_ProgressBar *restrict progress_bar);

/**
 * \brief Calculate the recent rate of progress (percentage per second) based on recent
 * updates.
 *
 * \param[in] progress_bar Pointer to the progress bar structure.
 *
 * \return The recent rate of progress.
 */
double calculate_recent_rate(const CPB_ProgressBar *restrict progress_bar);

#endif /* C_PROGRESS_BAR_INTERNAL_MATH_UTILS_H */
