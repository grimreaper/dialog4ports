#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct list_el {
	char *name;
	char *options;
	struct list_el *next;
};

typedef struct list_el item;

int main(int argc, char* argv[])
{
	item *curr;
	item *head;
	item *prev;

//	head = (item *)malloc(sizeof(item));
	prev = NULL;
	head = NULL;
	curr = NULL;

	for (int arg=1; arg < argc; ++arg)
	{
		curr = (item *)malloc(sizeof(item));
		if (!head)
		{
			head = curr;
		}
		if (prev)
		{
			prev->next = curr;
		}

		bool gotName = false;
		char *internal_token;
		while((internal_token = strsep(&argv[arg], "=")) != NULL)
		{
			if (!gotName)
			{
				printf("Setting name to %s\n", internal_token);
				curr->name = internal_token;
				gotName = true;
			}
			else
			{
				printf("Setting %s's option to %s\n", curr->name, internal_token);
				curr->options = internal_token;
			}
		}
		prev = curr;
		curr = curr->next;
	}

	curr = head;

	while(curr) {
		if (strcmp(curr->options,"%") == 0)
		{
			printf("BOOLEAN %s\n", curr->name);
		}
		else if (strcmp(curr->options,"-") == 0)
		{
			printf("INPUT %s\n", curr->name);
		}
		else {
			printf("RADIO %s supports %s\n", curr->name, curr->options);
		}
		curr = curr->next;
	}

	return 0;
}
