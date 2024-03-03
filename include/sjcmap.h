/*

  _________     ____._________     _____      _____ __________ 
 /   _____/    |    |\_   ___ \   /     \    /  _  \\______   \
 \_____  \     |    |/    \  \/  /  \ /  \  /  /_\  \|     ___/
 /        \/\__|    |\     \____/    Y    \/    |    \    |    
/_______  /\________| \______  /\____|__  /\____|__  /____|    
        \/                   \/         \/         \/          


  Welcome to SJCMAP - A Thread-Safe C Map Library
  
  Description:
  SJCMAP is a C library that provides a flexible and thread-safe implementation of a generic map data structure.
  It allows users to create maps with customizable key and value types, as well as control the mod size for optimizing
  performance and memory usage. The library supports essential operations like creating, finding, setting values,
  and freeing the map.

  Thread Safety:
  The library is designed with thread safety in mind, ensuring safe concurrent access to the map. This is achieved
  by using locking mechanisms internally, allowing multiple threads to work with the map simultaneously without
  risking data corruption.

*/

#ifndef __SJCMAP__
#define __SJCMAP__

#include <stdlib.h>

#define SJCMAP_ENABLE_THREAD_SAFETY 1

typedef void *sjcmap;

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

// Create a new map with the given key and value types and mod size
// key_type: The type of the key
// value_type: The type of the value
// mod_size: Mod size specifies the number of buckets lists in the map more increases speed but uses more memory on create
#define sjcmap_create_ex(key_type, value_type, mod_size) sjcmap_create_size_ex(sizeof(key_type), sizeof(value_type), mod_size)

// Create a new map with the given key and value sizes and mod size
// key_size: The size of the key
// value_size: The size of the value
// mod_size: Mod size specifies the number of buckets lists in the map more increases speed but uses more memory on create
sjcmap sjcmap_create_size_ex(unsigned int key_size, unsigned int value_size, unsigned int mod_size);

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
#define sjcmap_set(cmap, key, value) sjcmap_set_ptr(cmap, &key, &value)

// Set a value in the map
// cmap: The map to set the value in
// key: The key pointer to set
// value: The value pointer to set
void sjcmap_set_ptr(sjcmap cm, const void *key, void *value);

// Free the map
// cmap: The map to free
// Returns: count of elements freed in the map
unsigned int sjcmap_free(sjcmap cm);

#endif
