enum OPTION_TYPE {
	CHECKBOX,
	RADIOBOX,
	USER_INPUT,
};

struct list_el {
	const char *name;
	const char *options;
	const char *descr;
	const char *value;		//this is user supplied
	const char *longDescrFile;
	enum OPTION_TYPE mode;
	struct list_el *next;
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

struct {
	unsigned int nElements;
	unsigned int nHashMarks;
	OptionEl * head;
	const char * portname;
	const char * portcomment;
} arginfo;

void
parseArguments(const int argc, char * argv[]);

