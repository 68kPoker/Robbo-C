
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define DELAY 6
#define WIDTH 16
#define HEIGHT 20

#define LEFT -1
#define RIGHT 1
#define UP -WIDTH
#define DOWN WIDTH

#define AMMO_PACK 8

#define enterCell(cell, as, dir, frame) types[(cell)->type].enter(cell, as, dir, frame)

enum
{
    T_SPACE,
    T_WALL,

    T_CAPSULE,

    T_SCREW,
    T_KEY,
    T_AMMO,

    T_DOOR,

    T_BOX,
    T_WHEELED_BOX,

    T_BOMB,
    T_BOMB_EXPLODING,
    T_BAT,

    T_CANNON,
    T_LASER,
    T_BLASTER,

    T_SURPRISE,

    T_DEBRIS,

    T_MAGNET_RIGHT,
    T_MAGNET_LEFT,
    T_BULLET,
    T_BEAM,
    T_BEAM_SHRINK,
    T_BEAM_EXTEND,

    T_ROBBO,
    T_BLANK,
    T_EXPLOSION,
    T_FIRE,
    T_STREAM,

    T_COUNT
};

typedef struct Cell
{
    WORD type;
    WORD dir;
    WORD frame;

    WORD delay;
    BOOL scanned;
    WORD index;
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
    WORD pos;
    WORD toggle;
    struct BitMap *gfx;
    struct RastPort *rp;
    WORD block;
    BOOL done;
    struct BitMap *back, *gfxBlit;
    WORD dx, dy;
} Map;

typedef BOOL Enter( Cell *cell, WORD as, WORD dir, WORD frame );

typedef VOID Scan( Cell *cell );

typedef struct Type
{
    Enter *enter;
    Scan *scan;
    WORD base, count;
    WORD *frames;
    WORD dirs;
} Type;

extern Map map;

extern Type types[ T_COUNT ];

extern VOID initType( WORD type, Enter *enter, Scan *scan, WORD base, WORD dirs );

extern VOID sumBase( VOID );

extern VOID initTypes( VOID );

extern VOID updateCell( Cell *cell, WORD type, WORD dir, WORD frame );

extern VOID scanMap( VOID );

extern VOID initMap( VOID );
