#pragma once

#include <pebble.h>

#define Distance 0
#define Location 1
#define Cases 2
#define Deaths 3
#define Recovered 4
#define Scope 5
#define Ready 6
#define RequestUpdate 20

typedef struct {
    char location[24];
    char cases[24];
    char deaths[24];
    char recovered[24];
} LocationStats;