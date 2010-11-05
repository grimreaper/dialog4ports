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
getString(const char * const curVal)
{
	const int bufSize = 80;
	char mesg[]="Choose a new value: ";
	char *str = calloc(bufSize,sizeof(char));

	int row,col;
//	clear();				/* clear the screen; use erase() instead? */
	getmaxyx(stdscr,row,col);		/* get the number of rows and columns */
	mvprintw(row/2,(col-(int)strlen(mesg))/2,"%s",mesg);     /* print the message at the center of the screen */
	if (curVal != NULL)
	{
		mvprintw(row/2 + 1,(col-(int)strlen(curVal))/2,"%s",curVal);     /* print the message at the center of the screen */
		move(row/2 + 2, (col-(int)strlen(curVal))/2);
	}
	getnstr(str, bufSize -1);				/* request the input...*/
	clear();
	refresh();
	return str;
}

static unsigned int
countChar ( const char * const input, const char c )
{
	unsigned int retval = 0;
	const size_t len = strlen(input);
	size_t i;
	for (i=0; i < len; ++i)
	{
		if (input[i] == c)
		{
			++retval;
		}
	}
	return retval;
}


enum OPTION_TYPE {
	CHECKBOX,
	RADIOBOX,
	USER_INPUT,
};

struct list_el {
	char *name;
	char *options;
	char *descr;
	char *value;		//this is user supplied
	enum OPTION_TYPE mode;
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
	OptionEl *prev = NULL, *next = NULL;

	ITEM **option_items;
	MENU *option_menu;
	WINDOW *option_menu_win;



	unsigned int numElements = 0;
	unsigned int n_choices = 0;
	unsigned int hashMarks = 0;

	if (argc < 2)
	{
		errx(EX_USAGE,"We require some option type to work");
	}


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
				if (strcmp("%",curr->options) == 0)
				{
					curr->mode = CHECKBOX;
				}
				else if (strcmp("-", curr->options) == 0)
				{
					curr->mode = USER_INPUT;
				}
				else
				{
					// is curr->options nul terminated?
					printf("%d", strlen(curr->options));
					hashMarks += countChar(internal_token,'#');
					curr->mode = RADIOBOX;
				}
			}
		}
		curr->value = NULL;
		prev = curr;
		curr = curr->next;
	}


	//deal with curses
	curr = head;

	n_choices = numElements + hashMarks;
	printf("%d", n_choices + hashMarks);
	//getchar();

	initscr();
	start_color();
	cbreak();
//	noecho();
	keypad(stdscr, TRUE);
	init_pair(1, COLOR_GREEN, COLOR_BLACK);     //selected
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);	//selectable
	init_pair(3, COLOR_RED, COLOR_BLACK); //disabled

	option_items=(ITEM**)calloc(n_choices + 1, sizeof(ITEM *));

	curr = head;
	unsigned int count = 0;
	while(curr) {
		if (curr->mode != RADIOBOX)
		{
			option_items[count] = new_item(curr->name, curr->descr);
			set_item_userptr(option_items[count], curr);
			count++;
		}
		else
		{
			char *tmpToken;
			char *tmpOption = calloc(strlen(curr->options)+1,sizeof(char));
			strncpy(tmpOption,curr->options,strlen(curr->options));

			while((tmpToken = strsep(&tmpOption, "#")) != NULL)
			{
	                  option_items[count] = new_item(tmpToken, curr->descr);
	                  set_item_userptr(option_items[count], curr);
				count++;
			}
		}
		curr = curr->next;
	}

	option_items[n_choices] = (ITEM *)NULL;


	option_menu = new_menu((ITEM **)option_items);

	/* Set fore ground and back ground of the menu */
	set_menu_fore(option_menu, COLOR_PAIR(1) | A_REVERSE);
	set_menu_back(option_menu, COLOR_PAIR(2));
	set_menu_grey(option_menu, COLOR_PAIR(3));



	keypad(option_menu_win, TRUE);
	set_menu_mark(option_menu, " * ");

	mvprintw(LINES - 2, 0, "F1 to Exit");
	menu_opts_off(option_menu,O_ONEVALUE);

	post_menu(option_menu);
	refresh();

	int c;
	while( (c = getch()) != KEY_F(1) )
	{
		ITEM *curItem = current_item(option_menu);

		OptionEl *p = (OptionEl*)item_userptr(curItem);

		if (p != NULL && strcmp("-",p->options) == 0)
		{
			item_opts_off(curItem, O_SELECTABLE);
		}

		if (p != NULL && p->mode == RADIOBOX &&p->value != NULL)
		{
			if (p->value != item_name(curItem))
			{
				item_opts_off(curItem, O_SELECTABLE);
			}
		}
		else
		{
			item_opts_on(curItem, O_SELECTABLE);
		}

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
				if (item_opts(curItem) & O_SELECTABLE)
				{
					if (p->mode != USER_INPUT)
					{
						menu_driver(option_menu, REQ_TOGGLE_ITEM);
						if(item_value(curItem) == TRUE)
						{
							//item_name returns a const ptr
							const char const* curName = item_name(curItem);
							p->value = calloc(strlen(curName), sizeof(char)); //LEAKS
							strcpy(p->value, curName);
						}
						else
						{
							p->value = NULL;
						}
					}
					else
					{
						p->value = getString(p->value);
						if (p->value != NULL && strcmp("",p->value) != 0)
						{
							menu_driver(option_menu, REQ_TOGGLE_ITEM);
						}
						refresh();
					}
				}
				break;
			}
		}
	}
	unpost_menu(option_menu);
	endwin(); //get out of ncurses


	ITEM **items;
	items = menu_items(option_menu);
	int i;
	for(i = 0; i < item_count(option_menu); ++i)
	{
		OptionEl *p = (OptionEl*)item_userptr(items[i]);

		if (p->mode == CHECKBOX)
		{
			const char* val = (item_value(items[i]) == TRUE) ? "true" : "false";
			fprintf(stderr,"%s=%s\n", item_name(items[i]), val);
		}
		else if (p->mode == USER_INPUT)
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
		else //p->mode == RADIOBOX
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

	curr = head;
	while (curr)
	{
		next = curr->next;
		free(curr);
		curr = next;
	}


	return 0;
}

