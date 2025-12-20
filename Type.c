
#include <stdlib.h>

#include "Cell.h"

Map map;

Type types[ T_COUNT ];

VOID initType( WORD type, Enter *enter, Scan *scan, WORD base )
{
    Type *ptr = types + type;

    ptr->enter = enter;
    ptr->scan = scan;
    ptr->base = base;
}

VOID sumBase( VOID )
{
    WORD type;
    WORD sum = 0, base;

    for( type = 0; type < T_COUNT; type++ )
    {
        Type *ptr = types + type;

        base = ptr->base;
        ptr->base = sum;
        sum += base;
    }
}

STATIC BOOL enterSpace( Cell *cell, WORD as, WORD dir, WORD frame )
{
    updateCell( cell, as, dir, frame );
    return( TRUE );
}

STATIC BOOL enterWall( Cell *cell, WORD as, WORD dir, WORD frame )
{
    return( FALSE );
}

STATIC BOOL enterScrew( Cell *cell, WORD as, WORD dir, WORD frame )
{
    if( as == T_ROBBO )
    {
        map.collected++;
        updateCell( cell, as, dir, frame );
        return( TRUE );
    }
    return( FALSE );
}

STATIC BOOL enterAmmo( Cell *cell, WORD as, WORD dir, WORD frame )
{
    if( as == T_ROBBO )
    {
        map.ammo += AMMO_PACK;
        updateCell( cell, as, dir, frame );
        return( TRUE );
    }
    return( FALSE );
}

STATIC BOOL enterKey( Cell *cell, WORD as, WORD dir, WORD frame )
{
    if( as == T_ROBBO )
    {
        map.keys++;
        updateCell( cell, as, dir, frame );
        return( TRUE );
    }
    return( FALSE );
}

STATIC BOOL enterDoor( Cell *cell, WORD as, WORD dir, WORD frame )
{
    if( as == T_ROBBO )
    {
        if( map.keys > 0 )
        {
            map.keys--;
            updateCell( cell, T_SPACE, 0, 0 );
        }
    }
    return( FALSE );
}

STATIC BOOL enterBox( Cell *cell, WORD as, WORD dir, WORD frame )
{
    if( as == T_ROBBO )
    {
        WORD type = cell->type;
        Cell *dest = cell + dir;

        if( enterCell( dest, type, dir, cell->frame ) )
        {
            updateCell( cell, as, dir, frame );
            return( TRUE );
        }
    }
    return( FALSE );
}

STATIC VOID scanWheeledBox( Cell *cell )
{
    WORD dir = cell->dir;

    if( dir )
    {
        WORD type = cell->type;
        Cell *dest = cell + dir;

        if( enterCell( dest, type, dir, cell->frame ) )
        {
            updateCell( cell, T_SPACE, 0, 0 );
        }
        else
        {
            cell->dir = 0;
        }
    }
}

STATIC VOID scanRobbo( Cell *cell )
{
    WORD dir = map.dir;

    if( dir )
    {
        cell->delay = DELAY;
        if( !map.fire )
        {
            WORD type = cell->type;
            Cell *dest = cell + dir;
            if( enterCell( dest, type, dir, cell->frame ) )
            {
                map.pos += dir;
                updateCell( cell, T_SPACE, 0, 0 );
            }
        }
        else
        {
            if( map.ammo > 0 )
            {
                Cell *dest = cell + dir;
                map.ammo--;
                enterCell( dest, T_BULLET, dir, 0 );
            }
        }
    }
}

STATIC VOID scanBullet( Cell *cell )
{
    WORD dir = cell->dir;
    WORD type = cell->type;
    Cell *dest = cell + dir;

    if( enterCell( dest, type, dir, cell->frame ) )
    {
        updateCell( cell, T_SPACE, 0, 0 );
    }
    else
    {
        updateCell( cell, T_FIRE, 0, 0 );
    }
}

STATIC VOID scanFire( Cell *cell )
{
    cell->delay = DELAY;
    if( cell->frame < 2 )
    {
        cell->frame++;
    }
    else
    {
        updateCell( cell, T_SPACE, 0, 0 );
    }
}

STATIC VOID scanExplosion( Cell *cell )
{
    cell->delay = DELAY;
    if( cell->frame < 4 )
    {
        cell->frame++;
    }
    else
    {
        updateCell( cell, T_SPACE, 0, 0 );
    }
}

STATIC BOOL enterDebris( Cell *cell, WORD as, WORD dir, WORD frame )
{
    if( as == T_BULLET || as == T_BEAM_EXTEND )
    {
        updateCell( cell, T_EXPLOSION, 0, 0 );
    }
    else if( as == T_STREAM )
    {
        updateCell( cell, as, dir, frame );
    }
    return( FALSE );
}

STATIC VOID scanCannon( Cell *cell )
{
    WORD rnd = rand() & 0xff;

    cell->delay = DELAY;

    if( rnd < 0x10 )
    {
        enterCell( cell + cell->dir, T_BULLET, cell->dir, 0 );
    }
}

STATIC VOID scanBat( Cell *cell )
{
    WORD dir = cell->dir;
    Cell *dest = cell + dir;

    if( enterCell( dest, cell->type, dir, cell->frame ) )
    {
        updateCell( cell, T_SPACE, 0, 0 );
    }
    else
    {
        cell->dir = -cell->dir;
    }
}

STATIC VOID scanLaser( Cell *cell )
{
    WORD rnd = rand() & 0xff;

    cell->delay = DELAY;

    if( rnd < 0x10 )
    {
        enterCell( cell + cell->dir, T_BEAM_EXTEND, cell->dir, 0 );
    }
}

STATIC VOID scanBeamExtend( Cell *cell )
{
    WORD dir = cell->dir;
    WORD type = cell->type;
    Cell *dest = cell + dir;

    if( enterCell( dest, type, dir, cell->frame ) )
    {
        updateCell( cell, T_BEAM, dir, 0 );
    }
    else
    {
        updateCell( cell, T_BEAM_SHRINK, dir, 0 );
    }
}

STATIC VOID scanBeamShrink( Cell *cell )
{
    WORD dir = cell->dir;
    WORD type = cell->type;
    Cell *dest = cell - dir;

    if( dest->type == T_LASER )
    {
        updateCell( cell, T_FIRE, 0, 0 );
    }
    else
    {
        updateCell( cell, T_SPACE, 0, 0 );
        updateCell( dest, T_BEAM_SHRINK, dir, 0 );
    }
}

STATIC VOID scanBlaster( Cell *cell )
{
    WORD rnd = rand() & 0xff;

    cell->delay = DELAY;

    if( cell->index == 0 )
    {
        if( rnd < 0x10 )
        {
            cell->index = 1;
        }
    }
    if( cell->index > 0 )
    {
        enterCell( cell + cell->dir, T_STREAM, cell->dir, cell->index - 1 );
        if( ++cell->index == 6 )
        {
            cell->index = 0;
        }
    }
}

STATIC VOID scanStream( Cell *cell )
{
    Cell *dest = cell + cell->dir;

    enterCell( dest, cell->type, cell->dir, cell->frame );
    updateCell( cell, T_SPACE, 0, 0 );
}

STATIC BOOL enterStream( Cell *cell, WORD as, WORD dir, WORD frame )
{
    if( as == T_STREAM )
    {
        Cell *dest = cell + dir;
        enterCell( dest, cell->type, dir, cell->frame );
        updateCell( cell, as, dir, frame );
    }
    return( FALSE );
}

VOID initTypes( VOID )
{
    static WORD streamFrames[ 5 ] = { 0, 1, 2, 1, 0 }, fireFrames[ 3 ] = { 0, 1, 0 };

    initType( T_SPACE, enterSpace, NULL, 1 );
    initType( T_WALL, enterWall, NULL, 1 );
    initType( T_SCREW, enterScrew, NULL, 1 );
    initType( T_AMMO, enterAmmo, NULL, 1 );
    initType( T_KEY, enterKey, NULL, 1 );
    initType( T_DOOR, enterDoor, NULL, 1 );
    initType( T_BOX, enterBox, NULL, 1 );
    initType( T_WHEELED_BOX, enterBox, scanWheeledBox, 1 );
    initType( T_DEBRIS, enterDebris, NULL, 1 );
    initType( T_ROBBO, enterDebris, scanRobbo, 12 );
    initType( T_BULLET, enterWall, scanBullet, 4 );
    initType( T_FIRE, enterWall, scanFire, 2 );
    initType( T_EXPLOSION, enterWall, scanExplosion, 3 );
    initType( T_CANNON, enterWall, scanCannon, 1 );
    initType( T_LASER, enterWall, scanLaser, 4 );
    initType( T_BLASTER, enterWall, scanBlaster, 4 );
    initType( T_BAT, enterDebris, scanBat, 2 );
    initType( T_BEAM_SHRINK, enterWall, scanBeamShrink, 2 );
    initType( T_BEAM_EXTEND, enterWall, scanBeamExtend, 2 );
    initType( T_BEAM, enterWall, NULL, 2 );
    initType( T_STREAM, enterStream, scanStream, 3 );
    types[ T_EXPLOSION ].frames = types[ T_STREAM ].frames = streamFrames;
    types[ T_FIRE ].frames = fireFrames;
    types[ T_BULLET ].frames = types[ T_BEAM ].frames = types[ T_BEAM_EXTEND ].frames = types[ T_BEAM_SHRINK ].frames = &map.toggle;
    sumBase();
}

VOID initMap( VOID )
{
    WORD x, y;

    for( y = 0; y < HEIGHT; y++ )
    {
        for( x = 0; x < WIDTH; x++ )
        {
            Cell *cell = &map.map[ y ][ x ];

            if( x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1 )
            {
                cell->type = T_WALL;
            }
            else
            {
                cell->type = T_SPACE;
            }
        }
    }
}

VOID updateCell( Cell *cell, WORD type, WORD dir, WORD frame )
{
    cell->type = type;
    cell->dir = dir;
    cell->frame = frame;

    if( types[ type ].scan )
    {
        if( cell > map.scanned )
        {
            cell->scanned = TRUE;
        }
        cell->delay = DELAY;
    }
}

VOID scanMap( VOID )
{
    WORD x, y;
    static WORD toggleDelay = 20;

    if( toggleDelay > 0 )
    {
        toggleDelay--;
    }
    else
    {
        map.toggle ^= 1;
        toggleDelay = 20;
    }

    for( y = 0; y < HEIGHT; y++ )
    {
        for( x = 0; x < WIDTH; x++ )
        {
            Cell *cell = &map.map[ y ][ x ];

            if( cell->scanned )
            {
                cell->scanned = FALSE;
            }
            else if( cell->delay > 0 )
            {
                cell->delay--;
            }
            else if( types[ cell->type ].scan )
            {
                types[ cell->type ].scan( cell );
            }
        }
    }
}
