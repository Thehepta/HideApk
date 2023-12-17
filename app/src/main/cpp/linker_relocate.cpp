//
// Created by thehepta on 2023/12/17.
//




#include "linker_relocate.h"

struct linker_stats_t {
    int count[kRelocMax];
};



static linker_stats_t linker_stats;

void count_relocation(RelocationKind kind) {
    ++linker_stats.count[kind];
}
