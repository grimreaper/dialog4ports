#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <curses.h>
#include <menu.h>
#include <err.h>
#include <sysexits.h>
#include <sys/queue.h>


void convertArgs(int argc, char* argv[]);

enum OPTION_TYPE {
	CHECKBOX,
	RADIOBOX,
	USER_INPUT,
};

/*
	Used to track which mode the argument counter is in
*/
enum {
	pname,
	mode,
	value,
	descr,
} argmode;

struct OptionEl {
	char *name;
	char *options;
	char *descr;
	const char *value;		//this is user supplied
	enum OPTION_TYPE mode;
	TAILQ_ENTRY(OptionEl) ptrs;
} OptionElSample;

TAILQ_HEAD(OptionElHead, OptionEl) optionElHead;

const char* portname;

void
convertArgs(int argc, char* argv[])
{
	if (argc < 2)
		errx(EX_USAGE,"We require some option type to work");

	if (argc < 3)
		errx(EX_USAGE,"We need more than just a port name");

	struct OptionEl *currentArgument;

	argmode = mode;

	for (int i = 1; i < argc; ++i) {
	 	currentArgument = malloc(sizeof(OptionElSample));
		if (strcmp(argv[i], "--portname") == 0) {
			printf("Switching to portname mode\n");
			argmode = pname;
		}
		else if (strcmp(argv[i],"--check") == 0) {
			printf("Switching to checkbox mode\n");
			argmode = value;
			currentArgument->mode = CHECKBOX;
		}
		else if (strcmp(argv[i],"--radio") == 0) {
			printf("Switching to radio mode\n");
			argmode = value;
			currentArgument->mode = RADIOBOX;
		}
		else if (strcmp(argv[i],"--input") == 0) {
			printf("Switching to input mode\n");
			argmode = value;
			currentArgument->mode = USER_INPUT;
		}
		else if ( argmode == pname )
		{
			portname = argv[i];
			argmode = mode;
		}
		else
		{
			currentArgument->value = argv[i];
			argmode = mode;
		}

 		TAILQ_INSERT_TAIL(&optionElHead, currentArgument, ptrs);
 	}

}


int
main(int argc, char* argv[])
{
	TAILQ_INIT(&optionElHead);

	convertArgs(argc, argv);

      struct OptionEl *currentArgument;
	TAILQ_FOREACH(currentArgument, &optionElHead, ptrs) {
		TAILQ_REMOVE(&optionElHead,currentArgument, ptrs);
		free(currentArgument);
 	}

	return (0);
}

