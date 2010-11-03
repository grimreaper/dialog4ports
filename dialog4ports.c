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

typedef struct list_el OptionEl;

int main(int argc, char* argv[])
{

	//create the linked list that I work with later
	OptionEl *curr = NULL;
	OptionEl *head = NULL;
	OptionEl *prev = NULL;

	for (int arg=1; arg < argc; ++arg)
	{
		curr = (OptionEl *)malloc(sizeof(OptionEl));
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



	unsigned int numElements = 0;

	//print out my list
	curr = head;

	while(curr) {
		++numElements;
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


	//deal with curses
	curr = head;

	ITEM **option_items;
	MENU *option_menu;
	unsigned int n_choices = numElements;

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	option_items=(ITEM**)calloc(n_choices + 1, sizeof(ITEM *));

	curr = head;
	unsigned int count = 0;
	while(curr) {
		option_items[count++] = new_item(curr->name, curr->descr);
		curr = curr->next;
	}
	option_items[n_choices] = (ITEM *)NULL;

	option_menu = new_menu((ITEM **)option_items);

	mvprintw(LINES - 2, 0, "F1 to Exit");
	post_menu(option_menu);
	refresh();

	int c;
	while(  (c = getch()) != KEY_F(1)  )
	{
		switch(c)
		{
			case KEY_DOWN:
		      	menu_driver(option_menu, REQ_DOWN_ITEM);
				break;
			case KEY_UP:
				menu_driver(option_menu, REQ_UP_ITEM);
				break;
		}
	}

	for (count = 0; count < n_choices; ++count)
	{
		free_item(option_items[count]);
	}


	free_menu(option_menu);
	endwin();				/* close the ncurses window */

	return 0;
}

