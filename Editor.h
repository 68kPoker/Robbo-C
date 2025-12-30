
#ifndef EXEC_TYPES_H
#include <exec/types.h>
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
    E_ROBBO,
    E_COUNT
};

struct EdCell
{
    BYTE type, dir, index;
};

struct EdHeader
{
    WORD version;
    UBYTE width, height;
};

struct Editor
{
    WORD select;
    struct EdHeader head;
    struct EdCell map[ HEIGHT ][ WIDTH ];
};

IMPORT WORD ed[ E_COUNT ];

IMPORT VOID drawSelection( struct Window *w );
