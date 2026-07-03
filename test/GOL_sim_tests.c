#include "../simulation.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// rule sets for rectangle and hexagonal boards
static const RuleSet RECT_RULES = {
    RECT_DEFAULT_BIRTH,
    RECT_DEFAULT_SURVIVAL_MIN,
    RECT_DEFAULT_SURVIVAL_MAX,
};

static const RuleSet HEX_RULES = {
    HEX_DEFAULT_BIRTH,
    HEX_DEFAULT_SURVIVAL_MIN,
    HEX_DEFAULT_SURVIVAL_MAX,
};

static bool test_grid_matches(const Cell *actual, const Cell *expected,
                              int width, int height) {
    bool passed = true;
    int cells = height * width;
    for (int i = 0; i < cells; i++) {
        if (actual[i].status != expected[i].status) {
            passed = false;
            printf("Test fail, cell at flattened index %d is %d but should be "
                   "%d\n",
                   i, actual[i].status, expected[i].status);
        }
    }
    return passed;
}

static bool test_generation_updates(void) {
    // check no. of generations increments on update

    // grid setup
    enum { width = 4, height = 4 };
    Cell grid[width * height];
    empty_grid(grid, width, height);
    // gens
    unsigned long generations = 0;
    // update twice
    update_grid(grid, width, height, &generations, RECT, RECT_RULES);
    update_grid(grid, width, height, &generations, RECT, RECT_RULES);

    return (generations == 2);
}

static bool test_empty_grid(void) {
    printf("Testing empty_grid\n");

    // create actual and expected
    enum { width = 4, height = 3 };
    Cell grid[width * height];
    Cell expected[width * height];

    int cells = width * height;
    for (int i = 0; i < cells; i++) {
        grid[i].status = ALIVE; // have grid with all alive cells to start
        expected[i].status = DEAD;
    }

    // empty grid should empty all the alive cells
    empty_grid(grid, width, height);
    return test_grid_matches(grid, expected, width, height);
}

static bool test_single_cell_dies(void) {
    printf("Testing single cell dies in update_rect_grid\n");

    // create actual and expected
    enum { width = 5, height = 5 };
    Cell grid[width * height];
    Cell expected[width * height];

    empty_grid(grid, width, height);
    empty_grid(expected, width, height);

    // set a cell to alive
    grid[width + 2].status = ALIVE;

    unsigned long generations = 0;

    update_grid(grid, width, height, &generations, RECT,
                RECT_RULES); // should die due to underpop

    return test_grid_matches(grid, expected, width, height);
}

static bool test_single_hex_cell_dies(void) {
    // testing hex grid with only one cell
    printf("Testing single cell dies in hex mode\n");

    enum { width = 5, height = 5 };
    Cell grid[width * height];
    Cell expected[width * height];

    empty_grid(grid, width, height);
    empty_grid(expected, width, height);

    grid[width + 2].status = ALIVE;

    unsigned long generations = 0;
    update_grid(grid, width, height, &generations, HEX, HEX_RULES);

    return test_grid_matches(grid, expected, width, height);
}

static bool test_block_is_stable(void) {
    printf("Testing 2x2 block\n");
    // should stay the same

    // set up actual nad expected
    enum { width = 5, height = 5 };
    Cell grid[width * height];
    Cell expected[width * height];

    empty_grid(grid, width, height);
    empty_grid(expected, width, height);

    // block
    grid[width + 1].status = ALIVE;
    grid[width + 2].status = ALIVE;
    grid[(2 * width) + 1].status = ALIVE;
    grid[(2 * width) + 2].status = ALIVE;

    // block
    expected[(1 * width) + 1].status = ALIVE;
    expected[(1 * width) + 2].status = ALIVE;
    expected[(2 * width) + 1].status = ALIVE;
    expected[(2 * width) + 2].status = ALIVE;

    unsigned long generations = 0;
    update_grid(grid, width, height, &generations, RECT, RECT_RULES);
    return test_grid_matches(grid, expected, width, height);
}

static bool test_blinker_oscillates(void) {

    printf("Testing blinker oscillator\n");
    // horizontal then vertical

    // set up actual and expected

    enum { width = 5, height = 5 };
    Cell grid[width * height];
    Cell expected_after_one[width * height];
    Cell expected_after_two[width * height];

    empty_grid(grid, width, height);
    empty_grid(expected_after_one, width, height);
    empty_grid(expected_after_two, width, height);

    grid[(2 * width) + 1].status = ALIVE;
    grid[(2 * width) + 2].status = ALIVE;
    grid[(2 * width) + 3].status = ALIVE;

    expected_after_one[(1 * width) + 2].status = ALIVE;
    expected_after_one[(2 * width) + 2].status = ALIVE;
    expected_after_one[(3 * width) + 2].status = ALIVE;

    expected_after_two[(2 * width) + 1].status = ALIVE;
    expected_after_two[(2 * width) + 2].status = ALIVE;
    expected_after_two[(2 * width) + 3].status = ALIVE;

    unsigned long generations = 0;
    update_grid(grid, width, height, &generations, RECT, RECT_RULES);

    bool passed = test_grid_matches(grid, expected_after_one, width, height);
    update_grid(grid, width, height, &generations, RECT, RECT_RULES);

    passed &= test_grid_matches(grid, expected_after_two, width, height);
    return passed;
}

int main(void) {
    bool passed = true;
    passed &= test_empty_grid();
    passed &= test_single_cell_dies();
    passed &= test_single_hex_cell_dies();
    passed &= test_block_is_stable();
    passed &= test_blinker_oscillates();
    passed &= test_generation_updates();

    if (passed) {
        printf("\nAll tests passed!!\n");
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
