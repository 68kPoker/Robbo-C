
#include <intuition/intuition.h>

#include <clib/graphics_protos.h>

#include "Cell.h"
#include "Editor.h"
#include "Blitter.h"
#include "Window.h"

WORD ed[ E_COUNT ] =
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
    T_BAT,
    T_CANNON,
    T_LASER,
    T_BLASTER,
    T_SURPRISE,
    T_DEBRIS,
    T_MAGNET_LEFT,
    T_MAGNET_RIGHT,
    T_ROBBO
};

VOID drawSelection( struct Window *w )
{
    WORD i;

    for( i = 0; i < E_COUNT; i++ )
    {        
        drawBob( map.gfx, ( types[ ed[ i ] ].base % 20 ) * TILE_WIDTH, ( types[ ed[ i ] ].base / 20 ) * TILE_HEIGHT, w->RPort, 1 + ( i % 5 ) * ( TILE_WIDTH + 1 ), 1 + ( i / 5 ) * ( TILE_HEIGHT + 1 ), TILE_WIDTH, TILE_HEIGHT, 0xca, 0xff );
    }
}
