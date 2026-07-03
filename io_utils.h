/*
    handles taking in input for initial board state
    and printing grid to outa
*/
#ifndef GAMEUTILS_H
#define GAMEUTILS_H
#include "types_and_constants.h"
#include <stdbool.h>
#include <stdio.h>

void print_grid(const Cell *grid, int width, int height,
                unsigned long generations, FILE *out);
// prints the current state of the grid to out

bool export_grid(FILE *out, const Cell *grid, int width, int height,
                 GridType type);
// export grid to file, true iff successful

Cell *import_grid(FILE *in, int *width, int *height, GridType *type);
// import grid from file returns NULL on fail

bool save_grid(const char *path, const Cell *grid, int width, int height,
               GridType type);
// save grid to a specific path, true iff successful

Cell *load_grid(const char *path, int *width, int *height, GridType *type);
// load grid from a specific path, returns NULL on fail
#endif
