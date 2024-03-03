#include "sjcmap.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	sjcmap cm = sjcmap_create(int, int);

	int count = 500000;
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

	printf("%i elements set and found!\n", count);

	sjcmap_free(cm);

	return 0;
}
