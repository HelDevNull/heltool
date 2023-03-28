CC=gcc
CFLAGS=-Wall -Os -pthread

SRCDIR=src

PROGRAMS=heltool-mc heltool-u2t heltool-t2u

all: $(PROGRAMS)

heltool-mc: $(SRCDIR)/heltool-mc.c $(SRCDIR)/heltool-mc.h
	$(CC) $(CFLAGS) -o $@ $<

heltool-u2t: $(SRCDIR)/heltool-u2t.c $(SRCDIR)/heltool-u2t.h
	$(CC) $(CFLAGS) -o $@ $<

heltool-t2u: $(SRCDIR)/heltool-t2u.c $(SRCDIR)/heltool-t2u.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGRAMS)

