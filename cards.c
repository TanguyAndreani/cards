#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* sleep() */
#include <unistd.h>

/* srand() */
#include <time.h>

/* unicode support */
#include <locale.h>
#include <wchar.h>

struct Card {			/* a single card (a bucket's node) */

	wchar_t	       *question;	/* key */
	wchar_t	       *answer;
	unsigned int	priority;
	unsigned int	last_appearance;

	struct Card    *next;
};

struct Bucket {			/* buckets for the hash table */
	unsigned int	size;
	struct Card    *head;
};

struct Cards {			/* a hash table */
	unsigned int	size;
	unsigned int	population;
	struct Bucket  *buckets;
};

unsigned int
hash_key(const struct Cards *ht, const wchar_t * key)
{
	/* sdbm */
	unsigned int	hashAddress = 0;
	for (int i = 0; key[i] != L'\0'; i++) {
		hashAddress = key[i] + (hashAddress << 6) + (hashAddress << 16) - hashAddress;
	}
	return hashAddress % ht->size;
}

int
hash_table_init(struct Cards *ht, unsigned int size)
{
	ht->size = size;
	ht->population = 0;
	ht->buckets = malloc(sizeof(struct Bucket) * ht->size);
	if (ht->buckets == NULL)
		return (1);
	for (unsigned int i = 0; i < ht->size; i++) {
		ht->buckets[i].size = 0;
		ht->buckets[i].head = NULL;
	}
	return (0);
}

void
hash_table_destroy(struct Cards *ht)
{
	for (unsigned int i = 0; i < ht->size; i++) {

		struct Card    *kv;
		while ((kv = ht->buckets[i].head) != NULL) {
			free((void *)kv->question);
			free((void *)kv->answer);
			ht->buckets[i].head = kv->next;
			ht->buckets[i].size--;
			free(kv);
		}

	}

	free(ht->buckets);

	return;
}

int
hash_table_insert(struct Cards *ht, const wchar_t * question,
		  const wchar_t * answer, unsigned int priority,
		  unsigned int last_appearance)
{
	int		hashed_key = hash_key(ht, question);

	struct Card    *kv = malloc(sizeof(struct Card));
	if (kv == NULL)
		return (1);

	kv->question = wcsdup(question);
	if (kv->question == NULL)
		return (1);

	kv->answer = wcsdup(answer);
	if (kv->answer == NULL)
		return (1);

	kv->priority = priority;
	kv->last_appearance = last_appearance;
	kv->next = ht->buckets[hashed_key].head;

	ht->population++;
	ht->buckets[hashed_key].head = kv;
	ht->buckets[hashed_key].size++;

	/*if (ht->buckets[hashed_key].size > 1) {
		fprintf(stderr, "collision!\n");
		sleep(1);
	}*/

	return (0);
}

/* hardcore */
void
hash_table_iterate_over(struct Cards *ht, void (*f) (struct Card *))
{
	for (int i = 0; i < ht->size; i++) {
		struct Card    *kv = ht->buckets[i].head;
		while (kv != NULL) {
			f(kv);
			kv = kv->next;
		}
	}

	return;
}

/* accessed in main() and select_next_card() */
static struct Card *next_card;

void
select_next_card(struct Card *card)
{

	if (next_card == NULL
	    || card->priority > next_card->priority
	    || card->last_appearance < next_card->last_appearance) {
		if (rand() >= RAND_MAX / 3)	/* 2/3 chances of being
						 * selected */
			next_card = card;
	}

	return;
}

#define ht_init		hash_table_init
#define ht_insert	hash_table_insert
#define ht_destroy	hash_table_destroy
#define ht_iterate	hash_table_iterate_over

int
main()
{
	(void)setlocale(LC_ALL, "");

	srand(time(NULL));

	struct Cards   *cards = malloc(sizeof(struct Cards));
	if (cards == NULL
	    || ht_init(cards, 100) != 0) {
		fprintf(stderr, "Failed to allocate memory!\n");
		exit(1);
	}

	wchar_t	       *line = malloc(sizeof(wchar_t) * 1024);
	if (line == NULL) {
		ht_destroy(cards);
		free(cards);
		fprintf(stderr, "Failed to allocate memory!\n");
		exit(1);
	}

#define o(k,v) \
	ht_insert(cards, k, v, 1000, 0)	/* or 1 */

	o(L"あ", L"a\n");
	o(L"い", L"i\n");
	o(L"う", L"u\n");
	o(L"え", L"e\n");
	o(L"お", L"o\n");
	o(L"か", L"ka\n");
	o(L"き", L"ki\n");
	o(L"く", L"ku\n");
	o(L"け", L"ke\n");
	o(L"こ", L"ko\n");

#undef o

	int success = 0;
	int		i = 1;

	ht_iterate(cards, select_next_card);
	printf("\e[1;1H\e[2J");
	printf("[%d/%d]", success, 0);
	printf("\t%ls\n", next_card->question);
	printf("? ");

	while (fgetws(line, 1024, stdin) != NULL) {

		printf("\e[1;1H\e[2J"); /* clear the screen */
		if (wcscmp(next_card->answer, line) == 0) {
			printf("Nice!\n");
			next_card->priority--;
			success++;
		} else {
			printf("Wrong!\n");
			next_card->priority++;
		}
		next_card->last_appearance = i;

		ht_iterate(cards, select_next_card);
		usleep(500000);
		printf("\e[1;1H\e[2J"); /* clear the screen */
		printf("[%d/%d]", success, i);
		printf("\t%ls\n", next_card->question);
		printf("? ");

		i++;
	}

	putchar('\n');

	ht_destroy(cards);
	free(cards);
	free(line);

	return (0);
}
