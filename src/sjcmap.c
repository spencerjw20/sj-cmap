#include "sjcmap.h"

#define SJCMAP_DEFAULT_MOD_SIZE 0xFF
#define SJCMAP_EXTRA_BUCKET_SIZE 3

#if SJCMAP_ENABLE_THREAD_SAFETY
#include <intrin.h>
typedef unsigned long long mutex_t;
#define mutex_init(mutex_ptr) *mutex_ptr = 0
#define mutex_lock(mutex_ptr) while (_InterlockedExchange(mutex_ptr, 1) == 1);
#define mutex_unlock(mutex_ptr) _InterlockedExchange(mutex_ptr, 0);
#else
#define mutex_init(mutex_ptr)
#define mutex_lock(mutex_ptr)
#define mutex_unlock(mutex_ptr)
#endif

typedef struct sjcmap_t
{
	unsigned int key_size, value_size, mod_size;
	void **data;
#if SJCMAP_ENABLE_THREAD_SAFETY
	mutex_t mutex;
#endif
};

#define SJCMAP ((struct sjcmap_t*)cm)

static inline unsigned int __sjcmap_fnv1a_32(const void *key, unsigned int len)
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

struct sjcmap_link_header_t
{
	unsigned int bucket_count;
	struct sjcmap_link_header_t *next;
};

#define BUCKET_SIZE(cmap) (sizeof(unsigned int) + (cmap)->key_size + (cmap)->value_size)
#define BUCKET_HASH_PTR(bucket) (unsigned int*)(bucket)
#define BUCKET_KEY_PTR(bucket) ((unsigned char*)bucket + sizeof(unsigned int))
#define BUCKET_VALUE_PTR(bucket, cmap) ((unsigned char*)bucket + sizeof(unsigned int) + cmap->key_size)

sjcmap sjcmap_create_size_ex(unsigned int key_size, unsigned int value_size, unsigned int mod_size)
{
	struct sjcmap_t *cm = malloc(sizeof(struct sjcmap_t));
	if (!cm)
		return 0;

	cm->key_size = key_size;
	cm->value_size = value_size;
	cm->mod_size = mod_size;
	mutex_init(&cm->mutex);

	cm->data = malloc(cm->mod_size * sizeof(void *));
	memset(cm->data, 0, cm->mod_size * sizeof(void *));

	if (!cm)
	{
		free(cm);
		return 0;
	}
	return cm;
}

sjcmap sjcmap_create_size(unsigned int key_size, unsigned int value_size)
{
	return sjcmap_create_size_ex(key_size, value_size, SJCMAP_DEFAULT_MOD_SIZE);
}

unsigned int sjcmap_free(sjcmap cm)
{
	struct sjcmap_t *map = (struct sjcmap_t *)cm;
	unsigned int elements_freed = 0;

	for (unsigned int i = 0; i < SJCMAP->mod_size; i++)
	{
		struct sjcmap_link_header_t *first_link = SJCMAP->data[i];
		if (!first_link)
			continue;

		while (first_link->next)
		{
			struct sjcmap_link_header_t *prev = first_link;
			struct sjcmap_link_header_t *link = first_link->next;
			while (link->next)
			{
				prev = link;
				link = link->next;
			}

			elements_freed += link->bucket_count;
			free(link);
			prev->next = 0;
		}

		elements_freed += first_link->bucket_count;
		free(first_link);
	}
	free(SJCMAP->data);
	free(SJCMAP);

	return elements_freed;
}

static inline void **__private_sjcmap_bucket_array(sjcmap cm, unsigned int key_hash)
{
	return SJCMAP->data + (key_hash % SJCMAP->mod_size);
}

static inline void *__private_sjcmap_find_ptr(sjcmap cm, const void *key, unsigned int hash)
{
	void *buckets = *__private_sjcmap_bucket_array(SJCMAP, hash);
	if (!buckets)
		return 0;

	struct sjcmap_link_header_t *bucket_header = (struct sjcmap_link_header_t *)buckets;
	while (bucket_header)
	{
		for (unsigned int i = 0; i < bucket_header->bucket_count; i++)
		{
			void *bucket = ((unsigned char *)bucket_header) + sizeof(struct sjcmap_link_header_t) + (BUCKET_SIZE(SJCMAP) * i);
			unsigned int bucket_hash = *BUCKET_HASH_PTR(bucket);
			if (bucket_hash == hash)
			{
				void *bucket_key = BUCKET_KEY_PTR(bucket);
				if (memcmp(bucket_key, key, SJCMAP->key_size) == 0)
				{
					return BUCKET_VALUE_PTR(bucket, SJCMAP);
				}
			}
		}

		bucket_header = bucket_header->next;
	}

	return 0;
}

void *sjcmap_find_ptr(sjcmap cm, const void *key)
{
	unsigned int hash = __sjcmap_fnv1a_32(key, SJCMAP->key_size);
	return __private_sjcmap_find_ptr(SJCMAP, key, hash);
}

void sjcmap_set_ptr(sjcmap cm, const void *key, void *value)
{
	unsigned int hash = __sjcmap_fnv1a_32(key, SJCMAP->key_size);
	mutex_lock(&SJCMAP->mutex);
	void *found = __private_sjcmap_find_ptr(SJCMAP, key, hash);
	if (found)
	{
		memcpy(found, value, SJCMAP->value_size);
		mutex_unlock(&SJCMAP->mutex);
		return;
	}

	void **buckets = __private_sjcmap_bucket_array(SJCMAP, hash);

	if (!*buckets)
	{
		unsigned int bucket_total_size = sizeof(struct sjcmap_link_header_t) + (BUCKET_SIZE(SJCMAP) * (SJCMAP_EXTRA_BUCKET_SIZE + 1));
		void *first_link = malloc(bucket_total_size);
		if (!first_link)
		{
			mutex_unlock(&SJCMAP->mutex);
			return;
		}
		memset(first_link, 0, bucket_total_size);

		struct sjcmap_link_header_t *header = first_link;
		header->bucket_count = 1;
		void *bucket = ((unsigned char *)header) + sizeof(struct sjcmap_link_header_t);

		*BUCKET_HASH_PTR(bucket) = hash;
		memcpy(BUCKET_KEY_PTR(bucket), key, SJCMAP->key_size);
		memcpy(BUCKET_VALUE_PTR(bucket, SJCMAP), value, SJCMAP->value_size);

		*buckets = first_link;
		mutex_unlock(&SJCMAP->mutex);
		return;
	}

	struct sjcmap_link_header_t *header = ((struct sjcmap_link_header_t *)*buckets);
	do
	{
		if (header->next)
			header = header->next;
	} while (header->next);

	if (header->bucket_count >= SJCMAP_EXTRA_BUCKET_SIZE + 1)
	{
		unsigned int bucket_total_size = sizeof(struct sjcmap_link_header_t) + (BUCKET_SIZE(SJCMAP) * (SJCMAP_EXTRA_BUCKET_SIZE + 1));
		void *new_link = malloc(bucket_total_size);
		if (!new_link)
		{
			mutex_unlock(&SJCMAP->mutex);
			return;
		}

		memset(new_link, 0, bucket_total_size);

		struct sjcmap_link_header_t *new_header = new_link;
		new_header->bucket_count = 1;
		void *bucket = ((unsigned char *)new_header) + sizeof(struct sjcmap_link_header_t);

		*BUCKET_HASH_PTR(bucket) = hash;
		memcpy(BUCKET_KEY_PTR(bucket), key, SJCMAP->key_size);
		memcpy(BUCKET_VALUE_PTR(bucket, SJCMAP), value, SJCMAP->value_size);

		header->next = new_link;
		mutex_unlock(&SJCMAP->mutex);
		return;
	}

	void *bucket = ((unsigned char *)header) + sizeof(struct sjcmap_link_header_t) + (BUCKET_SIZE(SJCMAP) * header->bucket_count);

	*BUCKET_HASH_PTR(bucket) = hash;
	memcpy(BUCKET_KEY_PTR(bucket), key, SJCMAP->key_size);
	memcpy(BUCKET_VALUE_PTR(bucket, SJCMAP), value, SJCMAP->value_size);

	header->bucket_count++;

	mutex_unlock(&SJCMAP->mutex);
	return;
}

#undef mutex_init
#undef mutex_lock
#undef mutex_unlock

#undef SJCMAP_DEFAULT_MOD_SIZE
#undef SJCMAP_EXTRA_BUCKET_SIZE
#undef HASH
#undef BUCKET_SIZE
#undef BUCKET_HASH_PTR
#undef BUCKET_KEY_PTR
#undef BUCKET_VALUE_PTR