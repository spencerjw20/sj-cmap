#include "sjcmap.h"

#define SJCMAP_MOD_SIZE 0xFF
#define SJCMAP_EXTRA_BUCKET_SIZE 3

unsigned int __sjcmap_fnv1a_32(const void *key, unsigned int len)
{
	const unsigned int prime = 0x01000193; // 16777619
	unsigned int hash = 0x811C9DC5; // 2166136261
	const unsigned char *ptr = key;
	while (len--)
	{
		hash = (*ptr++ ^ hash) * prime;
	}
	return hash;
}

struct sjcmap_bucket_header_t
{
	unsigned bucket_count, max_bucket_count;
};

#define HASH(key, size) __sjcmap_fnv1a_32(key, sizeof(key))
#define BUCKET_SIZE(cmap) (sizeof(unsigned int) + (cmap)->key_size + (cmap)->value_size)
#define BUCKET_HASH_PTR(bucket) (unsigned int*)(bucket)
#define BUCKET_KEY_PTR(bucket) ((unsigned char*)bucket + sizeof(unsigned int))
#define BUCKET_VALUE_PTR(bucket, cmap) ((unsigned char*)bucket + sizeof(unsigned int) + cmap->key_size)

sjcmap sjcmap_create_size(unsigned int key_size, unsigned int value_size)
{
	struct sjcmap_t *cm = malloc(sizeof(struct sjcmap_t));
	if (!cm)
		return 0;

	cm->key_size = key_size;
	cm->value_size = value_size;

	cm->data = malloc(SJCMAP_MOD_SIZE * sizeof(void *));
	memset(cm->data, 0, SJCMAP_MOD_SIZE * sizeof(void *));

	if (!cm)
	{
		free(cm);
		return 0;
	}
	return cm;
}

void sjcmap_free(sjcmap cm)
{
	for (unsigned int i = 0; i < SJCMAP_MOD_SIZE; i++)
	{
		void *buckets = cm->data[i];
		if (buckets)
		{
			free(buckets);
		}
	}
	free(cm->data);
	free(cm);
}

void **__private_sjcmap_bucket_array(sjcmap cm, unsigned int key_hash)
{
	int mod = (key_hash % SJCMAP_MOD_SIZE);
	void **buckets = cm->data + (key_hash % SJCMAP_MOD_SIZE);
	return buckets;
}

void *sjcmap_find_ptr(sjcmap cm, const void *key)
{
	unsigned int hash = HASH(key, cm->key_size);
	void *buckets = *__private_sjcmap_bucket_array(cm, hash);
	if (!buckets)
		return 0;

	struct sjcmap_bucket_header_t *bucket_header = (struct sjcmap_bucket_header_t *)buckets;
	for (unsigned int i = 0; i < bucket_header->bucket_count; i++)
	{
		void *bucket = ((unsigned char *)buckets) + sizeof(struct sjcmap_bucket_header_t) + (BUCKET_SIZE(cm) * i);
		unsigned int bucket_hash = *BUCKET_HASH_PTR(bucket);
		if (bucket_hash == hash)
		{
			void *bucket_key = BUCKET_KEY_PTR(bucket);
			if (memcmp(bucket_key, key, cm->key_size) == 0)
			{
				return BUCKET_VALUE_PTR(bucket, cm);
			}
		}
	}

	return 0;
}

void *sjcmap_set_ptr(sjcmap cm, const void *key, void *value)
{
	void *found = sjcmap_find_ptr(cm, key);
	if (found)
	{
		memcpy(found, value, cm->value_size);
		return found;
	}

	unsigned int hash = HASH(key, cm->key_size);
	int mod = (hash % SJCMAP_MOD_SIZE);
	void **buckets = __private_sjcmap_bucket_array(cm, hash);

	void *bucket = 0;
	if (!*buckets)
	{
		unsigned int bucket_total_size = sizeof(struct sjcmap_bucket_header_t) + BUCKET_SIZE(cm) * (SJCMAP_EXTRA_BUCKET_SIZE + 1);
		*buckets = malloc(bucket_total_size);
		memset(*buckets, 0, bucket_total_size);
		struct sjcmap_bucket_header_t *header = ((struct sjcmap_bucket_header_t *)*buckets);
		header->bucket_count = 1;
		header->max_bucket_count = SJCMAP_EXTRA_BUCKET_SIZE + 1;
		bucket = ((unsigned char *)header) + sizeof(struct sjcmap_bucket_header_t);
	}
	else
	{
		struct sjcmap_bucket_header_t *header = ((struct sjcmap_bucket_header_t *)*buckets);
		if (header->bucket_count == header->max_bucket_count)
		{
			*buckets = realloc(*buckets, sizeof(struct sjcmap_bucket_header_t) + BUCKET_SIZE(cm) * (header->max_bucket_count + SJCMAP_EXTRA_BUCKET_SIZE));
			header = ((struct sjcmap_bucket_header_t *)*buckets);
			memset(((unsigned char *)*buckets) + sizeof(struct sjcmap_bucket_header_t) + BUCKET_SIZE(cm) * header->max_bucket_count, 0, BUCKET_SIZE(cm) * SJCMAP_EXTRA_BUCKET_SIZE);
			header->max_bucket_count += SJCMAP_EXTRA_BUCKET_SIZE;
		}

		bucket = ((unsigned char *)header) + sizeof(struct sjcmap_bucket_header_t) + (BUCKET_SIZE(cm) * header->bucket_count);
		header->bucket_count++;
	}

	*BUCKET_HASH_PTR(bucket) = hash;
	memcpy(BUCKET_KEY_PTR(bucket), key, cm->key_size);
	memcpy(BUCKET_VALUE_PTR(bucket, cm), value, cm->value_size);

	return bucket;
}

#undef SJCMAP_MOD_SIZE
#undef SJCMAP_EXTRA_BUCKET_SIZE
#undef HASH
#undef BUCKET_SIZE
#undef BUCKET_HASH_PTR
#undef BUCKET_KEY_PTR
#undef BUCKET_VALUE_PTR