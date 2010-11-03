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
		char *token;
		bool gotName = false;
		while ((token = strsep(&(argv[arg]), " \t")) != NULL)
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
			char *internal_token;
			while((internal_token = strsep(&token, "=")) != NULL)
			{
				if (!gotName)
				{
					printf("Setting name to %s\n", internal_token);
					curr->name = internal_token;
					gotName = true;

				}
				else
				{
					printf("Setting option to %s\n", internal_token);
					curr->options = internal_token;
				}
			}
			prev = curr;
			curr = curr->next;
		}
	}

	curr = head;

	while(curr) {
		printf("%s support %s \n", curr->name, curr->options);
		curr = curr->next;
	}

	return 0;
}
