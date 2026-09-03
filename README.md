# C Progress Bar
A simple progress bar library for C.
![Example Image (C Progress Bar)](.github/images/example.png)

## Features
* Colorful progress bar
* Remaining time estimation
* Elapsed time tracking
* Works with MSVC, Clang and GCC
* Written in C99 with minimal dependencies

## Limitations
* The progress bar only prints when you call update
* If you print anything when the progress bar is running, it may be
    - overwritten by the progress bar if there is no `\n`
    - appended to the end if there is `\n`
![Example Image (C Progress Bar)](.github/images/example2.png)
* On Windows, only the simple `no-color` version will be displayed. However, it is possible to fix by using wide characters (Help needed)

## Compiling Examples
To compile the examples, we use cmake:
```
git clone https://github.com/c-modules/c_progress_bar
cd c_progress_bar
cmake -B build -S . -DBUILD_EXAMPLES=ON
cmake --build build
```
Run the example by
```
./build/examples/example
```

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
    config.description = "Processing";                // Default: ""
    config.min_refresh_time = 0.1;                    // Minimum refresh time in seconds. Default: 0.1.
    config.timer_remaining_time_recent_weight = 0.3;  // Weight for recent rate in remaining time estimation. Range: [0, 1]. Default: 0.3.

    // You don't need to modify anything for CPB_ProgressBar.
    // Just call cpb_init
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
