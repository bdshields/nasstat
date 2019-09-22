IDIR =.
CC=gcc
CFLAGS=-I$(IDIR) -D_GNU_SOURCE

ODIR=obj
LDIR =../lib

LIBS=-lm

_DEPS = config_file.h lcd_api.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o config_file.o lcd_api.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nasstat: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
