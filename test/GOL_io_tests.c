#include "../io_utils.h"
#include "../simulation.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static bool test_export_grid(void) {
    printf("Testing export_grid\n");

    enum { width = 3, height = 2 };

    Cell grid[width * height] = {{ALIVE}, {DEAD},  {ALIVE},
                                 {DEAD},  {ALIVE}, {DEAD}};

    const char *expected = "rect 3 2\n#.#\n.#.\n"; // expected output

    FILE *tmp = tmpfile(); // temp file
    if (tmp == NULL) {     // check temp created
        printf("Test fail: couldn't make tmpfile\n");
        return false;
    }

    bool passed = export_grid(tmp, grid, width, height, RECT);
    if (!passed) { // check passed
        printf("Test fail: export_grid returned false\n");
    }

    rewind(tmp); // go back to start of tmp file

    // read chars from tmp
    char actual[64] = {0};
    size_t bytes_read = fread(actual, 1, sizeof(actual) - 1, tmp);
    actual[bytes_read] = '\0';

    if (strcmp(actual, expected) != 0) { // check they're the same
        printf("Test fail: exported text is:\n%sbut should be:\n%s", actual,
               expected);
        passed = false;
    }

    fclose(tmp);

    if (passed) {
        printf("Test passed!!!\n");
    }
    return passed;
}

static bool test_import_grid(void) {
    printf("Testing import_grid\n");

    const char *text = "rect 3 2\n#.#\n.#.\n";
    enum { expected_width = 3, expected_height = 2 };

    Cell expected_grid[expected_width * expected_height] = {
        {ALIVE}, {DEAD}, {ALIVE}, {DEAD}, {ALIVE}, {DEAD}};

    FILE *tmp = tmpfile(); // create tmp file
    if (tmp == NULL) {     // check tmpfile created
        printf("Test fail: couldn't make tmpfile\n");
        return false;
    }

    fputs(text, tmp); // write expected output to file
    rewind(tmp);      // start at start of file

    int width = 0;
    int height = 0;
    GridType type = HEX;
    Cell *grid = import_grid(tmp, &width, &height, &type);
    bool passed = true;

    if (grid == NULL) {
        printf("Test fail: import_grid returned NULL\n");
        passed = false;
    } else {
        if (width != expected_width) {
            printf("Test fail: width is %d but should be %d\n", width,
                   expected_width);
            passed = false;
        }
        if (height != expected_height) {
            printf("Test fail: height is %d but should be %d\n", height,
                   expected_height);
            passed = false;
        }
        if (type != RECT) {
            printf("Test fail: type is %d but should be %d\n", type, RECT);
            passed = false;
        }
        passed &= test_grid_matches(grid, expected_grid, width, height);
        // check that the imported matches expected
    }

    free(grid); // NEEDED TO PREVENT MEMORY LEAK
    fclose(tmp);

    if (passed) {
        printf("Test passed!!!\n");
    }
    return passed;
}

static bool test_import_rejects_invalid_input(void) {
    printf("Testing invalid input file to import\n");

    const char *text = "rect 3 2\n#x#\n.#.\n";
    FILE *tmp = tmpfile();
    if (tmp == NULL) {
        printf("Test fail: couldn't make tmpfile\n");
        return false;
    }

    fputs(text, tmp); // write text to output
    rewind(tmp);      // start at start

    int width = 0;
    int height = 0;
    GridType type = RECT;
    Cell *grid = import_grid(tmp, &width, &height, &type);

    bool passed = true;
    if (grid != NULL) { // should be null
        printf("Test fail, import_grid accepted invalid input\n");
        free(grid);
        passed = false;
    }

    fclose(tmp);

    if (passed) {
        printf("Test passed!!!\n");
    }
    return passed;
}

static bool test_save_and_load_grid(void) {
    printf("Testing save_grid\n");

    enum { width = 4, height = 3 };
    const char *path = "gameoflife_saved_grid.txt";
    Cell grid[width * height];
    Cell expected[width * height];

    empty_grid(grid, width, height);
    empty_grid(expected, width, height);

    // test grids
    grid[1].status = ALIVE;
    grid[width + 2].status = ALIVE;
    grid[2 * width].status = ALIVE;

    // expected should match saved version
    expected[1].status = ALIVE;
    expected[width + 2].status = ALIVE;
    expected[2 * width].status = ALIVE;

    bool passed = save_grid(path, grid, width, height, RECT);
    if (!passed) {
        printf("Test fail: save_grid returned false\n");
        return false;
    }

    int loaded_width = 0;
    int loaded_height = 0;
    GridType loaded_type = HEX;
    Cell *loaded = load_grid(path, &loaded_width, &loaded_height, &loaded_type);
    // load the grid we just saved

    if (loaded == NULL) { // should not be null
        printf("Test fail: load_grid returned NULL\n");
        remove(path);
        return false;
    }

    if (loaded_width != width) {
        printf("Test fail: width is %d, expected %d\n", loaded_width, width);
        passed = false;
    }
    if (loaded_height != height) {
        printf("Test fail: height is %d, expected %d\n", loaded_height, height);
        passed = false;
    }
    if (loaded_type != RECT) {
        printf("Test fail: type is %d, expected %d\n", loaded_type, RECT);
        passed = false;
    }
    passed &= test_grid_matches(loaded, expected, width, height);

    free(loaded);
    remove(path);

    if (passed) {
        printf("Test passed!!!\n");
    }
    return passed;
}

int main(void) {
    // pass for all tests
    bool passed = true;
    passed &= test_export_grid();
    passed &= test_import_grid();
    passed &= test_import_rejects_invalid_input();
    passed &= test_save_and_load_grid();

    if (passed) {
        printf("\nAll tests passed!!\n");
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
