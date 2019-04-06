#include <stdio.h>
#include <stdlib.h>		/* srand(), rand() */
#include <string.h>

/* sleep(), getopt() */
#include <unistd.h>

/* time() */
#include <time.h>

/* unicode support */
#include <locale.h>
#include <wchar.h>

#include "csv.h"

#define MAX_CSV_SIZE 1024
#define MAX_QA_SIZE 512

#define CARDS "cards.csv"

char	       *filename = CARDS;

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

	return (0);
}

int		stop_iterating;

/* hardcore */
void
hash_table_iterate_over(struct Cards *ht, void (*f) (struct Card *))
{
	stop_iterating = 0;
	for (int i = 0; i < ht->size; i++) {
		struct Card    *kv = ht->buckets[i].head;
		while (kv != NULL) {
			f(kv);
			if (stop_iterating == 1)
				goto fail;
			kv = kv->next;
		}
	}

fail:
	return;
}

/* accessed in main() and select_next_card() */
static struct Card *next_card;

void
select_next_card(struct Card *card)
{

	if (next_card == NULL
	    || (card->priority > next_card->priority
		&& card->last_appearance <= next_card->last_appearance
		&& (rand() >= RAND_MAX / 3))) {	/* 2/3 chances of being
						 * selected */
		next_card = card;
	}

	return;
}

void
dump_card(struct Card *card)
{
	printf("%ls\t%ls", card->question, card->answer);

	return;
}

void
dump_csv(struct Card *card)
{
	FILE	       *fp = fopen(filename, "a");
	if (fp == NULL) {
		fprintf(stderr, "Could not open %s in append mode!\n", filename);
		usleep(500000);
		stop_iterating = 1;
		return;
	}

	fprintf(fp, "%ls,", card->question);

	wchar_t	       *dup = card->answer;
	while (*dup != '\n') {	/* newline terminated string */
		fputwc(*dup, fp);
		dup++;
	}

	fprintf(fp, ",%u\n", card->priority);

	fclose(fp);

	return;
}

#define ht_init		hash_table_init
#define ht_insert	hash_table_insert
#define ht_destroy	hash_table_destroy
#define ht_iterate	hash_table_iterate_over

int
main(int argc, char *argv[])
{
	(void)setlocale(LC_ALL, "");

	srand(time(NULL));

	struct Cards   *cards = malloc(sizeof(struct Cards));
	if (cards == NULL
	    || ht_init(cards, 100) != 0) {
		fprintf(stderr, "Failed to allocate memory!\n");
		exit(1);
	}

	wchar_t	       *line = malloc(sizeof(wchar_t) * MAX_QA_SIZE);
	if (line == NULL) {
		ht_destroy(cards);
		free(cards);
		fprintf(stderr, "Failed to allocate memory!\n");
		exit(1);
	}


	struct {
		unsigned int	show_answer;
		unsigned int	dump_cards;
	}		flags;
	flags.show_answer = 0;	/* default */
	flags.dump_cards = 0;

	char		c;
	while ((c = getopt(argc, argv, "f:vLh?")) != -1) {
		switch (c) {
		case 'f':
			filename = optarg;
			break;
		case 'v':	/* enable verbose mode: answer are shown on
				 * failure */
			flags.show_answer = 1;
			break;
		case 'L':	/* print cards and exit */
			flags.dump_cards = 1;
			break;
		case 'h':
		case '?':
			printf("usage: %s [-Lvh]\n", argv[0]);
			goto end;
			break;
		}
	}


	FILE	       *fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "Could not open %s in read mode!\n", filename);
		goto end;
	}


	char		csv_line[MAX_CSV_SIZE];

	while (fgets(csv_line, MAX_CSV_SIZE, fp) != NULL) {

#define o(k,v, p, l) \
	ht_insert(cards, k, v, p, l)

		char	      **items = parse_csv(csv_line);
		if (!items[0] || !items[1] || !items[2])
			continue;

		wchar_t		q[MAX_QA_SIZE], a[MAX_QA_SIZE];
		swprintf(q, MAX_QA_SIZE, L"%hs", items[0]);
		swprintf(a, MAX_QA_SIZE, L"%hs\n", items[1]);

		unsigned int	p = strtol(items[2], NULL, 10);
		o(q, a, p == 0 ? 1000 : p, 0);

		free_csv_line(items);
#undef o

	}

	fclose(fp);

	if (cards->population == 0) {
		printf("No cards!\n");
		goto end;
	}

	if (flags.dump_cards == 1) {
		ht_iterate(cards, dump_card);
		goto end;
	}


	int		success = 0;
	int		i = 1;

	ht_iterate(cards, select_next_card);
	printf("\e[1;1H\e[2J");
	printf("[%d/%d]", success, 0);
	printf("\t%ls\n", next_card->question);
	printf("? ");

	while (fgetws(line, MAX_QA_SIZE, stdin) != NULL) {

		printf("\e[1;1H\e[2J");	/* clear the screen */
		if (wcscmp(L"!save\n", line) == 0) {
			fp = fopen(filename, "w");
			if (fp == NULL) {
				fprintf(stderr, "Could not open %s in write mode!\n", filename);
				usleep(500000);
				i--;
				goto prompt;
			}
			fputs("", fp);
			fclose(fp);
			ht_iterate(cards, dump_csv);
			i--;	/* cancels i++ */
			goto prompt;	/* show same card again */
		} else if (wcscmp(next_card->answer, line) == 0) {
			printf("Nice!\n");
			next_card->priority--;
			success++;
		} else {
			printf("Wrong!\n");
			next_card->priority++;

			if (flags.show_answer == 1)
				printf("right answer: %ls\n", next_card->answer);
		}
		next_card->last_appearance = i;

		ht_iterate(cards, select_next_card);
		usleep(500000);

prompt:
		printf("\e[1;1H\e[2J");	/* clear the screen */
		printf("[%d/%d]", success, i);
		printf("\t%ls\n", next_card->question);
		printf("? ");

		i++;
	}

	putchar('\n');

end:
	ht_destroy(cards);
	free(cards);
	free(line);

	return (0);
}
