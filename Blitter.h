
/* Enchanted Blitter */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define MIN(a, b) (a) <= (b) ? (a) : (b)
#define MAX(a, b) (a) >= (b) ? (a) : (b)

IMPORT VOID planePickArray( struct BitMap *bm, UWORD width, UWORD height, UWORD row, UWORD rows, UWORD *pick, BOOL mask );
IMPORT UWORD planePick( struct BitMap *bm, WORD x, WORD y, UWORD width, UWORD height, BOOL mask );
IMPORT BOOL planeOnOff( struct BitMap *bm, UBYTE p, WORD x, WORD y, UWORD width, UWORD height, BOOL ones );
IMPORT BOOL planeMaskOnOff( struct BitMap *bm, UBYTE p, WORD x, WORD y, UWORD width, UWORD height, BOOL ones );

IMPORT VOID drawTile( struct BitMap *gfx, WORD sx, WORD sy, struct BitMap *dest, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm, UBYTE writeMask );
IMPORT VOID drawTileRastPort( struct BitMap *gfx, WORD sx, WORD sy, struct RastPort *rp, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm );

IMPORT VOID setBG( struct BitMap *gfx, WORD sx, WORD sy, struct BitMap *dest, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm, UBYTE writeMask );
IMPORT VOID drawBob( struct BitMap *gfx, WORD sx, WORD sy, struct BitMap *dest, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm, UBYTE writeMask );
