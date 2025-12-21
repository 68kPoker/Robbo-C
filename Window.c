
#include <stdio.h>

#include <intuition/intuition.h>
#include <datatypes/pictureclass.h>

#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/layers_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/datatypes_protos.h>

#include "Cell.h"

#define ESC_KEY   0x45
#define F1_KEY    0x50
#define UP_KEY    0x4C
#define DOWN_KEY  0x4D
#define RIGHT_KEY 0x4E
#define LEFT_KEY  0x4F

#define VERS 39L

#define TILE_WIDTH  16
#define TILE_HEIGHT 16

#define VIEW_WIDTH  16
#define VIEW_HEIGHT 10

struct Library *IntuitionBase, *GfxBase, *LayersBase, *DataTypesBase;

WORD tiles[ VIEW_HEIGHT ][ VIEW_WIDTH ];

struct BitMap *gfx;

struct Window *openWindow( struct Region **reg )
{
    struct Screen *s;
    WORD pens[] = { ~0 };

    if( s = OpenScreenTags( NULL,
        SA_DisplayID, LORES_KEY,
        SA_Pens, pens,
        SA_SharePens, TRUE,
        SA_Title, "Robbo-C",
        SA_Depth, 5,
        TAG_DONE ) )
    {
        struct Window *w;

        if( w = OpenWindowTags( NULL,
            WA_CustomScreen, s,
            WA_Left, (s->Width - 256) /2,
            WA_Top, (s->Height - 160) /2,
            WA_InnerWidth, VIEW_WIDTH * TILE_WIDTH,
            WA_InnerHeight, VIEW_HEIGHT * TILE_HEIGHT,
            WA_Title, "Robbo-C",
            WA_ScreenTitle, "Robbo-C (c) 2008/2025 Robert Szacki",
            WA_DragBar, TRUE,
            WA_DepthGadget, TRUE,
            WA_CloseGadget, TRUE,
            WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY,
            WA_ReportMouse, TRUE,
            WA_Activate, TRUE,
            WA_SimpleRefresh, TRUE,
            WA_BackFill, LAYERS_BACKFILL,
            TAG_DONE ) )
        {
            if( *reg = NewRegion() )
            {
                struct Rectangle r;

                r.MinX = w->BorderLeft;
                r.MinY = w->BorderTop;
                r.MaxX = w->BorderLeft + w->GZZWidth - 1;
                r.MaxY = w->BorderTop + w->GZZHeight - 1;
                OrRectRegion( *reg, &r );
                InstallClipRegion( w->WLayer, *reg );
                return( w );
            }
            CloseWindow( w );
        }
        CloseScreen( s );
    }
    return( NULL );
}

VOID closeWindow( struct Window *w, struct Region *reg )
{
    struct Screen *s = w->WScreen;

    InstallClipRegion( w->WLayer, NULL );
    DisposeRegion( reg );
    CloseWindow( w );
    CloseScreen( s );
}

VOID drawTile( WORD type, WORD tile, struct Window *w, WORD x, WORD y )
{
    struct RastPort *rp = w->RPort;

    BltBitMapRastPort( gfx, ( tile % 20 ) * TILE_WIDTH, ( tile / 20 ) * TILE_HEIGHT, w->RPort, w->BorderLeft + x, w->BorderTop + y, TILE_WIDTH, TILE_HEIGHT, 0xc0 );
}

VOID drawMap( struct Window *w, WORD dx, WORD dy, BOOL force )
{
    WORD x, y;

    for( y = 0; y < VIEW_HEIGHT; y++ )
    {
        for( x = 0; x < VIEW_WIDTH; x++ )
        {
            Cell *cell = &map.map[ y + dy ][ x + dx ];
            Type *ptr = types + cell->type;
            WORD tile = ptr->base;

            if( ptr->frames )
            {
                tile += ptr->frames[ cell->frame ];
            }
            else
            {
                tile += cell->frame;
            }

            if( ptr->dirs )
            {
                WORD i;

                if( cell->dir == LEFT )
                {
                    i = 0;
                }
                else if( cell->dir == RIGHT )
                {
                    i = 1;
                }
                else if( cell->dir == UP )
                {
                    i = 2;
                }
                else
                {
                    i = 3;
                }
                tile += i * ptr->count;
            }

            if( tile != tiles[ y ][ x ] || force )
            {
                tiles[ y ][ x ] = tile;
                drawTile( cell->type, tile, w, x * TILE_WIDTH, y * TILE_HEIGHT );
            }
        }
    }
}

int main( void )
{
    struct Window *w;
    struct Region *reg;

    if( IntuitionBase = OpenLibrary( "intuition.library", VERS ) )
    {
        GfxBase = OpenLibrary( "graphics.library", VERS );
        LayersBase = OpenLibrary( "layers.library", VERS );
        if( DataTypesBase = OpenLibrary( "datatypes.library", VERS ) )
        {
            if( w = openWindow( &reg ) )
            {
                Object *o;
                if( o = NewDTObject( "Robbo.iff",
                    DTA_GroupID, GID_PICTURE,
                    PDTA_Screen, w->WScreen,
                    PDTA_Remap, FALSE,                    
                    TAG_DONE ) )
                {
                    BOOL done = FALSE;
                    WORD dx = 0, dy = 0, tx = 0, ty = 0;                    
                    WORD i;
                    ULONG *colors;

                    GetDTAttrs( o, PDTA_CRegs, &colors, TAG_DONE );
                    for( i = 0; i < 32; i++ )
                    {
                        SetRGB32CM( w->WScreen->ViewPort.ColorMap, i, colors[ 0 ], colors[ 1 ], colors[ 2 ] );
                        colors += 3;
                    }
                    MakeScreen( w->WScreen );
                    RethinkDisplay();

                    DoDTMethod( o, NULL, NULL, DTM_PROCLAYOUT, NULL, TRUE );

                    GetDTAttrs( o, PDTA_DestBitMap, &gfx, TAG_DONE );

                    initTypes();
                    initMap();
                    map.map[ 1 ][ 1 ].type = T_ROBBO;
                    map.pos = 1 * WIDTH + 1;
                    map.map[ 4 ][ 4 ].type = T_DEBRIS;
                    map.map[ 4 ][ 9 ].type = T_BAT;
                    map.map[ 4 ][ 9 ].dir = LEFT;
                    map.map[ 4 ][ 8 ].type = T_BOX;
                    map.map[ 5 ][ 7 ].type = T_BOX;
                    map.map[ 6 ][ 6 ].type = T_BOX;
                    map.map[ 1 ][ 13 ].type = T_KEY;
                    map.map[ 2 ][ 1 ].type = T_DOOR;
                    for( i = 2; i < 15; i++ )
                    {
                        map.map[ 2 ][ i ].type = T_WALL;
                    }
                    map.map[ 3 ][ 6 ].type = T_SCREW;
                    map.map[ 2 ][ 2 ].type = T_WALL;
                    map.map[ 2 ][ 3 ].type = T_WALL;
                    map.map[ 3 ][ 7 ].type = T_AMMO;
                    map.map[ 5 ][ 5 ].type = T_DEBRIS;
                    map.map[ 8 ][ 1 ].type = T_CANNON;
                    map.map[ 8 ][ 1 ].dir = RIGHT;
                    map.map[ 8 ][ 14 ].type = T_LASER;
                    map.map[ 8 ][ 14 ].dir = LEFT;
                    map.map[ 7 ][ 7 ].type = T_BAT;
                    map.map[ 7 ][ 7 ].dir = DOWN;
                    map.map[ 4 ][ 1 ].type = T_BLASTER;
                    map.map[ 4 ][ 1 ].dir = RIGHT;
                    drawMap( w, dx, dy, TRUE );
                    while( !done )
                    {
                        ULONG mask = 1L << w->UserPort->mp_SigBit;
                        WaitTOF();
                        scanMap();

                        if( ( ( map.pos / WIDTH ) - dy ) < 1 || ( ( map.pos / WIDTH ) - dy ) > VIEW_HEIGHT - 2 )
                        {
                            ty = ( map.pos / WIDTH ) - ( VIEW_HEIGHT / 2 );

                            if( ty < 0 )
                            {
                                ty = 0;
                            }
                            else if( ty > HEIGHT - VIEW_HEIGHT )
                            {
                                ty = HEIGHT - VIEW_HEIGHT;
                            }
                        }

                        if( ty > dy )
                        {
                            dy++;
                        }
                        else if( ty < dy )
                        {
                            dy--;
                        }

                        drawMap( w, dx, dy, FALSE );
                        if( SetSignal( 0L, mask ) & mask )
                        {
                            struct IntuiMessage *msg;
                            while( msg = ( struct IntuiMessage * )GetMsg( w->UserPort ) )
                            {
                                ULONG cls = msg->Class;
                                UWORD code = msg->Code;
                                WORD mx = msg->MouseX;
                                WORD my = msg->MouseY;
                                UWORD qual = msg->Qualifier;

                                ReplyMsg( ( struct Message * )msg );

                                if( cls == IDCMP_CLOSEWINDOW )
                                {
                                    done = TRUE;
                                }
                                else if( cls == IDCMP_RAWKEY )
                                {
                                    if( code == LEFT_KEY )
                                    {
                                        map.dir = LEFT;
                                    }
                                    else if( code == ( LEFT_KEY | IECODE_UP_PREFIX ) )
                                    {
                                        if( map.dir == LEFT )
                                        {
                                            map.dir = 0;
                                        }
                                    }
                                    if( code == RIGHT_KEY )
                                    {
                                        map.dir = RIGHT;
                                    }
                                    else if( code == ( RIGHT_KEY | IECODE_UP_PREFIX ) )
                                    {
                                        if( map.dir == RIGHT )
                                        {
                                            map.dir = 0;
                                        }
                                    }
                                    if( code == UP_KEY )
                                    {
                                        map.dir = UP;
                                    }
                                    else if( code == ( UP_KEY | IECODE_UP_PREFIX ) )
                                    {
                                        if( map.dir == UP )
                                        {
                                            map.dir = 0;
                                        }
                                    }
                                    if( code == DOWN_KEY )
                                    {
                                        map.dir = DOWN;
                                    }
                                    else if( code == ( DOWN_KEY | IECODE_UP_PREFIX ) )
                                    {
                                        if( map.dir == DOWN )
                                        {
                                            map.dir = 0;
                                        }
                                    }
                                    if( qual & ( IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT ) )
                                    {
                                        map.fire = TRUE;
                                    }
                                    else
                                    {
                                        map.fire = FALSE;
                                    }
                                }
                            }
                        }
                    }
                    DisposeDTObject( o );
                }
                closeWindow( w, reg );
            }
            CloseLibrary( DataTypesBase );
        }
        CloseLibrary( LayersBase );
        CloseLibrary( GfxBase );
        CloseLibrary( IntuitionBase );
    }
    return( 0 );
}
