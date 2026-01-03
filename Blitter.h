
/* Enchanted Blitter */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define MIN(a, b) (a) <= (b) ? (a) : (b)
#define MAX(a, b) (a) >= (b) ? (a) : (b)

#ifdef __VBCC__
#define REG(r) __reg(#r)
#elif __DICE__
#define REG(r) register __##r
#else
#define REG(r)
#endif

IMPORT VOID planePickArray( struct BitMap *bm, UWORD width, UWORD height, UWORD row, UWORD rows, UWORD *pick, BOOL mask );
IMPORT UWORD planePick( struct BitMap *bm, WORD x, WORD y, UWORD width, UWORD height, BOOL mask );
IMPORT BOOL planeOnOff( struct BitMap *bm, UBYTE p, WORD x, WORD y, UWORD width, UWORD height, BOOL ones );
IMPORT BOOL planeMaskOnOff( struct BitMap *bm, UBYTE p, WORD x, WORD y, UWORD width, UWORD height, BOOL ones );

IMPORT VOID drawTileStd( struct BitMap *gfx, WORD sx, WORD sy, struct BitMap *dest, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm, UBYTE writeMask );
IMPORT VOID drawTile( struct BitMap *gfx, WORD sx, WORD sy, struct BitMap *dest, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm, UBYTE writeMask );
IMPORT VOID drawTileRastPort( struct BitMap *gfx, WORD sx, WORD sy, struct RastPort *rp, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm );

IMPORT VOID setBG( struct BitMap *gfx, WORD sx, WORD sy, struct BitMap *dest, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm, UBYTE writeMask );
IMPORT VOID drawBob( struct BitMap *gfx, WORD sx, WORD sy, struct BitMap *dest, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm, UBYTE writeMask );

IMPORT VOID writeTileLine( REG( a0 ) PLANEPTR src[], REG( a1 ) PLANEPTR dest, REG( d0 ) WORD wordWidth, REG( d1 ) WORD height, REG( d2 ) WORD srcMod, WORD REG( d3 ) destMod, REG( d4 ) WORD len );
