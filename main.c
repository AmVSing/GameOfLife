#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h> // for terminal size info
#include <time.h>
#include <unistd.h>

#include "io_utils.h"
#include "simulation.h"

#define MAX_GENS 1000
int main(void) {
    struct winsize w_size;
    unsigned long generations = 0;
    RuleSet rules = {
        .birth_count = RECT_DEFAULT_BIRTH,
        .survival_min = RECT_DEFAULT_SURVIVAL_MIN,
        .survival_max = RECT_DEFAULT_SURVIVAL_MAX,
    };

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w_size) == -1) { // cant get size
        fprintf(stderr, "Error: could not determine terminal size\n");
        return EXIT_FAILURE;
    }

    int height = w_size.ws_row - 2;
    int width = w_size.ws_col;
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Error: terminal too small\n");
        return EXIT_FAILURE;
    }

    Cell *grid = malloc_grid(width, height);
    random_grid(grid, width, height);

    for (int i = 0; i < MAX_GENS; i++) {
        print_grid(grid, width, height, generations, stdout);
        usleep(SLEEP_TIME * 1000000);
        update_grid(grid, width, height, &generations, RECT, rules);
    }

    free(grid);
    return EXIT_SUCCESS;
}
