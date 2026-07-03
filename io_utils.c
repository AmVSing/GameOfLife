#include "io_utils.h"
#include <stdlib.h>
#include <string.h>

void print_grid(const Cell *grid, int width, int height,
                unsigned long generations, FILE *out) {
    fprintf(out, "\033[H\033[J");
    fprintf(out, "Generation: %lu\n", generations);
    for (int x = 0; x < height; x++) {
        for (int y = 0; y < width; y++) {
            if (grid[(x * width) + y].status == ALIVE) {
                fprintf(out, "#");
            } else {
                fprintf(out, " ");
            }
        }
        fprintf(out, "\n");
    }
}

static bool valid_dimensions(int width, int height) {
    return width > 0 && height > 0;
}

static int read_next_cell_char(FILE *in) {
    // read next character until newline.
    int ch;
    for (ch = fgetc(in); ch == '\n' || ch == '\r'; ch = fgetc(in)) {
    }
    // added '\r' for windows compatability
    return ch;
}
static const char *write_type(GridType type) {
    return (type == RECT) ? RECT_TEXT : HEX_TEXT;
}
static bool parse_type(const char *in, GridType *type) {
    if (strcmp(in, RECT_TEXT) == 0) {
        *type = RECT;
        return true;
    } else if (strcmp(in, HEX_TEXT) == 0) {
        *type = HEX;
        return true;
    }
    return false;
}

bool export_grid(FILE *out, const Cell *grid, int width, int height,
                 GridType type) {
    // check valid input
    if (out == NULL || grid == NULL || !valid_dimensions(width, height)) {
        return false;
    }

    if (fprintf(out, "%s %d %d\n", write_type(type), width, height) <
        0) { // check valid digits
        return false;
    }

    for (int x = 0; x < height; x++) {
        for (int y = 0; y < width; y++) {
            const Cell *cell = &grid[(x * width) + y]; // get current cell*
            if (fputc(cell->status == ALIVE ? '#' : '.', out) == EOF) {
                // EOF on fputc means writing error
                return false;
            }
        }
        if (fputc('\n', out) == EOF) {
            // EOF on fputc means writing error
            return false;
        }
    }

    return !ferror(out);
}

Cell *import_grid(FILE *in, int *width, int *height, GridType *type) {
    // prevent dereferencing null
    if (in == NULL || width == NULL || height == NULL || type == NULL) {
        return NULL;
    }

    char type_text[8];
    // validation
    if (fscanf(in, "%7s %d %d", type_text, width, height) != 3 ||
        !parse_type(type_text, type) || !valid_dimensions(*width, *height)) {
        return NULL;
    }
    // calloc grid (starts as all empty)
    Cell *grid = calloc((size_t)(*width) * (size_t)(*height), sizeof(*grid));

    if (grid == NULL) {
        perror("failed to malloc for grid size\n");
        return NULL;
    }

    for (int x = 0; x < *height; x++) {
        for (int y = 0; y < *width; y++) {
            int ch = read_next_cell_char(in);

            if (ch == '#') {
                grid[(x * (*width)) + y].status = ALIVE;
            } else if (ch == '.') { // not technically necessary, just in case
                                    // we change DEAD
                grid[(x * (*width)) + y].status = DEAD;
            } else { // invalid grid characters
                free(grid);
                return NULL;
            }
        }
    }

    return grid;
}

bool save_grid(const char *path, const Cell *grid, int width, int height,
               GridType type) {
    // check path is not null
    if (path == NULL) {
        return false;
    }

    FILE *out = fopen(path, "w");
    // make sure file opened/created correctly

    if (out == NULL) {
        return false;
    }

    bool exported = export_grid(out, grid, width, height, type);
    bool closed = fclose(out) != EOF;
    return exported && closed;
}

Cell *load_grid(const char *path, int *width, int *height, GridType *type) {
    // check path not null
    if (path == NULL) {
        return NULL;
    }

    FILE *in = fopen(path, "r");
    // check file opened correctly
    if (in == NULL) {
        return NULL;
    }

    Cell *grid = import_grid(in, width, height, type);
    fclose(in);
    return grid; // CAN RETURN NULL ON FAILED import_grid
}
