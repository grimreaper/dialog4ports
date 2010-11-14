/*-
* Copyright 2010 by Eitan Adler
* Copyright 2001 by Pradeep Padala.

* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation files
* (the "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge,
* publish, distribute, distribute with modifications, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
* IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name(s) of the above copyright holders
* shall not be used in advertising or otherwise to promote the sale, use or
* other dealings in this Software without prior written authorization.
*
*/

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sysexits.h>

#include "dialog4ports.h"

/*	TODO	- refactor large main into smaller units
*	TODO	- change -hfile to wxs's method for taking long descr
*	 --option WITH_FOO --hfile "FOO:long description" --wh
*	TODO	- --min --max options
*	TODO	- --requires option
*	TODO	- add panels code to allow for popups
*	TODO - convert windows to an array that can be looped?!
*	TODO - remove need for nElements ?
*	TODO - change radio handling to work with set_item_term
*/

/*
* Prints some text in the center of a specified line and window
*/
int
printInCenter(WINDOW *win, const int row, const char * const str)
{
	const int cols = getmaxx(win);
	if (cols == ERR)
		errx(EX_SOFTWARE, "unable to determine number of columns");
	const int start = (cols-(int)strlen(str))/2;
      if (mvwaddstr(win, row, start, str) == ERR)
		errx(EX_SOFTWARE, "unable to write string to center of screen for unknown reason");
	return (start);
}

/*
* Asks the user for some data and returns the data entered
*/
static char *
getString(WINDOW *win, const char * const curVal)
{
	int e;
	const unsigned int bufSize = 80;
	char mesg[]="Choose a new value: ";
	char *str = calloc(bufSize, sizeof(char));
	if (str == NULL)
		errx(EX_OSERR, "Can not calloc");

	int messageStart;
	int row = getmaxy(win);

	messageStart = printInCenter(win, row/2, mesg);

	if (curVal != NULL) {
		e = mvwprintw(win,row/2, messageStart + (int)strlen(mesg) + 1," [ %s ]",curVal);
		if (e == ERR)
			errx(EX_SOFTWARE, "unable to add original string for unknown reason");
	}
	e = echo();
	if (e == ERR)
		errx(EX_SOFTWARE, "character strokes must not echo");

	e = wmove(win,row/2 + 1, (messageStart + (int)strlen(mesg))/2);
	if (e == ERR)
		errx(EX_SOFTWARE, "unable to move to message start location");

	e = waddstr(win,"==>");
	if (e == ERR)
		errx(EX_SOFTWARE, "unable to add prompt for some reason");
	e= wgetnstr(win, str, (int)bufSize -1);				/* request the input...*/
	if (e == ERR)
		errx(EX_SOFTWARE, "error when attempting to get input");
	e = noecho();
	if (e == ERR)
		errx(EX_SOFTWARE, "character strokes must echo");

	e = wclear(win);
	if (e == ERR)
		errx(EX_SOFTWARE, "unable to clear the screen");

	e = doupdate();
	if (e == ERR)
		errx(EX_SOFTWARE, "unable to update the screen");

	return (str);
}

/*
* counts the occurences of a specified character in a string
*/
static unsigned int
countChar ( const char * const input, const char c )
{
	unsigned int retval = 0;
	const size_t len = strlen(input);
	size_t i;

	for (i = 0; i < len; ++i)
		if (input[i] == c)
			++retval;

	return (retval);
}

/*
*	Outputs binary values in a true/false form
*/
void
outputBinaryValue(ITEM* item, const char *key)
{
	bool val = item_value(item) == TRUE;
	fprintf(stderr, "%s=%s\n", key, (val) ? "true" : "false");
}

/*
* loops thru a menu and outputs selected values
*/
void
outputValues(MENU *menu)
{
	ITEM **items;
	int i;
	items = menu_items(menu);
	for(i = 0; i < item_count(menu); ++i) {
		OptionEl *p = (OptionEl*)item_userptr(items[i]);
		if (p->mode == CHECKBOX || p->mode == RADIOBOX) {
			outputBinaryValue(items[i], item_name(items[i]));
		} else if (p->mode == USER_INPUT) {
			if (p->value)
				fprintf(stderr, "%s=%s\n", item_name(items[i]), p->value);
			else
				fprintf(stderr, "%s=\n", item_name(items[i]));
		}
	}
	if (!requiredItemsSelected(items))
		fprintf(stderr, "REQUIRED_SELECTION=false\n");
}

/*
* returns true if all required items have a value
*/
bool
requiredItemsSelected(ITEM **items)
{
	int i;
	bool allRequired = true;
	i = 0;
	while (items[i] != NULL) {
		OptionEl *p = (OptionEl*)item_userptr(items[i]);
		if (p->required && p->value == NULL)
			allRequired = false;
		i++;
	}
	return allRequired;
}

/*
* sets radio items to appropriate values
*/

void
fixEnabledOptions(ITEM** option_items, int myIndex)
{
	int count;
	count = 0;
	while (option_items[count] != NULL) {
		OptionEl* p = item_userptr(option_items[count]);
		if (item_userptr(option_items[myIndex]) == p)
			if (myIndex == count || (item_value(option_items[myIndex]) != TRUE))
				item_opts_on(option_items[count], O_SELECTABLE);
			else
				item_opts_off(option_items[count], O_SELECTABLE);
		count++;
	}

}

/*
*	parses the arguments and modifies arginfo
*	with some information
*/
struct ARGINFO*
parseArguments(const int argc, char * argv[])
{
	struct ARGINFO *arginfo = malloc(sizeof(*arginfo));
	int arg = 0;
	OptionEl *curr = NULL;
	OptionEl *prev = NULL;
	const char* internal_token = NULL;

	for (arg=0; arg < argc; ++arg)
		printf("%s\n",argv[arg]);

	enum {
		OPEN,			/* we can get the next argument */
		NEXT_OPTION, 	/* fix the struct	*/
		READ_LNAME,		/* next thing is the wlicence name */
		READ_LTEXT, 	/* next thing to read is the licence text */
		READ_PNAME, 	/* next thing is the port name */
		READ_PCOMMENT,	/* next thing is the port comment */
		PREV_HFILE		/* next thing is prev's hfile */
	} stage;


	if (argc < 2) {
		usage();
		exit(0);
	}

	/* arg=0 program name
	* arg=1 port title & comment
	*/

	arginfo->outputLicenceRequest = false;
	arginfo->head = NULL;
	arginfo->portname = NULL;
	arginfo->portcomment = NULL;
	arginfo->licenceText = NULL;
	arginfo->licenceName = NULL;
	arginfo->nElements = 0;
	arginfo->nHashMarks = 0;

	stage = OPEN;

	for (arg=1; arg < argc; ++arg) {
		if (stage == OPEN) {
			if (strcmp("--help",argv[arg]) == 0 || strcmp("-?",argv[arg]) == 0) {
				usage();
				exit(0);
			}
			else if (strcmp("--licence", argv[arg]) == 0) {
				arginfo->outputLicenceRequest = true;
				stage = READ_LNAME;
				continue;
			}
			else if (strcmp("--licence-text", argv[arg]) == 0) {
				arginfo->outputLicenceRequest = true;
				stage = READ_LTEXT;
				continue;
			}
			else if (strcmp("--port", argv[arg]) == 0) {
				stage = READ_PNAME;
				continue;
			}
			else if (strcmp("--port-comment", argv[arg]) == 0) {
				stage = READ_PCOMMENT;
				continue;
			}
			else if (strcmp("--hfile", argv[arg]) == 0) {
				stage = PREV_HFILE;
				continue;
			}
			else if (strcmp("--required", argv[arg]) == 0) {
				prev->required = true;
				continue;
			}

			++arginfo->nElements;
			if ((curr = malloc (sizeof *curr)) == NULL)
				errx(EX_OSERR, "can not malloc");
			if (!arginfo->head)
				arginfo->head = curr;
			if (prev)
				prev->next = curr;
			if (strcmp("--option", argv[arg]) == 0) {
				curr->mode = CHECKBOX;
				stage = NEXT_OPTION;
			}
			else if (strcmp("--radio", argv[arg]) == 0) {
				curr->mode = RADIOBOX;
				stage = NEXT_OPTION;
			}
			else if (strcmp("--input", argv[arg]) == 0) {
				curr->mode = USER_INPUT;
				stage = NEXT_OPTION;
			}
			else {
				usage(true);
				errx(EX_USAGE,"Error code ID 10 T");
			}
		}
		else if (stage == NEXT_OPTION) {
			bool gotName = false;
			bool gotDescr = false;
			bool gotOpts = false;
			while((internal_token = strsep(&argv[arg], "=")) != NULL) {
				curr->longDescrFile = NULL;
				if (!gotName) {
					curr->name = internal_token;
					gotName = true;
				} else if (!gotDescr) {
					curr->descr = internal_token;
					gotDescr = true;
				} else if (!gotOpts && curr->mode == RADIOBOX) {
					arginfo->nHashMarks += countChar(internal_token, '#');
					curr->options = internal_token;
					gotOpts = true;
				}
				else {
					curr->longDescrFile = internal_token;
				}
			}
			curr->value = NULL;
			curr->required = false;
			prev = curr;
			curr = curr->next;
			stage = OPEN;
		}
		else if (stage == READ_LNAME) {
			arginfo->licenceName = argv[arg];
			stage = OPEN;
		}
		else if (stage == READ_LTEXT) {
			arginfo->licenceText = argv[arg];
			stage = OPEN;
		}
		else if (stage == READ_PNAME) {
			arginfo->portname = argv[arg];
			stage = OPEN;
		}
		else if (stage == READ_PCOMMENT) {
			arginfo->portcomment = argv[arg];
			stage = OPEN;
		}
		else if (stage == PREV_HFILE) {
			prev->longDescrFile = argv[arg];
			stage = OPEN;
		}
	}

/*	prev = arginfo->head;
	while (prev) {
		printf("YZ:\n \n\tname=%s \n\toptions=%s \n\tdescr=%s \n\tvalue=%s, \n\tlongDescrFile=%s \n\tmode=%d\n------\n",
				prev->name, prev->options, prev->descr, prev->value, prev->longDescrFile, prev->mode);
		prev = prev->next;
	}
*/

	if (arginfo->nElements == 0)
		errx(EX_USAGE,"We need at least one option");

	return (arginfo);
}

/*
* copies a specified file to specified window
* refuses to copy more than 80 char/line or max width of the window
*/
void
printFileToWindow(WINDOW * const win, const char * const filename)
{
	int row;
	int maxCols;
	/* function fileToWindow ? */
	const unsigned int maxCharPerLine = 80; /* NEVER - ever go above 80
								deal with terminal size below */

	const int maxRows = getmaxy(win) - 1;

	char buf[maxCharPerLine + 1];

	FILE *hFile = fopen(filename, "r");

	maxCols = getmaxx(win);
	if (hFile == NULL)
		errx(EX_IOERR, "File specified does not exist");
	/* never read more than 80 charaters per line */
	row = 1;
	while (fgets(buf, maxCharPerLine, hFile)) {
		if (row < maxRows) {
			const int result = mvwaddnstr(win, row++, 1, buf, maxCols - 1);
			if (result == ERR)
				errx(EX_SOFTWARE, "Unable to write string to screen for unknown reason");
		}
	}

	fclose(hFile);
}

/*
* Outputs usage information and optionally errors out.
*/
static void
usage() {
	fprintf(stderr,"%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
		"--port portname",
		"[--port-comment 'port comment']",
		"[--licence name of default licence]",
		"[--licence-text filename of licence]",
		"--option value=optionName=description [--hfile filename]",
		"--radio value=optionName=description=option1#option2 [--hfile filename]",
		"--input value=optionName=description [--hfile filename]"
	);
}

int
main(int argc, char* argv[])
{
	/* create the linked list that I work with later */

	/* culot: better use sys/queue.h instead of your own linked list implementation.
	   It will be easier to maintain, see queue(3). */
	OptionEl *curr = NULL;
	OptionEl *next = NULL;

	ITEM	**option_items;
	MENU	*option_menu;
	ITEM	**exitItems;
	ITEM	**licenceItems;

	bool weWantMore = true;
	bool somethingChanged = false;
	int c;
	ITEM *curItem;
	WINDOW *oldwindow;

	WINDOW	*headWindow;
	WINDOW	*exitWindow;
	WINDOW	*licenceWindow;
	WINDOW	*primaryWindow;
	WINDOW	*helpWindow;

	WINDOW	*winGetInput;
	MENU		*whichMenu;
	MENU		*licenceMenu;
	MENU		*exitMenu;

	bool licenceAccepted;

	const int nWindows = 5;
	PANEL *windowPanels[nWindows];


	unsigned int n_choices = 0;
	unsigned int count;

	int curTopRow;

	chtype topChar = '-';
	chtype bottomChar = '-';

	struct ARGINFO *arginfo = parseArguments(argc, argv);
	/*exit(42);*/

	/* deal with curses */
	curr = arginfo->head;

	n_choices = arginfo->nElements + arginfo->nHashMarks;

	initscr();
	if(has_colors() == TRUE) {
		start_color();
		init_pair(1, COLOR_GREEN, COLOR_BLACK);   /* selected */
		init_pair(2, COLOR_YELLOW, COLOR_BLACK);	/* selectable */
		init_pair(3, COLOR_RED, COLOR_BLACK); 	/* disabled */
	}

	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	option_items=(ITEM**)calloc(n_choices + 1, sizeof(ITEM *));
	if (option_items == NULL)
		errx(EX_OSERR, "unable to make room for option_items");

	curr = arginfo->head;
	count = 0;
	while(curr) {
		if (curr->mode != RADIOBOX) {
			option_items[count] = new_item(curr->name, curr->descr);
			set_item_userptr(option_items[count], curr);
			count++;
		} else {
			char *tmpToken;
			char *tmpOption = calloc(strlen(curr->options)+1, sizeof(char));
			if (tmpOption == NULL)
				errx(EX_OSERR, "unable to make room for tmpOption");
			strncpy(tmpOption, curr->options, strlen(curr->options));

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
	//set_item_term(option_menu, runMeOnMenuCall);

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
	/* menu == sizeof(largest item) + 1 space for each item */
	const int full_exit_menu_size = (int)strlen("CANCEL")*2+1;
	const int exitColStart = (frameCols - full_exit_menu_size)/2;

	const int licenceRows = 3;
	const int licenceCols = frameCols;
	const int licenceRowStart = exitRowStart - licenceRows;
	/* menu == sizeof(largest item) + 1 space for each item */
	const int full_licence_menu_size = (int)(strlen("ACCEPT")+strlen("the licence"))*2+1;
	/* Hack because menu ignores starting location */
	int licenceColStart;
      if (arginfo->outputLicenceRequest)
		licenceColStart = (frameCols - full_licence_menu_size)/2;
	else
		licenceColStart = 0;

	const int primaryRowStart = headRows + 1;
	const int primaryColStart = 0;
	const int primaryRows = frameRows - licenceRows - exitRows - 4;
	const int primaryCols = frameCols / 2;

	const int helpRowStart = headRows + 1;
	const int helpColStart = primaryCols + 1;
	const int helpRows = primaryRows;
	const int helpCols = frameCols - primaryCols - 1;


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


	exitItems = (ITEM**)calloc(2 + 1, sizeof(ITEM*));
	if (exitItems == NULL)
		errx(EX_OSERR, "unable to make room for exitItems");

	const int exitOK = 0;
	const int exitCancel = 1;
	exitItems[exitOK] = new_item("OK", "");
	exitItems[exitCancel] = new_item("CANCEL", "");
	exitItems[2] = (ITEM*)NULL;

	for (c = 0; c < 1; ++c) {
		if (exitItems[c] == NULL)
			errx(EX_OSERR, "Bad null in  exitItems");
	}

	exitMenu = new_menu(exitItems);

      set_menu_win(exitMenu, exitWindow);
      set_menu_sub(exitMenu, derwin(exitWindow, exitRows, exitCols, 0, 0));
	/* 1 row - 2 cols for ok/cancel */
      set_menu_format(exitMenu, 1, 2);
	set_menu_mark(exitMenu, ">");

	set_menu_fore(exitMenu, COLOR_PAIR(1));
	set_menu_back(exitMenu, COLOR_PAIR(2));
      set_menu_grey(exitMenu, COLOR_PAIR(3));

      post_menu(exitMenu);
      menu_driver(exitMenu, REQ_FIRST_ITEM);
	menu_driver(exitMenu, REQ_TOGGLE_ITEM);
	doupdate();


	licenceItems = (ITEM**)calloc(2 + 1, sizeof(ITEM*));
	if (licenceItems == NULL)
		errx(EX_OSERR, "unable to make room for licenceItems");


	/* default to no... */
	const int licenceNO = 0;
	const int licenceYES  = 1;
	licenceItems[licenceNO] = new_item("REJECT", "the licence");
	licenceItems[licenceYES] = new_item("ACCEPT", "the licence");
	licenceItems[2] = (ITEM*)NULL;

	ITEM* licenceSelected = licenceItems[licenceNO];

	if (arginfo->outputLicenceRequest) {
		licenceMenu = new_menu(licenceItems);

	      set_menu_win(licenceMenu, licenceWindow);
      	set_menu_sub(licenceMenu, derwin(licenceWindow, licenceRows, licenceCols, 1, 0));
		/* 1 row - 2 cols for ok/cancel */
      	set_menu_format(licenceMenu, 1, 2);
		set_menu_mark(licenceMenu, ">");
		set_menu_fore(licenceMenu, COLOR_PAIR(1));
		set_menu_back(licenceMenu, COLOR_PAIR(2));
      	set_menu_grey(licenceMenu, COLOR_PAIR(3));


	      post_menu(licenceMenu);
		menu_driver(licenceMenu, REQ_TOGGLE_ITEM);
	}
	else {
		const char* const licenceAcceptedMessage = "The licence for this port has already been accepted or does not exist";
		printInCenter(licenceWindow, 1, licenceAcceptedMessage);
	}
	doupdate();



	menu_opts_off(option_menu, O_SHOWDESC);
	menu_opts_on(option_menu, O_NONCYCLIC);

	/* we want to leave 3 lines for the title */
	const int startMenyWinRow = 3;

	const int nMenuRows = primaryRows - 2;
	const int nMenuCols = 1;

	/* display the title in the center of the top window */
	printInCenter(headWindow, startMenyWinRow/2, arginfo->portname);
	if (arginfo->portcomment != NULL)
		printInCenter(headWindow, startMenyWinRow/2 + 1, arginfo->portcomment);
	doupdate();

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
	if (arginfo->outputLicenceRequest)
		keypad(licenceWindow, TRUE);

	set_menu_mark(option_menu, " * ");

	/* Set main window and sub window */
	set_menu_win(option_menu, primaryWindow);
	set_menu_sub(option_menu, derwin(primaryWindow, nMenuRows, primaryCols - 2, 1, 1));
	set_menu_format(option_menu, nMenuRows, nMenuCols);

	/* Print a border around the main window and print a title */
	/* note the '.' for scrollable direction */

	if (n_choices > (unsigned int)nMenuRows)
		wborder(primaryWindow, '|', '|', '-',  ACS_DARROW,  0, 0, 0, 0);
	else
		wborder(primaryWindow, '|', '|', '-', '-',  0, 0, 0, 0);

	wborder(helpWindow, '|', '|', '-', '-',  0, 0, 0, 0);

	menu_opts_off(option_menu,O_ONEVALUE);
	post_menu(option_menu);


	doupdate();

	/* toggling and truth have nothing to do with each other :-)
	   so go thru each one, set the envrioment, and then return to top
	*/
	for(count = 0; count < n_choices; ++count) {
            curr = (OptionEl*)item_userptr(option_items[count]);
		curr->value = getenv(curr->name);
		if (curr->value != NULL)
		{
			set_item_value(option_items[count], true);
			menu_driver(option_menu, REQ_TOGGLE_ITEM);
		}
		if (curr->mode == CHECKBOX && curr->required) {
			set_item_value(option_items[count], true);
			curr->value = item_name(option_items[count]);
/*			item_opts_off(option_items[count], O_SELECTABLE);	*/
		}
            menu_driver(option_menu, REQ_DOWN_ITEM);
	}
      menu_driver(option_menu, REQ_FIRST_ITEM);


	licenceAccepted = false;
	winGetInput = primaryWindow;
	whichMenu = option_menu;

	windowPanels[0] = new_panel(headWindow);
	windowPanels[1] = new_panel(exitWindow);
	windowPanels[2] = new_panel(licenceWindow);
	windowPanels[3] = new_panel(primaryWindow);
	windowPanels[4] = new_panel(helpWindow);
	update_panels();
	wrefresh(primaryWindow); // get cursor to proper location
	doupdate();

	while(weWantMore) {

		c = wgetch(winGetInput);
		curItem = current_item(whichMenu);

		oldwindow = winGetInput;
		if (arginfo->outputLicenceRequest)
			if (winGetInput == licenceWindow)
				licenceSelected = curItem;
		switch(c) {
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
					primary -> [licence] -> exit -> ...

					I'm not sure how to handle scrolling help text

				*/
      		      set_menu_fore(whichMenu, COLOR_PAIR(1));

				if (winGetInput == primaryWindow) {
					if (arginfo->outputLicenceRequest) {
						winGetInput = licenceWindow;
						whichMenu = licenceMenu;
					}
					else {
						winGetInput = exitWindow;
						whichMenu = exitMenu;
					}
				}
				else if (winGetInput == licenceWindow) {
					winGetInput = exitWindow;
					whichMenu = exitMenu;
				}
				else if (winGetInput == exitWindow) {
					winGetInput = primaryWindow;
					whichMenu = option_menu;
				}
      		      set_menu_fore(whichMenu, COLOR_PAIR(1) | A_REVERSE);
				doupdate();
				break;
			case ' ':
			case 10:
			case KEY_ENTER:
				if (winGetInput == primaryWindow) {
					OptionEl *p = (OptionEl*)item_userptr(curItem);
					if (item_opts(curItem) & O_SELECTABLE) {
						if (p->mode != USER_INPUT) {
							menu_driver(whichMenu, REQ_TOGGLE_ITEM);
							if (item_value(curItem) == TRUE)
								p->value = item_name(curItem);
							/* if we are a radiobox - we need to disable/enable valid options */
							if (p->mode == RADIOBOX)
								fixEnabledOptions(option_items, item_index(curItem));
						}
						else {
							p->value = NULL;
							p->value = getString(primaryWindow,p->value);
							if (p->value != NULL && strcmp("",p->value) != 0) {
								if (item_value(curItem) != TRUE)
									menu_driver(whichMenu, REQ_TOGGLE_ITEM);
							}
							else {
								if (item_value(curItem) != FALSE)
									menu_driver(whichMenu, REQ_TOGGLE_ITEM);
								p->value = NULL;
							}
							//////////wborder(primaryWindow, '|', '|', '-', '-', 0, 0, 0, 0);
							wborder(helpWindow, '|', '|', '-', '-', 0, 0, 0, 0);
							doupdate();
						}
					}
				}
				else if (winGetInput == exitWindow) {
					if (curItem == exitItems[exitOK])
						somethingChanged = true;
					weWantMore = false;
				}
				break;
			case 27: /* ESCAPE */
				weWantMore = false;
				break;
			}
			/*
				this rereads the file each time. perhaps it could be cached?
			*/
			wclear(helpWindow);

			if (winGetInput == primaryWindow ) {
				OptionEl *p = (OptionEl*)item_userptr(current_item(whichMenu));
				if (p->longDescrFile != NULL) {
					printFileToWindow(helpWindow, p->longDescrFile);
				}

				topChar = '-';
				bottomChar = '-';

				curTopRow = top_row(whichMenu);
				if (curTopRow == ERR)
					errx(EX_SOFTWARE, "The current top row was unavailable");

				if ((int)n_choices - curTopRow > nMenuRows)
					bottomChar = ACS_DARROW;
				if (curTopRow != 0)
					topChar = ACS_UARROW;
				wborder(primaryWindow, '|', '|', topChar, bottomChar, 0, 0, 0, 0);
			}
			else if (winGetInput == licenceWindow) {
				if (arginfo->licenceText != NULL)
					printFileToWindow(helpWindow, arginfo->licenceText);
				else if (arginfo->licenceName != NULL) {
					printInCenter(helpWindow,helpRows/2, "This licence is the default");
					printInCenter(helpWindow,helpRows/2 + 1, arginfo->licenceName);
				}
			}
			wborder(helpWindow, '|', '|', '-', '-', 0, 0, 0, 0);
			wrefresh(helpWindow);
			wrefresh(winGetInput);
			doupdate();

			if (winGetInput == licenceWindow) {
				if (current_item(licenceMenu) == licenceItems[licenceYES])
					licenceAccepted = true;
				else
					licenceAccepted = false;
			}
		}
	unpost_menu(option_menu);
	endwin(); /* get out of ncurses */

	delwin(headWindow);
	delwin(primaryWindow);
	delwin(helpWindow);
	delwin(licenceWindow);
	delwin(exitWindow);


	if (somethingChanged) {
		outputValues(option_menu);
		if (arginfo->outputLicenceRequest)
			fprintf(stderr,"%s=%s\n","ACCEPTED_LICENCE",(licenceAccepted) ? "true" : "false");
	}

	for (count = 0; count < n_choices; ++count)
		free_item(option_items[count]);

	free_menu(option_menu);
	if (arginfo->outputLicenceRequest)
		free_menu(licenceMenu);
	free_menu(exitMenu);

	curr = arginfo->head;
	while (curr) {
		next = curr->next;
		free(curr);
		curr = next;
	}

	free(arginfo);

	return ((somethingChanged) ? exitOK : exitCancel);
}
