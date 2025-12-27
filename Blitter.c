
/* Enchanted Blitter */

#include <hardware/blit.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>

#include <graphics/rastport.h>
#include <graphics/clip.h>

#include <clib/graphics_protos.h>
#include <clib/layers_protos.h>

#include "Blitter.h"

#ifndef WIN32
__far
#endif

IMPORT struct Custom custom;

/* Determine all 0s (minterm 0xF0) or 1s (minterm 0x0F) in bitplane without mask plane */

BOOL planeOnOff( struct BitMap *bm, UBYTE p, WORD x, WORD y, UWORD width, UWORD height, BOOL ones )
{
    REGISTER struct Custom *c = &custom;
    UWORD span;
    BOOL result;
    UBYTE minterm = ones ? 0x0F : 0xF0;

    OwnBlitter();

    span = ( ( x + width - 1 ) >> 4 ) - ( x >> 4 ) + 1;

    WaitBlit();

    c->bltcon0 = SRCA | minterm;
    c->bltcon1 = 0;
    c->bltapt = bm->Planes[ p ] + y * bm->BytesPerRow + ( ( x >> 4 ) << 1 );
    c->bltamod = bm->BytesPerRow - ( span << 1 );
    c->bltafwm = 0xFFFF >> ( x & 0xF );
    c->bltalwm = 0xFFFF << ( 15 - ( ( x + width - 1 ) & 0xF ) );
    c->bltsize = ( height << HSIZEBITS ) | span;

    WaitBlit();

    result = custom.dmaconr & DMAF_BLTNZERO;

    DisownBlitter();

    return( result );
}

/* Determine all 0s (minterm 0xC0) or 1s (minterm 0x30) in bitplane with mask plane (last) */

BOOL planeMaskOnOff( struct BitMap *bm, UBYTE p, WORD x, WORD y, UWORD width, UWORD height, BOOL ones )
{
    REGISTER struct Custom *c = &custom;
    UWORD span;
    BOOL result;
    LONG offset;
    WORD mod;
    UBYTE depth = bm->Depth;
    UBYTE minterm = ones ? ANBC | ANBNC : ABC | ABNC;

    OwnBlitter();

    span = ( ( x + width - 1 ) >> 4 ) - ( x >> 4 ) + 1;
    offset = y * bm->BytesPerRow + ( ( x >> 4 ) << 1 );
    mod = bm->BytesPerRow - ( span << 1 );

    WaitBlit();

    c->bltcon0 = SRCA | SRCB | minterm;
    c->bltcon1 = 0;
    c->bltapt = bm->Planes[ depth - 1 ] + offset;
    c->bltbpt = bm->Planes[ p ] + offset;
    c->bltamod = mod;
    c->bltbmod = mod;
    c->bltafwm = 0xFFFF >> ( x & 0xF );
    c->bltalwm = 0xFFFF << ( 15 - ( ( x + width - 1 ) & 0xF ) );
    c->bltsize = ( height << HSIZEBITS ) | span;

    WaitBlit();

    result = custom.dmaconr & DMAF_BLTNZERO;

    DisownBlitter();

    return( result );
}

/* Determine PlanePick and PlaneOnOff of a bitmap with or without mask plane (last) */

UWORD planePick( struct BitMap *bm, WORD x, WORD y, UWORD width, UWORD height, BOOL mask )
{
    BOOL( *calcOnOff )( struct BitMap *bm, UBYTE p, WORD x, WORD y, UWORD width, UWORD height, BOOL ones );
    UBYTE pick = 0, onOff = 0;
    BYTE p, depth = ( BYTE )bm->Depth;

    if( mask )
    {
        depth--;
    }

    calcOnOff = mask ? planeMaskOnOff : planeOnOff;

    for( p = depth - 1; p >= 0; p-- )
    {
        pick <<= 1;
        onOff <<= 1;
        if( calcOnOff( bm, p, x, y, width, height, FALSE ) )
        {
            /* All 0s */
        }
        else if( calcOnOff( bm, p, x, y, width, height, TRUE ) )
        {
            /* All 1s */
            onOff |= 1;
        }
        else
        {
            pick |= 1;
        }
    }
    return( ( onOff << 8 ) | pick );
}

/* Determine PlanePick and PlaneOnOff for all tiles/bobs */

VOID planePickArray( struct BitMap *bm, UWORD width, UWORD height, UWORD row, UWORD rows, UWORD *pick, BOOL mask )
{
    WORD x, y;

    for( y = 0; y < rows; y++ )
    {
        for( x = 0; x < row; x++ )
        {
            *pick++ = planePick( bm, x * width, y * height, width, height, mask );
        }
    }
}

/* Draw single tile in BitMap */

VOID drawTile( struct BitMap *gfx, WORD sx, WORD sy, struct BitMap *dest, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm, UBYTE writeMask )
{
    REGISTER struct Custom *c = &custom;
    BYTE shift, mshift;
    UWORD reverse;
    UBYTE p, depth = dest->Depth;
    LONG gfxOffset, destOffset;
    WORD gfxMod, destMod;
    WORD gfxSpan, destSpan, span;
    UWORD firstMask, lastMask;
    WORD x;    

    OwnBlitter();

    gfxSpan = ( ( sx + width - 1 ) >> 4 ) - ( sx >> 4 ) + 1;
    destSpan = ( ( dx + width - 1 ) >> 4 ) - ( dx >> 4 ) + 1;

    shift = ( dx & 0xF ) - ( sx & 0xF );

    if( shift < 0 )
    {
        reverse = BLITREVERSE;
        shift = -shift;
    }
    else
    {
        reverse = 0;
    }

    if( gfxSpan >= destSpan )
    {
        span = gfxSpan;
        mshift = shift;
        x = sx;
    }
    else
    {
        span = destSpan;
        mshift = 0;
        x = dx;
    }

    firstMask = 0xFFFF >> ( x & 0xF );
    lastMask = 0xFFFF << ( 15 - ( ( x + width - 1 ) & 0xF ) );

    gfxOffset = sy * gfx->BytesPerRow + ( ( sx >> 4 ) << 1 );
    destOffset = dy * dest->BytesPerRow + ( ( dx >> 4 ) << 1 );
    gfxMod = gfx->BytesPerRow / depth - ( span << 1 );
    destMod = dest->BytesPerRow / depth - ( span << 1 );

    if( reverse )
    {
        UWORD temp;

        gfxOffset += ( height - 1 ) * gfx->BytesPerRow + ( ( span - 1 ) << 1 );
        destOffset += ( height - 1 ) * dest->BytesPerRow + ( ( span - 1 ) << 1 );

        temp = firstMask;
        firstMask = lastMask;
        lastMask = temp;
        p = depth - 1;
    }
    else
    {
        p = 0;
    }
#if 0
    writeMask &= ( 1 << dest->Depth ) - 1;

    for( p = 0; writeMask; p++, writeMask >>= 1 )
    {
        if( writeMask & 1 )
        {
#endif
            WaitBlit();

            c->bltcon0 = SRCB | SRCC | DEST | minterm | NABC | NANBC | ( mshift << ASHIFTSHIFT );
            c->bltcon1 = reverse | ( shift << BSHIFTSHIFT );
            c->bltadat = 0xFFFF;
            c->bltbpt = gfx->Planes[ p ] + gfxOffset;
            c->bltcpt = dest->Planes[ p ] + destOffset;
            c->bltdpt = dest->Planes[ p ] + destOffset;
            c->bltbmod = gfxMod;
            c->bltcmod = destMod;
            c->bltdmod = destMod;
            c->bltafwm = firstMask;
            c->bltalwm = lastMask;
            c->bltsize = ( ( height * depth ) << HSIZEBITS ) | span;
#if 0
        }
    }
#endif

    DisownBlitter();
}

VOID setBG( struct BitMap *gfx, WORD sx, WORD sy, struct BitMap *dest, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm, UBYTE writeMask )
{
    REGISTER struct Custom *c = &custom;
    UBYTE p;
    PLANEPTR mask;
    LONG gfxOffset, destOffset;
    WORD gfxMod, destMod;
    WORD span;
    UBYTE depth;

    OwnBlitter();

    depth = dest->Depth - 1;

    gfxOffset = sy * gfx->BytesPerRow + ( ( sx >> 4 ) << 1 );
    destOffset = dy * dest->BytesPerRow + ( ( dx >> 4 ) << 1 );

    span = width >> 4;

    gfxMod = gfx->BytesPerRow - ( span << 1 );
    destMod = dest->BytesPerRow - ( span << 1 );

    mask = dest->Planes[ depth ] + destOffset;

    writeMask &= ( 1 << depth ) - 1;

    for( p = 0; writeMask; p++, writeMask >>= 1 )
    {
        if( writeMask & 1 )
        {
            WaitBlit();
            c->bltcon0 = SRCA | SRCB | SRCC | DEST | minterm;
            c->bltcon1 = 0;
            c->bltapt = mask;
            c->bltbpt = gfx->Planes[ p ] + gfxOffset;
            c->bltcpt = dest->Planes[ p ] + destOffset;
            c->bltdpt = dest->Planes[ p ] + destOffset;
            c->bltamod = destMod;
            c->bltbmod = gfxMod;
            c->bltcmod = destMod;
            c->bltdmod = destMod;
            c->bltafwm = 0xFFFF;
            c->bltalwm = 0xFFFF;
            c->bltsize = ( height << HSIZEBITS ) | span;
        }
    }

    DisownBlitter();
}

VOID drawBob( struct BitMap *gfx, WORD sx, WORD sy, struct RastPort *rp, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm, UBYTE writeMask )
{
    REGISTER struct Custom *c = &custom;
    UBYTE p;
    PLANEPTR mask;
    LONG gfxOffset, destOffset;
    WORD gfxMod, destMod;
    WORD span;
    UBYTE depth;
    UBYTE shift;
    struct BitMap *dest;

    OwnBlitter();

    dest = rp->BitMap;

    if( rp->Layer )
    {
        dx += rp->Layer->bounds.MinX;
        dy += rp->Layer->bounds.MinY;
    }

    shift = dx & 0xF;

    depth = gfx->Depth - 1;

    gfxOffset = sy * gfx->BytesPerRow + ( ( sx >> 4 ) << 1 );
    destOffset = dy * dest->BytesPerRow + ( ( dx >> 4 ) << 1 );

    span = ( width >> 4 ) + 1;

    gfxMod = gfx->BytesPerRow - ( span << 1 );
    destMod = dest->BytesPerRow - ( span << 1 );

    mask = gfx->Planes[ depth ] + gfxOffset;

    writeMask &= ( 1 << depth ) - 1;

    for( p = 0; writeMask; p++, writeMask >>= 1 )
    {
        if( writeMask & 1 )
        {
            WaitBlit();
            c->bltcon0 = SRCA | SRCB | SRCC | DEST | minterm | ( shift << ASHIFTSHIFT );
            c->bltcon1 = shift << BSHIFTSHIFT;
            c->bltapt = mask;
            c->bltbpt = gfx->Planes[ p ] + gfxOffset;
            c->bltcpt = dest->Planes[ p ] + destOffset;
            c->bltdpt = dest->Planes[ p ] + destOffset;
            c->bltamod = gfxMod;
            c->bltbmod = gfxMod;
            c->bltcmod = destMod;
            c->bltdmod = destMod;
            c->bltafwm = 0xFFFF;
            c->bltalwm = 0;
            c->bltsize = ( height << HSIZEBITS ) | span;
        }
    }

    DisownBlitter();
}

VOID drawTileRastPort( struct BitMap *gfx, WORD sx, WORD sy, struct RastPort *rp, WORD dx, WORD dy, UWORD width, UWORD height, UBYTE minterm )
{
    struct BitMap *dest = rp->BitMap;
    struct Layer *layer = rp->Layer;
    struct ClipRect *clip;
    struct Rectangle *rect;

    if( layer == NULL )
    {
        drawTile( gfx, sx, sy, dest, dx, dy, width, height, minterm, rp->Mask );
        return;
    }

    LockLayer( 0, layer );

    rect = &layer->bounds;

    dx += rect->MinX;
    dy += rect->MinY;

    for( clip = layer->ClipRect; clip != NULL; clip = clip->Next )
    {
        WORD x0, y0, x1, y1, ox, oy;

        if( clip->lobs != NULL )
        {
            continue;
        }

        rect = &clip->bounds;

        x0 = MAX( rect->MinX, dx );
        y0 = MAX( rect->MinY, dy );
        x1 = MIN( rect->MaxX, dx + width - 1 );
        y1 = MIN( rect->MaxY, dy + height - 1 );

        if( x0 > x1 || y0 > y1 )
        {
            continue;
        }

        ox = x0 - dx;
        oy = y0 - dy;

        drawTile( gfx, sx + ox, sy + oy, dest, x0, y0, x1 - x0 + 1, y1 - y0 + 1, minterm, rp->Mask );
    }

    UnlockLayer( layer );
}
