
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define BUFFER_SIZE 50

typedef struct StringMap {
    int elements;
    int size;
    struct StringMapItem** stringItem;
} StringMap;

// data structure stored in the StringMap
typedef struct StringMapItem {
    char* key;
    void* item;
} StringMapItem;

// Allocate, initialise and return a new, empty StringMap
StringMap* stringmap_init(void) {
    struct StringMap* stringMap = malloc(sizeof(struct StringMap));
    stringMap->elements = 0;
    stringMap->size = BUFFER_SIZE;
    stringMap->stringItem = malloc(sizeof(struct StringMapItem*)
            * BUFFER_SIZE);
    for (int i = 0; i < stringMap->size; i++) {
        stringMap->stringItem[i] = malloc(sizeof(struct StringMapItem));
        stringMap->stringItem[i]->key = malloc(sizeof(char) * BUFFER_SIZE);
    }
    return stringMap;
}

// Free all memory associated with a StringMap.
// frees stored key strings but does not free() the (void *)item pointers
// in each StringMapItem. Does nothing if sm is NULL.
void stringmap_free(StringMap* sm) {
    if (sm != NULL) {
        for (int i = 0; i < sm->size; i++) {

            free(sm->stringItem[i]->key);
	    free(sm->stringItem[i]);
        }
        free(sm->stringItem);
        free(sm);

    }
}

// Search a stringmap for a given key, returning a pointer to the entry
// if found, else NULL. If not found or sm is NULL or key is NULL then
// returns NULL.
void* stringmap_search(StringMap* sm, char* key) {

    if (sm == NULL || key == NULL) {
        return ((void*)NULL);
    }
    for (int i = 0; i < sm->elements; i++) {
        if (strcmp(sm->stringItem[i]->key, key) == 0) {
	    return ((void*)sm->stringItem[i]->item);
        }
    }
    return ((void*)NULL);
}

// Add an item into the stringmap, return 1 if success else 0 (e.g. an item
// with that key is already present or any one of the arguments is NULL)
// The 'key' string is copied before being stored in the stringmap.
// The item pointer is stored as-is, no attempt is made to copy its contents.
int stringmap_add(StringMap* sm, char* key, void* item){
    if (sm->size == sm->elements + 1) {
        sm->size = sm->size + BUFFER_SIZE;
        sm->stringItem = realloc(sm->stringItem, sizeof(StringMapItem*)
                * sm->size);
        for (int i = sm->elements + 1; i < sm->size; i++) {
            sm->stringItem[i] = malloc(sizeof(StringMapItem));
            sm->stringItem[i]->key = malloc(sizeof(char) * BUFFER_SIZE);
        }
    } 
    if (sm->elements == 0) {
        strcpy(sm->stringItem[sm->elements]->key, key);
	sm->stringItem[sm->elements]->item = item; 
        sm->elements++;
        return 1;
    }
    for (int j = 0; j < sm->elements; j++) {
        if (strcmp(sm->stringItem[j]->key, key) == 0) {
            return 0;
        }
    }
    strcpy(sm->stringItem[sm->elements]->key, key);
    sm->stringItem[sm->elements]->item = item;
    sm->elements++;

    return 1;



}

// Removes an entry from a stringmap
// free()stringMapItem and the copied key string, but not
// the item pointer.
// Returns 1 if success else 0 (e.g. item not present or any argument is NULL)
int stringmap_remove(StringMap* sm, char* key) {

    if (sm == NULL || key == NULL) {
        return 0;
    }
    if (sm->elements == 0) {
	return 0;
    }
    for (int i = 0; i < sm->elements; i++) {
        if (strcmp(sm->stringItem[i]->key, key) == 0) {
      	    strcpy(sm->stringItem[i]->key, "NOT_AVAILABLE_ANYMORE");
	    return 1;
        }
    }
    return 0;
  
}

// Iterate through the stringmap - if prev is NULL then the first entry
// is returned otherwise prev should be a value returned from a previous call
// to stringmap_iterate()and the "next" entry will be returned.
// This operation is not thread-safe - any changes to the stringmap
// between successive calls to stringmap_iterate may result
// in undefined behaviour.
// Returns NULL if no more items to examine or sm is NULL.
// There is no expectation that items are returned in a particular order (i.e.
// the order does not have to be the same order in which items were added).
StringMapItem* stringmap_iterate(StringMap* sm, StringMapItem* prev) {
    if (sm == NULL) {
	return NULL;
    }
    if (sm->elements == 0) {
	return NULL;
    }
    if (prev == NULL) {
	return sm->stringItem[0];
    }
    for (int i = 0; i < sm->elements; i++) {
        if (strcmp(prev->key, sm->stringItem[i]->key) == 0) {
            if (i == sm->elements - 1) {

	        return NULL;
            } else {
                return sm->stringItem[i + 1];
    
            }


        }


    }
    return NULL;

}
