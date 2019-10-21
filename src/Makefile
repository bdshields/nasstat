IDIR =.
CC=gcc
CFLAGS=-I$(IDIR) -D_GNU_SOURCE -Werror=implicit

ODIR=obj
LDIR =../lib

LIBS=-lm

_DEPS = config_file.h lcd_api.h widget_ids.h md_screen.h utils.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o config_file.o lcd_api.o md_screen.o utils.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: nasstat

nasstat: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
