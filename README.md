# SJCMAP - Thread-Safe C Map Library

SJCMAP is a lightweight and thread-safe C library for handling generic maps. This library allows you to create, manage, and manipulate maps with customizable key and value types. It is designed with thread safety in mind, ensuring secure concurrent access to the map data structure.

## Table of Contents
- [Installation](#installation)
- [Usage](#usage)
- [Examples](#examples)
- [Thread Safety](#thread-safety)
- [Contributing](#contributing)
- [License](#license)

## Installation

To use SJCMAP in your project, simply include the `sjcmap.h` header file in your source code and link against the compiled library.

```c
#include "sjcmap.h"
```

## Usage

### Creating a Map

To create a new map, use the `sjcmap_create` macro, specifying the key and value types.

```c
sjcmap cm = sjcmap_create(int, int);
```

### Setting and Finding Values

```c
int count = 5000;
for (int i = 0; i < count; i++)
{
    int negative = -i;
    sjcmap_set(cm, i, negative);
}

for (int i = 0; i < count; i++)
{
    int *out = sjcmap_find(cm, i);

    if (!out)
        printf("Error not found! %i\n", i);

    if (out && *out != -i)
        printf("Error! %i\n", i);
}
```

### Freeing the Map

Don't forget to free the allocated memory once you're done using the map.

```c
sjcmap_free(cm);
```

## Examples

Here is a more detailed example illustrating the creation and usage of SJCMAP:

```c
#include "sjcmap.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    // Create a map with int keys and int values
    sjcmap cm = sjcmap_create(int, int);

    int count = 5000;

    // Set values in the map
    for (int i = 0; i < count; i++)
    {
        int negative = -i;
        sjcmap_set(cm, i, negative);
    }

    // Find and validate values in the map
    for (int i = 0; i < count; i++)
    {
        int *out = sjcmap_find(cm, i);

        if (!out)
            printf("Error not found! %i\n", i);

        if (out && *out != -i)
            printf("Error! %i\n", i);
    }

    printf("%i elements set and found!\n", count);

    // Free the map to release memory
    sjcmap_free(cm);

    return 0;
}
```

## Thread Safety

SJCMAP is designed with thread safety in mind, allowing multiple threads to safely access and manipulate the map concurrently. The library internally uses locking mechanisms to prevent data corruption during concurrent operations.

## Contributing

Contributions to SJCMAP are welcome! Feel free to open issues for bug reports, feature requests, or submit pull requests. Please follow the existing code style and provide tests for new features.

## License

SJCMAP is licensed under the [Apache License](LICENSE).
