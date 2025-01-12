#pragma once

#include <stdlib.h>
#include <string.h>

struct dynaList_s {
    void* data;
    size_t element_size;
    size_t size;
};

typedef struct dynaList_s* DynaList;

void DListInit(DynaList* list, size_t element_size);

void DListAppend(DynaList list, const void* element);

void* DListAppendNew(DynaList list);

void* DListGet(DynaList list, size_t index);

void DListFree(DynaList list);