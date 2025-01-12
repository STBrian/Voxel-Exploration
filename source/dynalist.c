#include "dynalist.h"

void DListInit(DynaList* list, size_t element_size)
{
    *list = malloc(sizeof(struct dynaList_s));
    if (list == NULL)
        return;
    (*list)->data = NULL;
    (*list)->element_size = element_size;
    (*list)->size = 0;
}

void DListAppend(DynaList list, const void* element)
{
    size_t new_size = list->size + 1;
    if (list->size == 0)
        list->data = malloc(new_size * list->element_size);
    else
        list->data = realloc(list->data, new_size * list->element_size);

    void* destination = list->data + (list->size * list->element_size);
    memcpy(destination, element, list->element_size);
    list->size++;
}

void* DListAppendNew(DynaList list)
{
    size_t new_size = list->size + 1;
    if (list->size == 0)
        list->data = malloc(new_size * list->element_size);
    else
        list->data = realloc(list->data, new_size * list->element_size);

    void* destination = list->data + (list->size * list->element_size);
    list->size++;
    return destination;
}

void* DListGet(DynaList list, size_t index)
{
    if (index >= list->size) {
        return NULL;
    }
    return list->data + (index * list->element_size);
}

void DListFree(DynaList list)
{
    free(list->data);
    free(list);
}
