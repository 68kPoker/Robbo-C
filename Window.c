
#include <stdio.h>

#include <intuition/intuition.h>

#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/layers_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>

#include "Cell.h"

#define ESC_KEY   0x45
#define F1_KEY    0x50
#define UP_KEY    0x4C
#define DOWN_KEY  0x4D
#define RIGHT_KEY 0x4E
#define LEFT_KEY  0x4F

#define VERS 39L

#define TILE_WIDTH  32
#define TILE_HEIGHT 16

#define VIEW_WIDTH  16
#define VIEW_HEIGHT 10

struct Library *IntuitionBase, *GfxBase, *LayersBase;

WORD tiles[ VIEW_HEIGHT ][ VIEW_WIDTH ];

struct Window *openWindow( struct Region **reg )
{
    struct Window *w;

    if( w = OpenWindowTags( NULL,
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
    return( NULL );
}

VOID closeWindow( struct Window *w, struct Region *reg )
{
    InstallClipRegion( w->WLayer, NULL );
    DisposeRegion( reg );
    CloseWindow( w );
}

VOID drawTile( WORD tile, struct Window *w, WORD x, WORD y )
{
    UBYTE text[ 4 ] = "   ";
    struct RastPort *rp = w->RPort;

    if( tile )
    {
        sprintf( text, "%3d", tile );
    }

    SetAPen( rp, tile );
    RectFill( rp, w->BorderLeft + x, w->BorderTop + y, w->BorderLeft + x + TILE_WIDTH - 1, w->BorderTop + y + TILE_HEIGHT - 1 );
    
    Move( rp, w->BorderLeft + x, w->BorderTop + 4 + rp->Font->tf_Baseline + y );
    SetABPenDrMd( rp, ( tile & 0x0f ) == 1 ? 0 : 1, 0, JAM1 );
    Text( rp, text, 3 );
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

            if( tile != tiles[ y ][ x ] || force )
            {
                tiles[ y ][ x ] = tile;
                drawTile( tile, w, x * TILE_WIDTH, y * TILE_HEIGHT );
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

        if( w = openWindow( &reg ) )
        {
            BOOL done = FALSE;
            WORD dx = 0, dy = 0, tx = 0, ty = 0;

            initTypes();
            initMap();
            map.map[ 1 ][ 1 ].type = T_ROBBO;
            map.pos = 1 * WIDTH + 1;
            map.map[ 4 ][ 4 ].type = T_BOX;
            map.map[ 4 ][ 8 ].type = T_WHEELED_BOX;
            map.map[ 8 ][ 8 ].type = T_KEY;
            map.map[ 2 ][ 1 ].type = T_DOOR;
            map.map[ 2 ][ 2 ].type = T_WALL;
            map.map[ 2 ][ 3 ].type = T_WALL;
            map.map[ 3 ][ 6 ].type = T_AMMO;
            map.map[ 5 ][ 5 ].type = T_DEBRIS;
            map.map[ 8 ][ 1 ].type = T_CANNON;            
            map.map[ 8 ][ 1 ].dir = RIGHT;
            map.map[ 8 ][ 14 ].type = T_LASER;
            map.map[ 8 ][ 14 ].dir = LEFT;
            map.map[ 1 ][ 7 ].type = T_BAT;
            map.map[ 1 ][ 7 ].dir = DOWN;
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
            closeWindow( w, reg );
        }
        CloseLibrary( LayersBase );
        CloseLibrary( GfxBase );
        CloseLibrary( IntuitionBase );
    }
    return( 0 );
}
