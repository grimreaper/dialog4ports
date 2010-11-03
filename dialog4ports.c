#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <curses.h>
#include <menu.h>

struct list_el {
	char *name;
	char *options;
	char *descr;
	char *value;		//this is user supplied
	struct list_el *next;
};

typedef struct list_el item;

int main(int argc, char* argv[])
{

	//create the linked list that I work with later
	item *curr = NULL;
	item *head = NULL;
	item *prev = NULL;

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
		bool gotDescr = false;
		char *internal_token;
		while((internal_token = strsep(&argv[arg], "=")) != NULL)
		{
			if (!gotName)
			{
				printf("Setting name to %s\n", internal_token);
				curr->name = internal_token;
				gotName = true;
			}
			else if (!gotDescr)
			{
				printf("Setting %s's descr to %s\n", curr->name, internal_token);
				curr->descr = internal_token;
				gotDescr = true;
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


	//print out my list
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


	//check return codes

	clear();				/* clear the screen; use erase() instead? */
	initscr();				/* start the curses mode */


	int row,col;				//rerun this on SIGWINCH
	getmaxyx(stdscr,row,col);		/* get the number of rows and columns */


	endwin();				/* close the ncurses window */

	return 0;
}

