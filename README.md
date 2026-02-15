# C Progress Bar
A colorful, lightweight progress bar library for C.
![Example Image (C Progress Bar)](.github/images/example.png)

## Features
* Beautiful progress bar
* Remaining time estimation
* Elapsed time tracking
* Fast and Thread-safe
* Works with MSVC, Clang and GCC
* Written in C99 with minimal dependencies

Note that this is a simple progress bar implementation
* The progress bar only prints when you call update (as we don't have multi-threading)


## Sample Usage
```c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "c_progress_bar.h"

#define N 1000000000

int main(void)
{
    double sum = 0.0;

    // Setup progress bar
    CPB_Config config = cpb_get_default_config();
    config.description = "Processing";
    CPB_ProgressBar progress_bar;
    cpb_init(&progress_bar, 0, N, config);

    // Main loop
    cpb_start(&progress_bar);
    for (int64_t i = 0; i <= N; i++)
    {
        if (i % 1000 == 0)
        {
            cpb_update(&progress_bar, i);
        }

        sum += (i % 100) * 0.0001;
    }
    cpb_finish(&progress_bar);

    printf("Final result: %f\n", sum);

    return 0;
}

```