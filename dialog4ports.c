#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <curses.h>
#include <menu.h>
#include <err.h>
#include <sysexits.h>


//TODO - radio options - binary and user input works!

static char *
getString(const char *curVal)
{
	const int bufSize = 80;
	char mesg[]="Choose a new value: ";
	char *str = calloc(bufSize,sizeof(char));

	int row,col;
//	clear();				/* clear the screen; use erase() instead? */
	getmaxyx(stdscr,row,col);		/* get the number of rows and columns */
	mvprintw(row/2,(col-strlen(mesg))/2,"%s",mesg);     /* print the message at the center of the screen */
	if (curVal != NULL)
	{
		mvprintw(row/2 + 1,(col-strlen(curVal))/2,"%s",curVal);     /* print the message at the center of the screen */
		move(row/2 + 2, (col-strlen(curVal))/2);
	}
	getnstr(str, bufSize -1);				/* request the input...*/
	clear();
	refresh();
	return str;
}

struct list_el {
	char *name;
	char *options;
	char *descr;
	char *value;		//this is user supplied
	struct list_el *next;
};



typedef struct list_el OptionEl;

int 
main(int argc, char* argv[])
{
	//create the linked list that I work with later

	/* culot: better use sys/queue.h instead of your own linked list implementation.
	   It will be easier to maintain, see queue(3). */
	OptionEl *curr = NULL;
	OptionEl *head = NULL;
	OptionEl *prev = NULL;

	unsigned int numElements = 0;
	/* Avoid C++ style declaration inside loops */
	int arg;

	for (arg=1; arg < argc; ++arg)
	{
		++numElements;
		curr = malloc (sizeof *curr);
		if (curr == NULL)
		{
			errx(EX_OSERR,"can not malloc");
		}
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
				printf("\n");
			}
		}
		curr->value = NULL;
		prev = curr;
		curr = curr->next;
	}


	//deal with curses
	curr = head;

	/* culot: avoid declaration not at the beginning of a block for portability */
	ITEM **option_items;
	MENU *option_menu;
	WINDOW *option_menu_win;
	unsigned int n_choices = numElements;

	initscr();
	cbreak();
//	noecho();
	keypad(stdscr, TRUE);

	option_items=(ITEM**)calloc(n_choices + 1, sizeof(ITEM *));

	curr = head;
	unsigned int count = 0;
	while(curr) {
		option_items[count] = new_item(curr->name, curr->descr);
		set_item_userptr(option_items[count], curr);
		count++;
		curr = curr->next;
	}

	option_items[n_choices] = (ITEM *)NULL;


	option_menu = new_menu((ITEM **)option_items);

	keypad(option_menu_win, TRUE);
	set_menu_mark(option_menu, " * ");

	mvprintw(LINES - 2, 0, "F1 to Exit");
	menu_opts_off(option_menu,O_ONEVALUE);

	post_menu(option_menu);
	refresh();

	int c;
	while( (c = getch()) != KEY_F(1) )
	{
		switch(c)
		{
			case KEY_DOWN:
		      	menu_driver(option_menu, REQ_DOWN_ITEM);
				break;
			case KEY_UP:
				menu_driver(option_menu, REQ_UP_ITEM);
				break;
			case KEY_LEFT:
				menu_driver(option_menu, REQ_LEFT_ITEM);
				break;
			case KEY_RIGHT:
				menu_driver(option_menu, REQ_RIGHT_ITEM);
				break;
			case KEY_NPAGE:
				menu_driver(option_menu, REQ_SCR_DPAGE);
				break;
			case KEY_PPAGE:
				menu_driver(option_menu, REQ_SCR_UPAGE);
				break;
			case ' ':
			case 10:
			{
				ITEM *curItem = current_item(option_menu);
				OptionEl *p = (OptionEl*)item_userptr(curItem);
				if (item_opts(current_item(option_menu)) & O_SELECTABLE)
				{
					menu_driver(option_menu, REQ_TOGGLE_ITEM);
					p->value = NULL;
				}
				else
				{
					p->value = getString(p->value);
					refresh();
				}
			}
				break;

		}

		ITEM *cur;
		cur = current_item(option_menu);
		OptionEl *p = (OptionEl*)item_userptr(cur);

		if (p != NULL && strcmp("-",p->options) == 0)
		{
			item_opts_off(cur, O_SELECTABLE);
		}

	}
	unpost_menu(option_menu);
	endwin(); //get out of ncurses


	ITEM **items;
	items = menu_items(option_menu);
	int i;
	for(i = 0; i < item_count(option_menu); ++i)
	{
//		cur = current_item(option_menu);
		OptionEl *p = (OptionEl*)item_userptr(items[i]);

		if (strcmp("%",p->options) == 0)
		{
			const char* val = (item_value(items[i]) == TRUE) ? "true" : "false";
			fprintf(stderr,"%s=%s\n", item_name(items[i]), val);
		}
		else if (strcmp("-", p->options) == 0)
		{
			if (p->value)
			{
				fprintf(stderr,"%s=%s\n", item_name(items[i]), p->value);
			}
			else
			{
				fprintf(stderr,"%s=\n", item_name(items[i]));
			}
		}
		else
		{
			const char* val = (item_value(items[i]) == TRUE) ? "true" : "false";
			fprintf(stderr,"%s=%s  #radio\n", item_name(items[i]), val);
		}
	}


	for (count = 0; count < n_choices; ++count)
	{
		free_item(option_items[count]);
	}

	free_menu(option_menu);

	return 0;
}

