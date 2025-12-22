
# Robbo-C

cc = vc
link = vc
cflags = +aos68k -c
lflags = +aos68k -lamiga -o

all: Robbo.exe

.c.o:
  $(cc) $(cflags) $*.c

Game.exe: Window.o Type.o Blitter.o
  $(link) $(lflags) $@ $**

Window.o: Window.h Cell.h Blitter.h

Blitter.o: Blitter.h

Type.o: Cell.h
