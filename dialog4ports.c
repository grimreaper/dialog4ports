#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <curses.h>
#include <menu.h>
#include <err.h>
#include <sysexits.h>


//TODO - handle sigwinch - resize winsows
//TODO - replecate OTPIONS's looks and feel (the triple window approach)
//TODO - default values (from the envrioment - re dougb)

//BUG - getString() clears the border

static char *
getString(WINDOW *win, const char * const curVal)
{
	const int bufSize = 80;
	char mesg[]="Choose a new value: ";
	char *str = calloc(bufSize,sizeof(char));

	int row,col;
	getmaxyx(win,row,col);		/* get the number of rows and columns */
	mvwprintw(win,row/2,(col-(int)strlen(mesg))/2,"%s",mesg);     /* print the message at the center of the screen */
	if (curVal != NULL)
	{
		mvwprintw(win,row/2 + 1,(col-(int)strlen(curVal))/2,"%s",curVal);     /* print the message at the center of the screen */
		wmove(win,row/2 + 2, (col-(int)strlen(curVal))/2);
	}
	wgetnstr(win,str, bufSize -1);				/* request the input...*/
	wclear(win);
	wrefresh(win);
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
	const char *value;		//this is user supplied
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
	WINDOW *title_menu_win;



	unsigned int numElements = 0;
	unsigned int n_choices = 0;
	unsigned int hashMarks = 0;

	if (argc < 2)
	{
		errx(EX_USAGE,"We require some option type to work");
	}
	if (argc < 3)
	{
		errx(EX_USAGE,"We need more than just a port name");
	}


	/* Avoid C++ style declaration inside loops */
	int arg;

	const char * const portName = argv[1];


	// arg=0 program name
	// arg=1 port title
	for (arg=2; arg < argc; ++arg)
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
				curr->name = internal_token;
				gotName = true;
			}
			else if (!gotDescr)
			{
				curr->descr = internal_token;
				gotDescr = true;
			}
			else
			{
				curr->options = internal_token;
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
	// we want to leave 3 lines for the title
	const int startMenyWinRow = 3;

	const int nlines= getmaxy(stdscr) - startMenyWinRow;
	const int ncols= getmaxx(stdscr);

	const int nMenuRows = 5;
	const int nMenuCols = 1;

	title_menu_win =  newwin(startMenyWinRow, ncols, 0, 0);
	option_menu_win = newwin(nlines, ncols, startMenyWinRow, 0);

	//display the title in the center of the top window
	mvwprintw(title_menu_win,startMenyWinRow/2 + 1,(ncols-(int)strlen(portName))/2,"%s",portName);
	wrefresh(title_menu_win);



	/* Set fore ground and back ground of the menu */
	set_menu_fore(option_menu, COLOR_PAIR(1) | A_REVERSE);
	set_menu_back(option_menu, COLOR_PAIR(2));
	set_menu_grey(option_menu, COLOR_PAIR(3));


	keypad(option_menu_win, TRUE);
	set_menu_mark(option_menu, " * ");

	/* Set main window and sub window */
	set_menu_win(option_menu, option_menu_win);
	set_menu_sub(option_menu, derwin(option_menu_win, nlines -3, ncols-2, 1, 1));
	set_menu_format(option_menu, nMenuRows, nMenuCols);

	/* Print a border around the main window and print a title */
      box(option_menu_win, 0, 0);
//	print_in_middle(option_menu_win, 1, 0, 40, "My Menu", COLOR_PAIR(1));
	mvwaddch(option_menu_win, 2, 0, ACS_LTEE);
	mvwhline(option_menu_win, 2, 1, ACS_HLINE, 38);
	mvwaddch(option_menu_win, 2, 39, ACS_RTEE);


	mvwprintw(option_menu_win, nlines - 2, 2, "Escape to Exit");
	menu_opts_off(option_menu,O_ONEVALUE);

	post_menu(option_menu);
	wrefresh(title_menu_win);
	wrefresh(option_menu_win);

	int c;
	while( (c = wgetch(option_menu_win)) != 27 )
	{
		ITEM *curItem = current_item(option_menu);

		OptionEl *p = (OptionEl*)item_userptr(curItem);

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
						bool setToTrue= false;
						menu_driver(option_menu, REQ_TOGGLE_ITEM);
						if(item_value(curItem) == TRUE)
						{
							setToTrue = true;
							p->value = item_name(curItem);
//							fprintf(stderr,"setting foobar = %s \n\n", item_name(curItem));
						}
						//if we are a radiobox - we need to disable/enable valid options
						if (p->mode == RADIOBOX)
						{
							for(count = 0; count < n_choices; ++count)
							{
						            curr = (OptionEl*)item_userptr(option_items[count]);
								// if we have the same user ptr - but we are not ourself - disable it!
								if (curr == p)
								{
									if (curItem == option_items[count] || !setToTrue)
									{
										item_opts_on(option_items[count], O_SELECTABLE);
									}
									else
									{
										item_opts_off(option_items[count], O_SELECTABLE);
									}
								}
							}
						}
					}
					else
					{
						p->value = NULL;
					}
				}
				else
				{
					p->value = getString(option_menu_win,p->value);
					if (p->value != NULL && strcmp("",p->value) != 0)
					{
						menu_driver(option_menu, REQ_TOGGLE_ITEM);
					}
					wrefresh(title_menu_win);
					wrefresh(option_menu_win);
				}
			}
			break;
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

		if (p->mode == CHECKBOX || p->mode == RADIOBOX)
		{
			const char* val = (item_value(items[i]) == TRUE) ? "true" : "false";
			printf("%s=%s\n", item_name(items[i]), val);
		}
		else if (p->mode == USER_INPUT)
		{
			if (p->value)
			{
				printf("%s=%s\n", item_name(items[i]), p->value);
			}
			else
			{
				printf("%s=\n", item_name(items[i]));
			}
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

