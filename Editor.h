
#ifndef EDITOR_H
#define EDITOR_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef CONST_H
#include "Const.h"
#endif

#ifndef CELL_H
#include "Cell.h"
#endif

enum
{
    E_SPACE,
    E_WALL,
    E_CAPSULE,
    E_SCREW,
    E_KEY,
    E_AMMO,
    E_DOOR,
    E_BOX,
    E_WHEELED_BOX,
    E_BOMB,
    E_BAT,
    E_CANNON,
    E_LASER,
    E_BLASTER,
    E_SURPRISE,
    E_DEBRIS,
    E_MAGNET_LEFT,
    E_MAGNET_RIGHT,    
    E_COUNT
};

typedef struct EdCell
{
    BYTE type, dir, index;
} EdCell;

struct EdHeader
{
    WORD version;
    UBYTE width, height;
    UBYTE x, y;
};

struct Editor
{
    WORD select;
    struct EdHeader head;
    EdCell map[ HEIGHT ][ WIDTH ];
};

IMPORT WORD ed[ E_COUNT ];

IMPORT VOID drawSelection( struct Window *w );

#endif
