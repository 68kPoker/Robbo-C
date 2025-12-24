
#include <stdio.h>

#include <intuition/intuition.h>
#include <datatypes/pictureclass.h>
#include <libraries/iffparse.h>
#include <exec/memory.h>

#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/layers_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/iffparse_protos.h>

#include "Cell.h"
#include "Blitter.h"
#include "Window.h"

#define RGB(c) ((c)|((c)<<8)|((c)<<16)|((c)<<24))

#define VERS 39L

#define TILE_WIDTH  16
#define TILE_HEIGHT 16

#define VIEW_WIDTH  16
#define VIEW_HEIGHT 12

struct Library *IntuitionBase, *GfxBase, *LayersBase, *IFFParseBase;

WORD tiles[ VIEW_HEIGHT ][ VIEW_WIDTH ];

struct Window *openWindow( struct Screen *s, struct Region **reg )
{
    struct Window *w;

    if( w = OpenWindowTags( NULL,
        WA_CustomScreen, s,
        WA_Left, 0,
        WA_Top, 0,
        WA_Width, s->Width,
        WA_Height, s->Height,
        WA_ScreenTitle, "RobboC (c)2025 Robert Szacki",
        WA_IDCMP, IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY,
        WA_ReportMouse, TRUE,
        WA_Activate, TRUE,
        WA_SimpleRefresh, TRUE,
        WA_BackFill, LAYERS_NOBACKFILL,
        WA_Borderless, TRUE,
        WA_Backdrop, TRUE,
        WA_RMBTrap, TRUE,
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

VOID pasteTile( WORD type, WORD tile, struct Window *w, WORD x, WORD y )
{
    struct RastPort *rp = w->RPort;

    BltBitMapRastPort( map.gfx, ( tile % 20 ) * TILE_WIDTH, ( tile / 20 ) * TILE_HEIGHT, w->RPort, w->BorderLeft + x, w->BorderTop + y, TILE_WIDTH, TILE_HEIGHT, 0xc0 );
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
                pasteTile( cell->type, tile, w, x * TILE_WIDTH, y * TILE_HEIGHT );
            }
        }
    }
}

VOID testMap( VOID )
{
    WORD i, j;

    char *p[] =
    {
        "################",
        "#....#...SN....#",
        "#.R..#.B.#..$$$#",
        "#........#.#####",
        "###.##...#.#...#",
        "#>....GGGS....A#",
        "#M.......#######",
        "#.............[#",
        "##D######.######",
        "#.GGG...#.#...L#",
        "#GGG....#......#",
        "#....K..########",
        "#...K.S........#",
        "#.......#......#",
        "#.......#..$$..#",
        "#SSSGGSS#+####+#",
        "#.......###..###",
        "#..............#",
        "#.............C#",
        "################",
    };

    for( i = 0; i < HEIGHT; i++ )
    {
        for( j = 0; j < WIDTH; j++ )
        {
            WORD type = T_SPACE, dir = 0;
            switch( p[ i ][ j ] )
            {
            case '#': type = T_WALL; break;
            case '.': type = T_SPACE; break;
            case 'R': type = T_ROBBO; break;
            case 'S': type = T_BOX; break;
            case 'K': type = T_WHEELED_BOX; break;
            case 'G': type = T_DEBRIS; break;
            case '[': type = T_LASER; dir = LEFT; break;
            case '>': type = T_CANNON; dir = RIGHT; break;
            case '+': type = T_CANNON; dir = UP; break;
            case '$': type = T_SCREW; break;
            case 'D': type = T_DOOR; break;
            case 'L': type = T_KEY; break;
            case 'A': type = T_AMMO; break;
            case 'N': type = T_BAT; dir = DOWN; break;
            case 'B': type = T_BOMB; break;
            case 'M': type = T_MAGNET_RIGHT; break;
            case 'C': type = T_CAPSULE; break;
            }
            map.map[ i ][ j ].type = type;
            map.map[ i ][ j ].dir = dir;
        }
    }

    map.pos = 2 * WIDTH + 2;
#if 0
    map.map[ 2 ][ 2 ].type = T_ROBBO;
    map.pos = 2 * WIDTH + 2;
    map.map[ 2 ][ 4 ].type = T_SCREW;
    map.map[ 4 ][ 2 ].type = T_SCREW;
    for(i = 0; i <= 5; i++ )
    {
        map.map[ 6 ][ i ].type = T_WALL;
        map.map[ i ][ 5 ].type = T_WALL;
    }
    map.map[ 4 ][ 5 ].type = T_SPACE;
    map.map[ 2 ][ 7 ].type = T_AMMO;
    map.map[ 3 ][ 7 ].type = T_WALL;
    map.map[ 4 ][ 7 ].type = T_WALL;

    map.map[ 5 ][ 9 ].type = T_DEBRIS;
    map.map[ 5 ][ 10 ].type = T_DEBRIS;
    map.map[ 4 ][ 10 ].type = T_DEBRIS;

    map.map[ 2 ][ 12 ].type = T_BOX;
    map.map[ 3 ][ 13 ].type = T_BOX;

    map.map[ 2 ][ 14 ].type = T_SCREW;
    for( i = 0; i <= 6; i++ )
    {
        map.map[ 6 ][ i + 9 ].type = T_WALL;
    }
    for( i = 0; i <= 4; i++ )
    {
        map.map[ i ][ 9 ].type = T_WALL;
    }

    map.map[ 12 ][ 12 ].type = T_BAT;
    map.map[ 12 ][ 12 ].dir = UP;
    map.map[ 12 ][ 14 ].type = T_BAT;
    map.map[ 12 ][ 14 ].dir = DOWN;
        
    map.map[ 7 ][ 14 ].type = T_KEY;
#endif
}

BOOL unpackRow( BYTE **bufPtr, LONG *sizePtr, BYTE *dest, WORD bpr, UBYTE cmp )
{
    BYTE *buf = *bufPtr;
    LONG size = *sizePtr;

    if( cmp == cmpNone )
    {
        if( size < bpr )
        {
            return( FALSE );
        }
        size -= bpr;
        while( bpr-- > 0 )
        {
            *dest++ = *buf++;
        }
    }
    else if( cmp == cmpByteRun1 )
    {
        while( bpr > 0 )
        {
            BYTE con;
            if( size < 1 )
            {
                return( FALSE );
            }
            size--;
            if( ( con = *buf++ ) >= 0 )
            {
                WORD count = con + 1;
                if( size < count || bpr < count )
                {
                    return( FALSE );
                }
                size -= count;
                bpr -= count;
                while( count-- > 0 )
                {
                    *dest++ = *buf++;
                }
            }
            else if( con != -128 )
            {
                WORD count = ( -con ) + 1;
                BYTE data;
                if( size < 1 || bpr < count )
                {
                    return( FALSE );
                }
                size--;
                bpr -= count;
                data = *buf++;
                while( count-- > 0 )
                {
                    *dest++ = data;
                }
            }
        }
    }
    else
    {
        return( FALSE );
    }

    *bufPtr = buf;
    *sizePtr = size;
    return( TRUE );
}

struct BitMap *unpackPicture( struct BitMapHeader *bmhd, BYTE *buffer, LONG size )
{
    struct BitMap *bm;
    WORD rows = bmhd->bmh_Height;
    UBYTE depth = bmhd->bmh_Depth;
    UBYTE cmp = bmhd->bmh_Compression;
    WORD bpr = ( ( bmhd->bmh_Width + 15 ) >> 4 ) << 1;

    if( bmhd->bmh_Masking == mskHasMask )
    {
        depth++;
    }

    if( bm = AllocBitMap( bmhd->bmh_Width, rows, depth, BMF_INTERLEAVED, NULL ) )
    {
        PLANEPTR planes[ 8 ];
        WORD row, plane;

        for( plane = 0; plane < depth; plane++ )
        {
            planes[ plane ] = bm->Planes[ plane ];
        }

        for( row = 0; row < rows; row++ )
        {
            for( plane = 0; plane < depth; plane++ )
            {
                if( !unpackRow( &buffer, &size, planes[ plane ], bpr, cmp ) )
                {
                    FreeBitMap( bm );
                    return( NULL );
                }
                planes[ plane ] += bm->BytesPerRow;
            }
        }
        return( bm );
    }
}

struct BitMap *readPicture( STRPTR name, struct Screen **s )
{
    struct IFFHandle *iff;
    LONG props[] = { ID_ILBM, ID_BMHD, ID_ILBM, ID_CMAP, ID_ILBM, ID_CAMG };
    struct BitMap *bm = NULL;
    struct StoredProperty *sp;
    struct BitMapHeader *bmhd;
    ULONG srcID, monID, modeID;
    UBYTE *cmap;
    WORD count;
    struct Screen *pubs;
    struct ContextNode *cn;
    BYTE *buffer;
    LONG size;

    if( iff = AllocIFF() )
    {
        if( iff->iff_Stream = Open( name, MODE_OLDFILE ) )
        {
            InitIFFasDOS( iff );
            if( OpenIFF( iff, IFFF_READ ) == 0 )
            {
                if( PropChunks( iff, props, 3 ) == 0 )
                {
                    if( StopChunk( iff, ID_ILBM, ID_BODY ) == 0 )
                    {
                        if( ParseIFF( iff, IFFPARSE_SCAN ) == 0 )
                        {
                            if( sp = FindProp( iff, ID_ILBM, ID_BMHD ) )
                            {
                                bmhd = ( struct BitMapHeader * )sp->sp_Data;
                                if( sp = FindProp( iff, ID_ILBM, ID_CAMG ) )
                                {
                                    srcID = *( ULONG * )sp->sp_Data;
                                    if( sp = FindProp( iff, ID_ILBM, ID_CMAP ) )
                                    {
                                        cmap = ( UBYTE * )sp->sp_Data;
                                        count = sp->sp_Size / 3;
                                        if( pubs = LockPubScreen( NULL ) )
                                        {
                                            monID = GetVPModeID( &pubs->ViewPort ) & MONITOR_ID_MASK;

                                            UnlockPubScreen( NULL, pubs );
                                            modeID = BestModeID(
                                                BIDTAG_SourceID, srcID,
                                                BIDTAG_MonitorID, monID,
                                                BIDTAG_Depth, bmhd->bmh_Depth,
                                                TAG_DONE );
                                            if( modeID != INVALID_ID )
                                            {
                                                if( *s = OpenScreenTags( NULL,
                                                    SA_DisplayID, modeID,
                                                    SA_Depth, bmhd->bmh_Depth,
                                                    SA_Interleaved, TRUE,
                                                    SA_ColorMapEntries, count,
                                                    SA_Quiet, TRUE,
                                                    SA_BackFill, LAYERS_NOBACKFILL,
                                                    SA_Title, "RobboC (c)2025 Robert Szacki",
                                                    SA_ShowTitle, FALSE,
                                                    SA_Exclusive, TRUE,
                                                    SA_Behind, TRUE,
                                                    TAG_DONE ) )
                                                {
                                                    WORD i;
                                                    for( i = 0; i < count; i++ )
                                                    {
                                                        UBYTE red = *cmap++, green = *cmap++, blue = *cmap++;

                                                        SetRGB32CM( ( *s )->ViewPort.ColorMap, i, RGB( red ), RGB( green ), RGB( blue ) );
                                                    }
                                                    MakeScreen( *s );
                                                    RethinkDisplay();
                                                    if( cn = CurrentChunk( iff ) )
                                                    {
                                                        size = cn->cn_Size;
                                                        if( buffer = AllocMem( size, MEMF_PUBLIC ) )
                                                        {
                                                            if( ReadChunkBytes( iff, buffer, size ) == size )
                                                            {
                                                                bm = unpackPicture( bmhd, buffer, size );
                                                            }
                                                            FreeMem( buffer, size );
                                                        }
                                                    }
                                                    if( !bm )
                                                    {
                                                        CloseScreen( *s );
                                                    }
                                                }
                                            }
                                        }

                                    }
                                }
                            }
                        }
                    }
                }
                CloseIFF( iff );
            }
            Close( iff->iff_Stream );
        }
        FreeIFF( iff );
    }
    return( bm );
}

VOID freePicture( struct BitMap *bm, struct Screen *s )
{
    CloseScreen( s );
    FreeBitMap( bm );
}

VOID drawPanel( struct Window *w )
{
    WORD i, j;

    for( i = 0; i < 3; i++ )
    {
        pasteTile( 0, 3 * 20 + i, w, VIEW_WIDTH * TILE_WIDTH, ( i + 1 ) * ( TILE_HEIGHT + 2 ) );
        for( j = 0; j < 3; j++ )
        {
            pasteTile( 0, 4 * 20 + j, w, VIEW_WIDTH * TILE_WIDTH + TILE_WIDTH * ( j + 1 ), ( i + 1 ) * ( TILE_HEIGHT + 2 ) );
        }
    }
}

VOID updatePanel( struct Window *w )
{
    struct RastPort *rp = w->RPort;
    UBYTE text[ 4 ];
    static WORD collected = -1, keys = -1, ammo = -1;

    SetAPen( rp, 1 );

    if( collected != map.collected )
    {
        Move( rp, VIEW_WIDTH * ( TILE_WIDTH + 1 ) + 2, ( TILE_HEIGHT + 2 ) + 4 + rp->Font->tf_Baseline );
        sprintf( text, "%3d", map.collected );
        Text( rp, text, 3 );
        collected = map.collected;
    }

    if( keys != map.keys )
    {
        Move( rp, VIEW_WIDTH * ( TILE_WIDTH + 1 ) + 2, 2 * ( TILE_HEIGHT + 2 ) + 4 + rp->Font->tf_Baseline );
        sprintf( text, "%3d", map.keys );
        Text( rp, text, 3 );
        keys = map.keys;
    }

    if( ammo != map.ammo )
    {
        Move( rp, VIEW_WIDTH * ( TILE_WIDTH + 1 ) + 2, 3 * ( TILE_HEIGHT + 2 ) + 4 + rp->Font->tf_Baseline );
        sprintf( text, "%3d", map.ammo );
        Text( rp, text, 3 );
        ammo = map.ammo;
    }
}

int main( void )
{
    struct Screen *s;
    struct Window *w;
    struct Region *reg;

    if( IntuitionBase = OpenLibrary( "intuition.library", VERS ) )
    {
        if( GfxBase = OpenLibrary( "graphics.library", VERS ) )
        {
            if( LayersBase = OpenLibrary( "layers.library", VERS ) )
            {
                if( IFFParseBase = OpenLibrary( "iffparse.library", VERS ) )
                {
                    if( map.gfx = readPicture( "Robbo.iff", &s ) )
                    {
                        if( w = openWindow( s, &reg ) )
                        {
                            BOOL done = FALSE;
                            WORD dx = 0, dy = 0, tx = 0, ty = 0;
                            WORD i;
                            BOOL view = FALSE;
                            BYTE dir = 0;

                            initTypes();
                            initMap();
                            testMap();

                            drawMap( w, dx, dy, TRUE );
                            drawPanel( w );
                            WaitBlit();
                            ScreenToFront( s );
                            while( !done && !map.done )
                            {
                                ULONG mask = 1L << w->UserPort->mp_SigBit;
                                WaitTOF();
                                scanMap();

                                if( view )
                                {
                                    ty += dir / WIDTH;
                                }
                                else if( ( ( map.pos / WIDTH ) - dy ) < 1 || ( ( map.pos / WIDTH ) - dy ) > VIEW_HEIGHT - 2 )
                                {
                                    ty = ( map.pos / WIDTH ) - ( VIEW_HEIGHT / 2 );
                                }

                                if( ty < 0 )
                                {
                                    ty = 0;
                                }
                                else if( ty > HEIGHT - VIEW_HEIGHT )
                                {
                                    ty = HEIGHT - VIEW_HEIGHT;
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
                                updatePanel( w );
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

                                        if( cls == IDCMP_MOUSEBUTTONS )
                                        {
                                            done = TRUE;
                                        }
                                        else if( cls == IDCMP_RAWKEY )
                                        {
                                            if( code == LEFT_KEY )
                                            {
                                               dir = LEFT;
                                            }
                                            else if( code == ( LEFT_KEY | IECODE_UP_PREFIX ) )
                                            {
                                                if( dir == LEFT )
                                                {
                                                    dir = 0;
                                                }
                                            }
                                            if( code == RIGHT_KEY )
                                            {
                                                dir = RIGHT;
                                            }
                                            else if( code == ( RIGHT_KEY | IECODE_UP_PREFIX ) )
                                            {
                                                if( dir == RIGHT )
                                                {
                                                    dir = 0;
                                                }
                                            }
                                            if( code == UP_KEY )
                                            {
                                                dir = UP;
                                            }
                                            else if( code == ( UP_KEY | IECODE_UP_PREFIX ) )
                                            {
                                                if( dir == UP )
                                                {
                                                    dir = 0;
                                                }
                                            }
                                            if( code == DOWN_KEY )
                                            {
                                                dir = DOWN;
                                            }
                                            else if( code == ( DOWN_KEY | IECODE_UP_PREFIX ) )
                                            {
                                                if( dir == DOWN )
                                                {
                                                    dir = 0;
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
                                            if( qual & ( IEQUALIFIER_LALT | IEQUALIFIER_RALT ) )
                                            {
                                                view = TRUE;
                                            }
                                            else
                                            {
                                                view = FALSE;
                                                map.dir = dir;
                                            }
                                        }
                                    }
                                }
                            }
                            closeWindow( w, reg );
                        }
                        freePicture( map.gfx, s );
                    }
                    CloseLibrary( IFFParseBase );
                }
                CloseLibrary( LayersBase );
            }
            CloseLibrary( GfxBase );
        }
        CloseLibrary( IntuitionBase );
    }
    return( 0 );
}
