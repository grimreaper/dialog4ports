q/*-
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
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "dialog4ports.h"

/*
*	 --option WITH_FOO --hfile "FOO:long description" --wh
*	TODO	- --requires option
*	TODO - scrolling in help window
*	TODO - Is it legal to modify the ptr after its set? (mitem_current)
*/

/*
* Prints some text in the center of a specified line and window
*/
int
printInCenter(WINDOW *win, const int row, const char * const str)
{
	int start;
	const int cols = getmaxx(win);
	if (cols == ERR)
		errx(EX_SOFTWARE, "unable to determine number of columns");
	start = (cols-(int)strlen(str))/2;
      if (mvwaddstr(win, row, start, str) == ERR)
		errx(EX_SOFTWARE, "unable to write string to center of screen for unknown reason");
	return (start);
}

/*
* Asks the user for some data and returns the data entered
*/
char *
getString(WINDOW *win, const char * const curVal)
{
	int e;
	int messageStart;
	const size_t bufSize = 80;
	int row;
	char mesg[]="Choose a new value: ";
	char *str = calloc(bufSize, sizeof(char));
	if (str == NULL)
		errx(EX_OSERR, "Can not calloc for getting string");

	row = getmaxy(win);

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
unsigned int
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
	fprintf(stderr, "%s=%s\n", key, (item_value(item)) ? "true" : "false");
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
			outputBinaryValue(items[i], item_name(items[i])+strlen(preNameToken));
		} else if (p->mode == USER_INPUT) {
			if (p->value)
				fprintf(stderr, "%s=%s\n", item_name(items[i])+strlen(preNameToken), p->value);
			else
				fprintf(stderr, "%s=\n", item_name(items[i])+strlen(preNameToken));
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
	for (int i = 0; items[i] != NULL; ++i) {
		OptionEl *p = (OptionEl*)item_userptr(items[i]);
		if (p->required && p->value == NULL)
			return false;
	}
	return true;
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
			if (myIndex == count || !item_value(option_items[myIndex]))
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
	if (arginfo == NULL)
		errx(EX_OSERR, "can't make space for arginfo - bailing out!");
	int arg = 0;
	OptionEl *curr = NULL;
	OptionEl *prev = NULL;
	const char* internal_token = NULL;
	unsigned int radioID = 0;

#ifdef DEBUG
	for (arg=0; arg < argc; ++arg)
		printf("%s\n",argv[arg]);
#endif
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
			curr->next = NULL;
			if (!arginfo->head)
				arginfo->head = curr;
			if (prev)
				prev->next = curr;
			if (strcmp("--option", argv[arg]) == 0) {
				curr->mode = CHECKBOX;
				curr->id = 99; //doesn't matter now
				stage = NEXT_OPTION;
			}
			else if (strcmp("--radio", argv[arg]) == 0) {
				curr->mode = RADIOBOX;
				curr->id = radioID;
				radioID++;
				stage = NEXT_OPTION;
			}
			else if (strcmp("--input", argv[arg]) == 0) {
				curr->id = 99; //doesn't matter now
				curr->mode = USER_INPUT;
				stage = NEXT_OPTION;
			}
			else {
				usage();
				errx(EX_USAGE,"Error code ID 10 T");
			}
		}
		else if (stage == NEXT_OPTION) {
			bool gotName = false;
			bool gotDescr = false;
			bool gotOpts = false;

			if (curr->mode == RADIOBOX) {
				if (countChar(argv[arg], '=') < 2)
      	     			errx(EX_USAGE,"Radioboxes must contain at least 2");
			}
			else {
				if (countChar(argv[arg], '=') != 1)
      	     			errx(EX_USAGE,"Options must contain exactly one equals sign [ %s ]", argv[arg]);
			}


			while((internal_token = strsep(&argv[arg], "=")) != NULL) {
				if (!gotName) {
					char *useName = calloc(strlen(internal_token) + strlen(preNameToken) + 1,sizeof(char));
					if (useName == NULL)
						errx(EX_OSERR, "unable to make room for other useName");
					strcpy(useName, preNameToken);
					strcat(useName, internal_token);
					switch (curr->mode) {
						case CHECKBOX:
							useName[0] = '[';
							useName[2] = ']';
							break;
						case RADIOBOX:
							useName[0] = '(';
							useName[1] = '0' + curr->id;
							useName[2] = ')';
						break;
						case USER_INPUT:
							useName[0] = '{';
							useName[2] = '}';
						break;
					}
					curr->name = useName;
					gotName = true;
				} else if (!gotDescr) {
					curr->descr = internal_token;
					gotDescr = true;
				} else if (!gotOpts && curr->mode == RADIOBOX) {
					arginfo->nElements += countChar(internal_token, '#');
					curr->options = internal_token;
					gotOpts = true;
				}
			}
			curr->value = NULL;
			curr->required = false;
			curr->longDescrFile = NULL;
			curr->longDescrText = NULL;
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
			if (prev == NULL)
				errx(EX_USAGE, "--hfile requires previous --option, --radio or --input option.");
			prev->longDescrFile = argv[arg];
			stage = OPEN;
		}
	}

#ifdef DEBUG
	prev = arginfo->head;
	while (prev) {
		printf("YZ:\n \n\tname=%s \n\toptions=%s \n\tdescr=%s \n\tvalue=%s, \n\tlongDescrFile=%s \n\tmode=%d\n\tid=%d\n------\n",
				prev->name, prev->options, prev->descr, prev->value, prev->longDescrFile, prev->mode, prev->id);
		prev = prev->next;
	}
#endif

	if (arginfo->portname == NULL)
		errx(EX_USAGE,"Port name is required");

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
	//exit (99);
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
		errx(EX_IOERR, "File specified [%s] does not exist", filename);
	/* never read more than 80 charaters per line */
	row = 1;
	while (fgets(buf, maxCharPerLine, hFile)) {
		if (row < maxRows) {
			const int result = mvwaddnstr(win, row++, 1, buf, maxCols - 1);
			if (result == ERR)
				errx(EX_SOFTWARE, "Unable to write string to screen for unknown reason");
		}
	}

	if (fclose(hFile) != 0)
		errx(EX_OSERR, "Failed to close the file - bailing out now");
}

/*
* Outputs usage information and optionally errors out.
*/
void
usage(void) {
	fprintf(stderr,"%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
		"--port portname",
		"[--port-comment 'port comment']",
		"[--licence name of default licence]",
		"[--licence-text filename of licence]",
		"--option optionName=description [--hfile filename]",
		"--radio optionName=description=option1#option2 [--hfile filename]",
		"--input optionName=description [--hfile filename]",
		"please note that this program is intended to be used by the ports system - not the end user"
	);
}

/*
	endwin if ncurses is running
*/
void
cleanNcursesExit(const int n)
{
	endwin();
}


int
main(int argc, char* argv[])
{
	enum windowID {
		HEAD,
		PRIMARY,
		HELP,
		LICENCE,
		EXIT,
	};

	/* create the linked list that I work with later */

	/* culot: better use sys/queue.h instead of your own linked list implementation.
	   It will be easier to maintain, see queue(3). */
	OptionEl *curr = NULL;
	OptionEl *next = NULL;

	ITEM	**option_items;
	ITEM	**exitItems;
	ITEM	**licenceItems;
	WINDOW **windowList;

	bool weWantMore = true;
	bool somethingChanged = false;
	int c;
	enum windowID whichLocation;
	ITEM *curItem;


	const int nWindows = 5;
	MENU	**menuList = calloc(nWindows, sizeof(*menuList));
	if (menuList == NULL)
		errx(EX_OSERR, "unable to make room for menu list");

	const char * const colorCodes = getenv("D4PCOLOR");

	const char selectedMark = '*';
	unsigned int count;
	int frameRows;
	int frameCols;

	int curTopRow;

	_malloc_options = "J";

	chtype topChar = ACS_HLINE;
	chtype bottomChar = ACS_HLINE;

	struct ARGINFO *arginfo = parseArguments(argc, argv);
	/*exit(42);*/

	/* deal with curses */
	curr = arginfo->head;

	arginfo->nElements = arginfo->nElements;

	initscr();
	err_set_exit(cleanNcursesExit);

	short colorList[3][2] = { { COLOR_GREEN, COLOR_BLACK },
					{ COLOR_YELLOW, COLOR_BLACK },
					{ COLOR_RED, COLOR_BLACK }
					};

	if (colorCodes != NULL && strlen(colorCodes) != 6) {
		errx(EX_DATAERR, "colorCode must be exactly 6 characters long");
	}
	if (colorCodes != NULL) {
		for (c = 0; c < 6; ++c) {
			/* get the color code to an int instead of ascii value */
			if (colorCodes[c] - 0x30 <= 0x39)
				colorList[c/2][c & 1] = (short)(colorCodes[c] - 0x30);
			else
				errx(EX_DATAERR, "colorCode must be an int between 0 and 9");
		}
	}

	if(has_colors() == TRUE) {
		start_color();
		init_pair(1, colorList[0][0], colorList[0][1]);   /* selected */
		init_pair(2, colorList[1][0], colorList[1][1]);	/* selectable */
		init_pair(3, colorList[2][0], colorList[2][1]); 	/* disabled */
	}

	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	option_items=(ITEM**)calloc(arginfo->nElements + 1, sizeof(ITEM *));
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
				char *useName = calloc(strlen(preNameToken) + strlen(tmpToken) + 1,sizeof(char));
				if (useName == NULL)
					errx(EX_OSERR, "unable to make room for useName");
				strcpy(useName, preNameToken);
				strcat(useName, tmpToken);
                        useName[0] = '(';
                        useName[2] = ')';
	                  option_items[count] = new_item(useName, curr->descr);
	                  set_item_userptr(option_items[count], curr);
				count++;
			}
			free(tmpOption);
		}
		curr = curr->next;
	}

	option_items[arginfo->nElements] = (ITEM *)NULL;

	menuList[PRIMARY] = new_menu(option_items);
	//set_item_term(menuList[PRIMARY], runMeOnMenuCall);

	windowList = calloc (nWindows, sizeof(*windowList));
	if (windowList == NULL)
		errx(EX_OSERR, "window list is unfindable");

	frameRows = getmaxy(stdscr);
	frameCols = getmaxx(stdscr);

	struct windowStats *windowStatList = calloc(nWindows, sizeof(*windowStatList));
	windowStatList[HEAD].rowStart = 0;
	windowStatList[HEAD].colStart = 0;
	windowStatList[HEAD].cols = frameCols;
	windowStatList[HEAD].rows = 3;

	windowStatList[EXIT].rows = 2;
	windowStatList[EXIT].cols = frameCols;
	windowStatList[EXIT].rowStart = frameRows - windowStatList[EXIT].rows;
	/* menu == sizeof(largest item) + 1 space for each item */
	const int full_exit_menu_size = (int)strlen("CANCEL")*2+1;
	windowStatList[EXIT].colStart = (frameCols - full_exit_menu_size)/2;

	windowStatList[LICENCE].rows = 2;
	windowStatList[LICENCE].cols = frameCols;
	windowStatList[LICENCE].rowStart = windowStatList[EXIT].rowStart - windowStatList[LICENCE].rows;
	/* menu == sizeof(largest item) + 1 space for each item */
	const int full_licence_menu_size = (int)(strlen("ACCEPT")+strlen("the licence"))*2+1;
	/* Hack because menu ignores starting location */
      if (arginfo->outputLicenceRequest)
		windowStatList[LICENCE].colStart = (frameCols - full_licence_menu_size)/2;
	else
		windowStatList[LICENCE].colStart = 0;

	windowStatList[HELP].rows = 6;
	windowStatList[HELP].cols = frameCols;
	windowStatList[HELP].rowStart = windowStatList[LICENCE].rowStart - windowStatList[HELP].rows;
	windowStatList[HELP].colStart = 0 ;

	windowStatList[PRIMARY].rowStart = windowStatList[HEAD].rows + 1;
	windowStatList[PRIMARY].colStart = 0;
	windowStatList[PRIMARY].rows = windowStatList[HELP].rowStart - windowStatList[HEAD].rows - 1;
	windowStatList[PRIMARY].cols = frameCols ;

//	endwin();
	printf("FrameRows: %d\n",frameRows);
	for (c=0; c < nWindows; ++c) {
		windowList[c] = newwin(windowStatList[c].rows, windowStatList[c].cols, windowStatList[c].rowStart, windowStatList[c].colStart);
		if (windowList[c] == NULL)
			errx(EX_OSERR,"Ahh! a window has not been created!");
		printf("Window %d\n\t rows = %d\n\t cols = %d \n\t rowStart = %d \n\t colStart = %d \n" ,
			c,windowStatList[c].rows, windowStatList[c].cols, windowStatList[c].rowStart, windowStatList[c].colStart);
	}
	printf("0 HEAD, 1 PRIMARY, 2 HELP, 3 LICENCE, 4 EXIT");
//	exit(42);
	//box(windowList[HELP],ACS_PI,ACS_PI);
	exitItems = (ITEM**)calloc(3 + 1, sizeof(ITEM*));
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

	menuList[EXIT] = new_menu(exitItems);

      set_menu_win(menuList[EXIT], windowList[EXIT]);
      set_menu_sub(menuList[EXIT], derwin(windowList[EXIT], windowStatList[EXIT].rows, windowStatList[EXIT].cols, 0, 0));
	/* 1 row - 2 cols for ok/cancel */
      set_menu_format(menuList[EXIT], 1, 2);
	set_menu_mark(menuList[EXIT], ">");

      post_menu(menuList[EXIT]);
      menu_driver(menuList[EXIT], REQ_FIRST_ITEM);
	menu_driver(menuList[EXIT], REQ_TOGGLE_ITEM);

	licenceItems = (ITEM**)calloc(2 + 1, sizeof(ITEM*));
	if (licenceItems == NULL)
		errx(EX_OSERR, "unable to make room for licenceItems");


	/* default to no... */
	const int licenceNO = 0;
	const int licenceYES  = 1;
	const int licenceVIEW = 2;
	licenceItems[licenceNO] = new_item("REJECT", "the licence");
	licenceItems[licenceYES] = new_item("ACCEPT", "the licence");
	licenceItems[licenceVIEW] = new_item("VIEW", "the licence");
	licenceItems[3] = (ITEM*)NULL;

	if (arginfo->outputLicenceRequest) {
		menuList[LICENCE] = new_menu(licenceItems);
		//menu_opts_off(menuList[LICENCE], O_ONEVALUE);

	      set_menu_win(menuList[LICENCE], windowList[LICENCE]);
      	set_menu_sub(menuList[LICENCE], derwin(windowList[LICENCE], windowStatList[LICENCE].rows, windowStatList[LICENCE].cols, 1, 0));
		/* 1 row - 2 cols for ok/cancel */
      	set_menu_format(menuList[LICENCE], 1, 3);
		set_menu_mark(menuList[LICENCE], ">");


	      post_menu(menuList[LICENCE]);
		menu_driver(menuList[LICENCE], REQ_TOGGLE_ITEM);
	}
	else {
		const char* const licenceAcceptedMessage = "The licence for this port has already been accepted or does not exist";
		printInCenter(windowList[LICENCE], 1, licenceAcceptedMessage);
	}

	menu_opts_on(menuList[PRIMARY], O_SHOWDESC);
	menu_opts_on(menuList[PRIMARY], O_NONCYCLIC);

	/* we want to leave 3 lines for the title */
	const int startMenyWinRow = 3;

	const int nMenuRows = windowStatList[PRIMARY].rows - 2;
	const int nMenuCols = 1;

	/* display the title in the center of the top window */
	printInCenter(windowList[HEAD], startMenyWinRow/2, arginfo->portname);
	if (arginfo->portcomment != NULL)
		printInCenter(windowList[HEAD], startMenyWinRow/2 + 1, arginfo->portcomment);

	if(has_colors() == TRUE) {
		for (c = 0; c < nWindows; ++c) {
			if (menuList[c] != NULL) {
				set_menu_fore(menuList[c], COLOR_PAIR(1) | A_REVERSE);
				set_menu_back(menuList[c], COLOR_PAIR(2));
				set_menu_grey(menuList[c], COLOR_PAIR(3));
			}
		}
	}


	for (c = 0; c < nWindows; ++c)
		keypad(windowList[c], TRUE);

	set_menu_mark(menuList[PRIMARY], "");

	/* Set main window and sub window */
	set_menu_win(menuList[PRIMARY], windowList[PRIMARY]);
	set_menu_sub(menuList[PRIMARY], derwin(windowList[PRIMARY], nMenuRows, windowStatList[PRIMARY].cols - 2, 1, 1));
	set_menu_format(menuList[PRIMARY], nMenuRows, nMenuCols);

	/* Print a border around the main window and print a title */
	/* note the '.' for scrollable direction */

	if (arginfo->nElements > (unsigned int)nMenuRows)
		bottomChar = ACS_DARROW;

	wborder(windowList[PRIMARY], ACS_VLINE, ACS_VLINE, topChar, bottomChar,  0, 0, 0, 0);
	wborder(windowList[HELP], ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,  0, 0, 0, 0);

	menu_opts_off(menuList[PRIMARY],O_ONEVALUE);
	post_menu(menuList[PRIMARY]);

	/*
	   so go thru each one, set the envrioment, and then return to top
	*/
	for(count = 0; count < arginfo->nElements; ++count) {
            curr = (OptionEl*)item_userptr(option_items[count]);
		curr->value = getenv(curr->name + strlen(preNameToken));
		char * n;;
		if (curr->mode == RADIOBOX) {
			/* I highly doubt this is defined or legal behavior */
			n = (char*)item_name(option_items[count]);
		}
		else {
			n = curr->name;
		}
		if (curr->value != NULL)
		{
			set_item_value(option_items[count], true);
			n[1] = selectedMark;
		}
		else if (curr->mode == RADIOBOX ) {
			n[1] = '0' + curr->id;
		}
		if (curr->mode == CHECKBOX && curr->required) {
			set_item_value(option_items[count], true);
			curr->value = item_name(option_items[count]);
			curr->name[1] = selectedMark;
/*			item_opts_off(option_items[count], O_SELECTABLE);	*/
		}
            menu_driver(menuList[PRIMARY], REQ_DOWN_ITEM);
	}
      menu_driver(menuList[PRIMARY], REQ_FIRST_ITEM);


	whichLocation = PRIMARY;

	/* get cursor to proper location */
	pos_menu_cursor(menuList[whichLocation]);
	for (int i = 0; i < nWindows; ++i) {
		if (windowList[i] != NULL)
			wrefresh(windowList[i]);
	}
	doupdate();

	while(weWantMore) {

		c = wgetch(windowList[whichLocation]);
		curItem = current_item(menuList[whichLocation]);

		switch(c) {
			case KEY_DOWN:
				menu_driver(menuList[whichLocation], REQ_DOWN_ITEM);
				break;
			case KEY_UP:
				menu_driver(menuList[whichLocation], REQ_UP_ITEM);
				break;
			case KEY_LEFT:
				menu_driver(menuList[whichLocation], REQ_LEFT_ITEM);
				break;
			case KEY_RIGHT:
				menu_driver(menuList[whichLocation], REQ_RIGHT_ITEM);
				break;
			case KEY_NPAGE:
				menu_driver(menuList[whichLocation], REQ_SCR_DPAGE);
				break;
			case KEY_PPAGE:
				menu_driver(menuList[PRIMARY], REQ_SCR_UPAGE);
				break;
			case  9: /* tab */
				/* it goes
					primary -> [licence] -> exit -> ...

				*/
				if (whichLocation == PRIMARY) {
					if (arginfo->outputLicenceRequest)
						whichLocation = LICENCE;
					else
						whichLocation = EXIT;
				}
				else if (whichLocation == LICENCE)
					whichLocation = EXIT;
				else if (whichLocation == EXIT)
					whichLocation = PRIMARY;
				break;
			case ' ':
			case 10:
			case KEY_ENTER:
				if (whichLocation == PRIMARY) {
					OptionEl *p = (OptionEl*)item_userptr(curItem);

					if (item_opts(curItem) & O_SELECTABLE) {
						if (p->mode != USER_INPUT) {
							menu_driver(menuList[whichLocation], REQ_TOGGLE_ITEM);
							if (item_value(curItem) == TRUE)
								p->value = item_name(curItem);
							/* if we are a radiobox - we need to disable/enable valid options */
							if (p->mode == RADIOBOX)
								fixEnabledOptions(option_items, item_index(curItem));
						}
						else {
							p->value = getString(windowList[PRIMARY],p->value);
							if (p->value != NULL && strcmp("",p->value) != 0) {
								if (item_value(curItem) != TRUE)
									menu_driver(menuList[whichLocation], REQ_TOGGLE_ITEM);
							}
							else {
								if (item_value(curItem) == TRUE)
									menu_driver(menuList[whichLocation], REQ_TOGGLE_ITEM);
								p->value = NULL;
							}
						}
					}
					char * name = p->name;
					if (p->mode == RADIOBOX)
						name =  (char*)item_name(curItem);

					if (item_value(curItem) == TRUE)
						name[1] = selectedMark;
					else {
            				if (p->mode == RADIOBOX )
                  				name[1] = '0' + p->id;
						else
                 					name[1] = ' ';
					}

					/*
						hack to get star to show up now instead of after next key movment
					*/
					menu_driver(menuList[whichLocation], REQ_DOWN_ITEM);
					menu_driver(menuList[whichLocation], REQ_UP_ITEM);

					pos_menu_cursor(menuList[whichLocation]) ;
				}
				else if (whichLocation == LICENCE) {
					if (curItem == licenceItems[licenceVIEW]) {
						char const * const pager = getenv("PAGER");
						def_prog_mode();
						endwin();
						const pid_t pid = fork();
						if (pid == 0) {
							int e = execl(pager, pager, arginfo->licenceText,NULL);
							if (e == -1)
								errx(EX_OSERR,"failed to exec: %d", errno);
						}
						else if (pid == -1)
							errx(EX_OSERR, "Can't fork");
						else {
							waitpid(pid, NULL, 0);
						}
						reset_prog_mode();
						wrefresh(curscr);
					}
				}
				else if (whichLocation == EXIT) {
					if (curItem == exitItems[exitOK])
						somethingChanged = true;
					weWantMore = false;
				}
				break;
			case 27: /* ESCAPE */
				weWantMore = false;
				break;
			default:
				continue;
		}
		/*
			this rereads the file each time. perhaps it could be cached?
		*/
		/* don't wclear help unless we need to; this results in some cases of stale text
		in the help window */
		if (whichLocation == PRIMARY ) {
			wclear(windowList[HELP]);
			OptionEl *p = (OptionEl*)item_userptr(current_item(menuList[whichLocation]));
			if (p->longDescrFile != NULL) {
				printFileToWindow(windowList[HELP], p->longDescrFile);
			}
			topChar = ACS_HLINE;
			bottomChar = ACS_HLINE;
			curTopRow = top_row(menuList[whichLocation]);
			if (curTopRow == ERR)
				errx(EX_SOFTWARE, "The current top row was unavailable");
			if ((int)arginfo->nElements - curTopRow > nMenuRows)
				bottomChar = ACS_DARROW;
			if (curTopRow != 0)
				topChar = ACS_UARROW;
			wborder(windowList[PRIMARY], ACS_VLINE, ACS_VLINE, topChar, bottomChar, 0, 0, 0, 0);
		}
		else if (whichLocation == LICENCE) {
			wclear(windowList[HELP]);
			if (arginfo->licenceText != NULL)
			{
				printFileToWindow(windowList[HELP], arginfo->licenceText);
			}
			else if (arginfo->licenceName != NULL) {
				printInCenter(windowList[HELP],windowStatList[HELP].rows/2, "This licence is the default");
				printInCenter(windowList[HELP],windowStatList[HELP].rows/2 + 1, arginfo->licenceName);
			}
		}
		wborder(windowList[HELP], ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, 0, 0, 0, 0);
		wrefresh(windowList[HELP]);
		wrefresh(windowList[whichLocation]);
		doupdate();
	}
	bool licenceAccepted  = false;

	if (menuList[LICENCE] != NULL) {
		if (current_item(menuList[LICENCE]) == licenceItems[licenceYES])
			licenceAccepted = true;
	}
	for (c=0; c < nWindows; ++c)
		unpost_menu(menuList[c]);
	endwin(); /* get out of ncurses */
	err_set_exit(NULL);

	for (c = 0; c < nWindows; ++c)
		delwin(windowList[c]);

	if (somethingChanged) {
		outputValues(menuList[PRIMARY]);
		if (arginfo->outputLicenceRequest)
			fprintf(stderr,"%s=%s\n","ACCEPTED_LICENCE",(licenceAccepted) ? "true" : "false");
	}

	for (count = 0; count < arginfo->nElements; ++count)
		free_item(option_items[count]);

	for (count = 0; count < nWindows; ++count)
		if (menuList[count] != NULL)
			free_menu(menuList[count]);

	curr = arginfo->head;
	while (curr) {
		next = curr->next;
		free(curr->name);
		free(curr);
		curr = next;
	}
	free(windowList);
	free(menuList);
	free(arginfo);

	return ((somethingChanged) ? exitOK : exitCancel);
}
