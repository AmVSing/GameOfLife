#include "simulation.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// cell generation function typedef
typedef cell_status (*cell_init_f)(int x, int y, void *state);

// random cell generation function
static cell_status random_cell(int x, int y, void *state) {
    (void)x;
    (void)y;
    (void)state; // do nothing with x, y and state
    return (cell_status)(rand() % 2);
}
// 0 cell generation function
static cell_status empty_cell(int x, int y, void *state) {
    (void)x;
    (void)y;
    (void)state; // do nothing with x, y and state
    return DEAD;
}

static void initialise_any_grid(Cell *grid, int width, int height,
                                cell_init_f init, void *state) {
    // initialises any type of grid, given an initialisation function
    for (int x = 0; x < height; x++) {
        for (int y = 0; y < width; y++) {
            grid[(x * width) + y].status = init(x, y, state);
        }
    }
}

static int count_rect_neighbours(const Cell *grid,      // PtoM (grid)
                                 const Cell *prev_row,  // PtoM (row)
                                 const Cell *curr_row,  // PtoM (row)
                                 const Cell *first_row, // PtoM (row)
                                 int width, int height, int x, int y) {
    // counts the number of neighbours for the cell at coordinates (x,y)
    // check in bounds
    assert(x >= 0 && x < height);
    assert(y >= 0 && y < width);

    int count = 0;

    for (int i = -1; i <= 1; i++) {     // do every adjacent x pos
        for (int j = -1; j <= 1; j++) { // do every adjacent y pos
            if (i == 0 && j == 0)
                continue; // skip this pos
            int neighbour_y = (y + j + width) % width;

            if (i == -1) {
                count += (prev_row[neighbour_y].status == ALIVE);
            } else if (i == 0) {
                count += (curr_row[neighbour_y].status == ALIVE);
            } else {
                int neighbour_x = (x + i + height) % height;
                if (neighbour_x == 0) {
                    count += (first_row[neighbour_y].status == ALIVE);
                } else {
                    count +=
                        (grid[(neighbour_x * width) + neighbour_y].status ==
                         ALIVE);
                }
            }
        }
    }
    return count;
}

static int count_neighbours_hex(Cell *grid,     // PtoM grid
                                Cell *prev_row, // PtoM row
                                Cell *curr_row, // PtoM row
                                Cell *first_row, int width, int height, int x,
                                int y) {
    int count = 0;
    int left = (y - 1 + width) % width; // left neighbour
    int right = (y + 1) % width;        // right neighbour

    int upper_l_y, upper_r_y, lower_l_y, lower_r_y; // 4 other neighbours

    // x for below row
    int lower_x = (x + 1) % height;
    Cell *below_row = (lower_x == 0) ? first_row : &grid[lower_x * width];

    // y pos for neighbours:
    if ((x & 1) == 0) {
        upper_l_y = left;
        upper_r_y = y;
        lower_l_y = left;
        lower_r_y = y;
    } else {
        upper_l_y = y;
        upper_r_y = right;
        lower_l_y = y;
        lower_r_y = right;
    }
    // this row
    count += (curr_row[left].status == ALIVE);
    count += (curr_row[right].status == ALIVE);

    // above row
    count += (prev_row[upper_l_y].status == ALIVE);
    count += (prev_row[upper_r_y].status == ALIVE);

    // below row
    count += (below_row[lower_l_y].status == ALIVE);
    count += (below_row[lower_r_y].status == ALIVE);
    return count;
}

Cell *malloc_grid(int width, int height) {
    Cell *grid = malloc((size_t)width * (size_t)height * sizeof(*grid));

    if (grid == NULL) {
        fprintf(stderr, "Error: failed to allocate grid\n");
        exit(EXIT_FAILURE);
    }
    return grid;
}

void free_grid(Cell *grid) { free(grid); }

void seed_random(unsigned int seed) { srand(seed); }

void empty_grid(Cell *grid, int width, int height) {
    initialise_any_grid(grid, width, height, empty_cell, NULL);
}

void random_grid(Cell *grid, int width, int height) {
    initialise_any_grid(grid, width, height, random_cell, NULL);
}

static void update_rect_grid(Cell *grid, int width, int height,
                             unsigned long *generations, RuleSet rules) {
    /* because we are updating grid in place row by row, buffers are needed so
    we do not overwrite info we need to determine the next state of other rows,
    this method saves a lot of memory in comparison to using a second full grid.
    we are using 2 single-row buffers on the stack, dynamically adjust based on
    size of terminal width*/
    Cell prev_row[width];         // backup of row above
    Cell current_row_copy[width]; // backup of current row
    Cell first_row_copy[width];   // safety vault of first row for toroidal
    // cache warpping bottom row (height-1)
    // safety prepping row 0
    memcpy(first_row_copy, &grid[0], sizeof(Cell) * width);
    // toroidal wrapping prep as when processing Row 0, its neighbor is the very
    // bottom of the grid, and as it has not yet been modified we can take teh
    // current data

    memcpy(prev_row, &grid[(height - 1) * width], sizeof(Cell) * width);
    for (int x = 0; x < height; x++) {
        // backup the current row
        memcpy(current_row_copy, &grid[x * width], sizeof(Cell) * width);
        // loop through grid horizontally column by column
        for (int y = 0; y < width;
             y++) { // loop to count the number of adjacent alive cells
            int neighbours =
                count_rect_neighbours(grid, prev_row, current_row_copy,
                                      first_row_copy, width, height, x, y);

            int index = (x * width) + y;

            if (current_row_copy[y].status == ALIVE) {
                grid[index].status = (neighbours >= rules.survival_min &&
                                      neighbours <= rules.survival_max)
                                         ? ALIVE
                                         : DEAD;
            } else {
                grid[index].status =
                    (neighbours == rules.birth_count) ? ALIVE : DEAD;
            }
        }
        memcpy(prev_row, current_row_copy, sizeof(Cell) * width);
    }
    (*generations)++; // increment no gens.
}

static void update_hex_grid(Cell *grid, int width, int height,
                            unsigned long *generations, RuleSet rules) {

    Cell prev_row[width];
    Cell curr_row_copy[width];
    Cell first_row_copy[width]; // needed to wrap around from bottom

    // fill first row copy
    memcpy(first_row_copy, &grid[0], sizeof(Cell) * width);
    // set prev row to final row to begin with
    memcpy(prev_row, &grid[(height - 1) * width], sizeof(Cell) * width);

    for (int x = 0; x < height; x++) {
        // fill in current row copy from grid
        memcpy(curr_row_copy, &grid[x * width], sizeof(Cell) * width);

        for (int y = 0; y < width; y++) {
            int neighbours =
                count_neighbours_hex(grid, prev_row, curr_row_copy,
                                     first_row_copy, width, height, x, y);
            int index = (x * width) + y; // flatenned index

            // update state for current cell
            if (curr_row_copy[y].status == ALIVE) {
                grid[index].status = (neighbours >= rules.survival_min &&
                                      neighbours <= rules.survival_max)
                                         ? ALIVE
                                         : DEAD;
            } else { // currently dead
                grid[index].status =
                    (neighbours == rules.birth_count) ? ALIVE : DEAD;
            }
        }
        memcpy(prev_row, curr_row_copy, sizeof(Cell) * width);
    }

    (*generations)++;
}

void update_grid(Cell *grid, int width, int height, unsigned long *generations,
                 GridType type, RuleSet rules) {
    if (type == RECT) {
        update_rect_grid(grid, width, height, generations, rules);
    } else {
        update_hex_grid(grid, width, height, generations, rules);
    }
}
