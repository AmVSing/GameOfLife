/*
    Handles generating and updating grid
*/

#ifndef SIMULATION_H
#define SIMULATION_H

#include "types_and_constants.h"

Cell *malloc_grid(int width, int height);
void free_grid(Cell *grid);
void seed_random(unsigned int seed);

void empty_grid(Cell *grid, int width, int height);
// initialises grid to all 0s (empty clels)

void random_grid(Cell *grid, int width, int height);

void update_grid(Cell *grid, int width, int height, unsigned long *generations,
                 GridType type, RuleSet rules);
// updates a rectangular grid based on previous state
// now also takes in rules

#endif
