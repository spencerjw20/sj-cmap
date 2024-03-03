#ifndef __SJCMAP__
#define __SJCMAP__

#include <stdlib.h>

typedef struct sjcmap_t
{
	unsigned int key_size, value_size;
	void **data;
} *sjcmap;

// Create a new map with the given key and value types
// key_type: The type of the key
// value_type: The type of the value
// Returns: A new map (must be freed with sjcmap_free)
#define sjcmap_create(key_type, value_type) sjcmap_create_size(sizeof(key_type), sizeof(value_type))

// Create a new map with the given key and value sizes
// key_size: The size of the key
// value_size: The size of the value
// Returns: A new map (must be freed with sjcmap_free)
sjcmap sjcmap_create_size(unsigned int key_size, unsigned int value_size);

// Find a value in the map
// cmap: The map to search
// key: The key to search for
// Returns: The pointer to value if found, otherwise NULL
#define sjcmap_find(cmap, key) sjcmap_find_ptr(cmap, &key)

// Find a value in the map
// cmap: The map to search
// key: The key pointer to search for
// Returns: The pointer to value if found, otherwise NULL
void *sjcmap_find_ptr(sjcmap cm, const void *key);

// Set a value in the map
// cmap: The map to set the value in
// key: The key to set
// value: The value to set
// Returns: The pointer to the value
#define sjcmap_set(cmap, key, value) sjcmap_set_ptr(cmap, &key, &value)

// Set a value in the map
// cmap: The map to set the value in
// key: The key pointer to set
// value: The value pointer to set
// Returns: The pointer to the value
void *sjcmap_set_ptr(sjcmap cm, const void *key, void *value);

// Free the map
// cmap: The map to free
void sjcmap_free(sjcmap cm);

#endif