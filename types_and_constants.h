/*
    Common types
*/
#ifndef TYPES_AND_CONSTANTS_H
#define TYPES_AND_CONSTANTS_H

#define RECT_DEFAULT_SURVIVAL_MIN 2
#define RECT_DEFAULT_SURVIVAL_MAX 3
#define RECT_DEFAULT_BIRTH 3

#define HEX_DEFAULT_SURVIVAL_MIN 3
#define HEX_DEFAULT_SURVIVAL_MAX 4
#define HEX_DEFAULT_BIRTH 2

#define ALIVE 1 // status representing alive
#define DEAD 0  // status representing dead

#define SLEEP_TIME 0.1 // sleep time in seconds

#define RECT_TEXT "rect"
#define HEX_TEXT "hex"

// removed 1 bit field, since memory is byte addressable,
// so each struct takes at least 1 byte
typedef unsigned char cell_status;

typedef enum {
    RECT,
    HEX
} GridType; // whether to treat as rectangular or hexagonal.

typedef struct {
    cell_status status;
} Cell;

// ruleset struct for custom rules
typedef struct {
    int birth_count;
    int survival_min;
    int survival_max;
} RuleSet;

#endif
