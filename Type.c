
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
        map.ammo++;
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
            return( FALSE );
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

STATIC BOOL enterDebris( Cell *cell, WORD as, WORD dir, WORD frame )
{
    if( as == T_BULLET )
    {
        updateCell( cell, T_EXPLOSION, 0, 0 );
    }
    return( FALSE );
}

VOID initTypes( VOID )
{
    initType( T_SPACE, enterSpace, NULL, 1 );
    initType( T_WALL, enterWall, NULL, 1 );
    initType( T_SCREW, enterScrew, NULL, 1 );
    initType( T_AMMO, enterAmmo, NULL, 1 );
    initType( T_KEY, enterKey, NULL, 1 );
    initType( T_DOOR, enterDoor, NULL, 1 );
    initType( T_BOX, enterBox, NULL, 1 );
    initType( T_WHEELED_BOX, enterBox, scanWheeledBox, 1 );
    initType( T_ROBBO, enterDebris, scanRobbo, 16 );
    initType( T_BULLET, enterWall, scanBullet, 4 );
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
