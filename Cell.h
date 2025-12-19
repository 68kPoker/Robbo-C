
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define DELAY 16
#define WIDTH 16
#define HEIGHT 32

#define enterCell(cell, as, dir, frame) types[(cell)->type].enter(cell, as, dir, frame)

enum
{
    T_SPACE,
    T_WALL,

    T_SCREW,
    T_AMMO,
    T_KEY,

    T_BOX,
    T_WHEELED_BOX,

    T_DOOR,
    T_DEBRIS,

    T_ROBBO,

    T_CAPSULE,

    T_BULLET,

    T_FIRE,
    T_EXPLOSION,

    T_COUNT
};

typedef struct Cell
{
    WORD type;
    WORD dir;
    WORD frame;

    WORD delay;
    BOOL scanned;
} Cell;

typedef struct Map
{
    Cell map[ HEIGHT ][ WIDTH ];
    Cell *scanned;
    WORD collected, required;
    WORD ammo;
    WORD keys;
    WORD dir;
    BOOL fire;
} Map;

typedef BOOL Enter( Cell *cell, WORD as, WORD dir, WORD frame );

typedef VOID Scan( Cell *cell );

typedef struct Type
{
    Enter *enter;
    Scan *scan;
    WORD base;
    WORD *frames;
} Type;

extern Map map;

extern Type types[ T_COUNT ];

extern VOID initType( WORD type, Enter *enter, Scan *scan, WORD base );

extern VOID sumBase( VOID );

extern VOID initTypes( VOID );

extern VOID updateCell( Cell *cell, WORD type, WORD dir, WORD frame );
