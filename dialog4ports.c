#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <curses.h>
#include <menu.h>
#include <err.h>
#include <sysexits.h>


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

//TODO - handle sigwinch - resize winsows
//TODO - replecate OTPIONS's looks and feel (the triple window approach)

static char *
getString(WINDOW *win, const char * const curVal)
{
	const int bufSize = 80;
	char mesg[]="Choose a new value: ";
	char *str = calloc(bufSize,sizeof(char));

	int row,col;
	getmaxyx(win,row,col);		/* get the number of rows and columns */

	int messageLen = (int)strlen(mesg);
	int messageStart = (col-messageLen)/2;

	mvwprintw(win,row/2,messageStart,"%s",mesg);     /* print the message at the center of the screen */
	if (curVal != NULL) {
		mvwprintw(win,row/2, messageStart + messageLen + 1," [ %s ]",curVal);
	}
	echo();
	wmove(win,row/2 + 1, (messageStart + messageLen)/2);
	waddstr(win,"==>");
	wgetnstr(win,str, bufSize -1);				/* request the input...*/
	noecho();
	wclear(win);
	wrefresh(win);

	return (str);
}

static unsigned int
countChar ( const char * const input, const char c )
{
	unsigned int retval = 0;
	const size_t len = strlen(input);
	size_t i;

	for (i=0; i < len; ++i)
		if (input[i] == c)
			++retval;

	return (retval);
}

void
outputValues(MENU *menu) {
	ITEM **items;
      OptionEl *p;
	const char* val;

	items = menu_items(menu);
	int i;
	for(i = 0; i < item_count(menu); ++i) {
		OptionEl *p = (OptionEl*)item_userptr(items[i]);

		if (p->mode == CHECKBOX || p->mode == RADIOBOX) {
			val = (item_value(items[i]) == TRUE) ? "true" : "false";
			fprintf(stderr,"%s=%s\n", item_name(items[i]), val);
		} else if (p->mode == USER_INPUT) {
			if (p->value)
				fprintf(stderr,"%s=%s\n", item_name(items[i]), p->value);
			else
				fprintf(stderr,"%s=\n", item_name(items[i]));
		}
	}
}


//parseArguments(int atg



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

	int c;

	unsigned int numElements = 0;
	unsigned int n_choices = 0;
	unsigned int hashMarks = 0;

	if (argc < 2)
		errx(EX_USAGE,"We require some option type to work");

	if (argc < 3)
		errx(EX_USAGE,"We need more than just a port name");


	/* Avoid C++ style declaration inside loops */
	int arg;

	const char * const portName = argv[1];


	// arg=0 program name
	// arg=1 port title
	for (arg=2; arg < argc; ++arg) {
		++numElements;
		if ((curr = malloc (sizeof *curr)) == NULL)
			errx(EX_OSERR,"can not malloc");

		if (!head)
			head = curr;

		if (prev)
			prev->next = curr;

		bool gotName = false;
		bool gotDescr = false;
		char *internal_token;
		while((internal_token = strsep(&argv[arg], "=")) != NULL) {
			if (!gotName) {
				curr->name = internal_token;
				gotName = true;
			} else if (!gotDescr) {
				curr->descr = internal_token;
				gotDescr = true;
			} else {
				curr->options = internal_token;
				if (strcmp("%",curr->options) == 0) {
					curr->mode = CHECKBOX;
				} else if (strcmp("-", curr->options) == 0) {
					curr->mode = USER_INPUT;
				} else {
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
	if(has_colors() == TRUE) {
		start_color();
		init_pair(1, COLOR_GREEN, COLOR_BLACK);     //selected
		init_pair(2, COLOR_YELLOW, COLOR_BLACK);	//selectable
		init_pair(3, COLOR_RED, COLOR_BLACK); //disabled
	}

	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	option_items=(ITEM**)calloc(n_choices + 1, sizeof(ITEM *));

	curr = head;
	unsigned int count = 0;

	while(curr) {
		if (curr->mode != RADIOBOX) {
			option_items[count] = new_item(curr->name, curr->descr);
			set_item_userptr(option_items[count], curr);
			count++;
		} else {
			char *tmpToken;
			char *tmpOption = calloc(strlen(curr->options)+1,sizeof(char));
			strncpy(tmpOption,curr->options,strlen(curr->options));

			while((tmpToken = strsep(&tmpOption, "#")) != NULL) {
	                  option_items[count] = new_item(tmpToken, curr->descr);
	                  set_item_userptr(option_items[count], curr);
				count++;
			}
		}
		curr = curr->next;
	}

	option_items[n_choices] = (ITEM *)NULL;

	option_menu = new_menu(option_items);

/* a bunch of constants re the size of the window */


	/*
		maybe I should make this a struct instead?
	*/
	const int frameRows = getmaxy(stdscr);
	const int frameCols = getmaxx(stdscr);

	const int headRowStart = 0;
	const int headColStart = 0;
	const int headRows = 3;
	const int headCols = frameCols;

	const int exitRows = 4;
	const int exitCols = frameCols;
	const int exitRowStart = frameRows - exitRows;

	// menu == sizeof(largest item) + 1 space for each item
	const int full_exit_menu_size = strlen("CANCEL")*2+1;
	const int exitColStart = (frameCols - full_exit_menu_size)/2;

	const int licenceRows = 3;
	const int licenceCols = frameCols;
	const int licenceRowStart = exitRowStart - licenceRows;
	const int licenceColStart = 0;

	const int primaryRowStart = headRows + 1;
	const int primaryColStart = 0;
	const int primaryRows = frameRows - licenceRows - exitRows - 4;
	const int primaryCols = frameCols / 2;

	const int helpRowStart = headRows + 1;
	const int helpColStart = primaryCols + 1;
	const int helpRows = primaryRows;
	const int helpCols = frameCols - primaryCols;


	WINDOW *headWindow;
	WINDOW *exitWindow;
	WINDOW *licenceWindow;
	WINDOW *primaryWindow;
	WINDOW *helpWindow;

	headWindow = newwin(headRows, headCols, headRowStart, headColStart);
	exitWindow = newwin(exitRows, exitCols, exitRowStart, exitColStart);
	licenceWindow = newwin(licenceRows, licenceCols, licenceRowStart, licenceColStart);
	primaryWindow = newwin(primaryRows, primaryCols, primaryRowStart, primaryColStart);
	helpWindow = newwin(helpRows, helpCols, helpRowStart, helpColStart);

	/*	head + primary + help + licence + exit */
	const int minRows = headRows + licenceRows + exitRows + 1;
	const int minCols = headCols;

	if (frameRows < minRows || frameCols < minCols) {
		errx(EX_UNAVAILABLE, "Terminal size is to small");
	}


	ITEM** exitItems = (ITEM**)calloc(2 + 1, sizeof(ITEM*));

	const int exitOK = 0;
	const int exitCancel = 1;
	exitItems[exitOK] = new_item("OK", "");
	exitItems[exitCancel] = new_item("CANCEL", "");
	exitItems[2] = (ITEM*)NULL;

	MENU *exitMenu = new_menu(exitItems);

      set_menu_win(exitMenu, exitWindow);
      set_menu_sub(exitMenu, derwin(exitWindow, exitRows, exitCols, 0, 0));
	// 1 row - 2 cols for ok/cancel
      set_menu_format(exitMenu, 1, 2);
	set_menu_mark(exitMenu, ">");


      post_menu(exitMenu);
	wrefresh(exitWindow);


	menu_opts_off(option_menu, O_SHOWDESC);
	menu_opts_on(option_menu, O_NONCYCLIC);



















	// we want to leave 3 lines for the title
	const int startMenyWinRow = 3;

	const int nMenuRows = primaryRows - 1;
	const int nMenuCols = 1;

	//display the title in the center of the top window
	mvwprintw(headWindow,startMenyWinRow/2 + 1,(headCols-(int)strlen(portName))/2,"%s",portName);
	wrefresh(headWindow);

	if(has_colors() == TRUE) {
		/* Set fore ground and back ground of the menu */
		set_menu_fore(option_menu, COLOR_PAIR(1) | A_REVERSE);
		set_menu_back(option_menu, COLOR_PAIR(2));
		set_menu_grey(option_menu, COLOR_PAIR(3));
	}


	keypad(primaryWindow, TRUE);
	keypad(exitWindow, TRUE);
	keypad(helpWindow, TRUE);
	keypad(headWindow, TRUE);
	keypad(licenceWindow, TRUE);

	set_menu_mark(option_menu, " * ");

	/* Set main window and sub window */
	set_menu_win(option_menu, primaryWindow);
	set_menu_sub(option_menu, derwin(primaryWindow, primaryRows -3, primaryCols - 2, 1, 1));
	set_menu_format(option_menu, nMenuRows, nMenuCols);

	/* Print a border around the main window and print a title */
	wborder(primaryWindow, '|', '|', '-', '-', ACS_PI, ACS_PI, ACS_PI, ACS_PI);

	menu_opts_off(option_menu,O_ONEVALUE);
	post_menu(option_menu);


	wrefresh(headWindow);
	wrefresh(primaryWindow);

	/* toggling and truth have nothing to do with each other :-)
	   so go thru each one, set the envrioment, and then return to top
	*/
	for(count = 0; count < n_choices; ++count) {

            curr = (OptionEl*)item_userptr(option_items[count]);
		curr->value = getenv(curr->name);
		if (curr->value != NULL)
		{
			set_item_value(curr, true);
			menu_driver(option_menu, REQ_TOGGLE_ITEM);
		}
            menu_driver(option_menu, REQ_DOWN_ITEM);
	}
      menu_driver(option_menu, REQ_FIRST_ITEM);


	const WINDOW *winGetInput = primaryWindow;
	const MENU	 *whichMenu = option_menu;
	bool weWantMore = true;
	bool somethingChanged = false;
	while(weWantMore) {
		c = wgetch(winGetInput);
		ITEM *curItem = current_item(whichMenu);

		switch(c)
		{
			case KEY_DOWN:
				menu_driver(whichMenu, REQ_DOWN_ITEM);
				break;
			case KEY_UP:
				menu_driver(whichMenu, REQ_UP_ITEM);
				break;
			case KEY_LEFT:
				menu_driver(whichMenu, REQ_LEFT_ITEM);
				break;
			case KEY_RIGHT:
				menu_driver(whichMenu, REQ_RIGHT_ITEM);
				break;
			case KEY_NPAGE:
				menu_driver(whichMenu, REQ_SCR_DPAGE);
				break;
			case KEY_PPAGE:
				menu_driver(option_menu, REQ_SCR_UPAGE);
				break;
			case  9: /* tab */
				/* it goes
					primary -> licence -> exit -> ...

					I'm not sure how to handle scrolling help text

				*/
				if (winGetInput == primaryWindow) {
					winGetInput = exitWindow;
					whichMenu = exitMenu;
				}
				else if (winGetInput == exitWindow) {
					winGetInput = primaryWindow;
					whichMenu = option_menu;
				}
				break;
			case ' ':
			case 10:
			case KEY_ENTER:
				if (winGetInput == primaryWindow)
				{
					OptionEl *p = (OptionEl*)item_userptr(curItem);
					if (item_opts(curItem) & O_SELECTABLE)
					{
						if (p->mode != USER_INPUT)
						{
							bool setToTrue= false;
							menu_driver(whichMenu, REQ_TOGGLE_ITEM);
							if(item_value(curItem) == TRUE)
							{
								setToTrue = true;
								p->value = item_name(curItem);
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
											item_opts_on(option_items[count], O_SELECTABLE);
										else
											item_opts_off(option_items[count], O_SELECTABLE);
									}
								}
							}
						}
						else
						{
							p->value = getString(primaryWindow,p->value);
							wborder(primaryWindow, '|', '|', '-', '-', ACS_PI, ACS_PI, ACS_PI, ACS_PI);
							wrefresh(headWindow);
//							wrefresh(primaryWindow);
//							refresh();
							if (p->value != NULL && strcmp("",p->value) != 0)
							{
								if (item_value(curItem) != TRUE)
								{
									menu_driver(whichMenu, REQ_TOGGLE_ITEM);
								}
							}
							else
							{
								if (item_value(curItem) != FALSE)
								{
									menu_driver(whichMenu, REQ_TOGGLE_ITEM);
								}
								p->value = NULL;
							}

							wrefresh(headWindow);
							wrefresh(primaryWindow);
						}
					}
				}
				else if (winGetInput == exitWindow)
				{
					if (curItem == exitItems[exitOK])
						somethingChanged = true;
					weWantMore = false;
				}
				break;
/*			default:
				endwin();
				printf("%d",c); */
			}
		}
	unpost_menu(option_menu);
	endwin(); //get out of ncurses

	delwin(headWindow);
	delwin(primaryWindow);
	delwin(helpWindow);
	delwin(licenceWindow);
	delwin(exitWindow);

	if (somethingChanged)
	{
		outputValues(option_menu);
	}

	for (count = 0; count < n_choices; ++count)
		free_item(option_items[count]);

	free_menu(option_menu);
	free_menu(exitMenu);

	curr = head;
	while (curr) {
		next = curr->next;
		free(curr);
		curr = next;
	}

	return ((somethingChanged) ? exitOK : exitCancel);
}

