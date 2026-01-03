
# Robbo-C

cc = vc
link = vc
cflags = +aos68k -O=15 -c
lflags = +aos68k -lamiga -o

all: Robbo.exe

.c.o:
  $(cc) $(cflags) $*.c

Robbo.exe: Window.o Type.o Editor.o Blitter.o
  $(link) $(lflags) $@ $**

Window.o: Window.h Cell.h Blitter.h Editor.h Const.h

Editor.o: Editor.h Window.h Cell.h Const.h

Blitter.o: Blitter.h

Type.o: Cell.h Editor.h Const.h
