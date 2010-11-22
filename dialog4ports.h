#define _POSIX_C_SOURCE
#include <menu.h>
#include <ncurses.h>
#include <panel.h>

#include <stdbool.h>

enum OPTION_TYPE {
	CHECKBOX,
	RADIOBOX,
	USER_INPUT
};

struct list_el {
	const char *descr;
	const char *longDescrFile;
	const char *longDescrText;
	enum OPTION_TYPE mode;
	char *name;
	struct list_el *next;
	const char *options;
	bool required;
	const char *value;		/* this is user supplied */
};

typedef struct list_el OptionEl;

static char *
getString(WINDOW *win, const char * const curVal);

static unsigned int
countChar ( const char * const input, const char c );

void
outputBinaryValue(ITEM* item, const char *key);

void
outputValues(MENU *menu);

/*
	this is fugly
*/
struct ARGINFO {
	OptionEl *head;
	const char *licenceName;
	const char *licenceText;
	unsigned int nElements;
	bool outputLicenceRequest;
	const char *portname;
	const char *portcomment;
};

struct ARGINFO*
parseArguments(const int argc, char * argv[]);

/* returns the starting location for the message */
int
printInCenter(WINDOW * const win, const int row, const char * const str);

void
printFileToWindow(WINDOW * const win, const char * const filename);

static void
usage();

bool
requiredItemsSelected(ITEM **items);

void
fixEnabledOptions(ITEM** option_items, int myIndex);

/*void
runMeOnMenuCall(MENU *menu);*/

const char * const preNameToken = "? ? ";

struct windowStats {
	int cols;
	int rows;
	int colStart;
	int rowStart;
};

void
cleanNcursesExit(const int n);
