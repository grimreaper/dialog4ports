#include <menu.h>
#include <ncurses.h>
#include <stdbool.h>

enum OPTION_TYPE {
	CHECKBOX,
	RADIOBOX,
	USER_INPUT
};

struct list_el {
	const char *descr;
	const char *longDescrFile;
	enum OPTION_TYPE mode;
	const char *name;
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
	unsigned int nHashMarks;
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
