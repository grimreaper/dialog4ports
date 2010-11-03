# To protect us from stupid errors (like running make nameclean in the root directory)
NAME=dialog4ports

CFLAGS = -pipe -O3

CC = clang
CFLAGS += -std=c99 
CFLAGS += -Wimplicit-function-declaration -Wbad-function-cast -Wdeclaration-after-statement
CFLAGS += -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes
CFLAGS += -Wmissing-declarations -Wnested-externs
CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -Wformat=2 -Wstrict-aliasing=2 -Wstrict-overflow=4
CFLAGS += -Wunused -Wunused-parameter -Wswitch-enum 
CFLAGS += -Winit-self -Wmissing-include-dirs -Wpointer-arith -Wconversion
CFLAGS += -Wfloat-equal -Wundef -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings
CFLAGS += -fabi-version=0 -funswitch-loops  #-fprefetch-loop-arrays #-funroll-loops 
CFLAGS += -Winline -Wmissing-noreturn -Wpacked -Wredundant-decls
CFLAGS += -Wno-write-strings -Waggregate-return -Winvalid-pch -Wlong-long
CFLAGS += -Wvariadic-macros -Wvolatile-register-var
CFLAGS += -Wmissing-format-attribute
#CFLAGS += -Wlogical-op -Wnormalized=nfc
CFLAGS += -Wimport -Wunused-macros

CFLAGS += -lmenu -lncurses 

#-Wunreachable-code is disabled for way too many false postives

INCLUDE_FILES = -I/usr/local/include
# Default includes...
CFLAGS += -isystem /usr/local/include
LDFLAGS = -L/usr/local/lib

PREFIX = /usr/local

nameclean: .NOTMAIN .USE .EXEC .IGNORE .PHONY
	rm -f ./$(NAME)
coreclean: .NOTMAIN .USE .EXEC .IGNORE .PHONY
	rm -f ./$(NAME).core
objclean: .NOTMAIN .USE .EXEC .IGNORE .PHONY
	rm -fv ./*.o
remake: .NOTMAIN .USE .EXEC .IGNORE .PHONY clean all

clean: .NOTMAIN .PHONY .IGNORE nameclean coreclean objclean

$(NAME):

all: $(NAME)

rebuild: .NOTMAIN .PHONY clean $(NAME)

check: .NOTMAIN
	splint -strict-lib -showcolumn -showfunc -strict *.c *.h
	lint -aabcehprsxH $(INCLUDE_FILES) *.c *.h
	rats -rw3 *
