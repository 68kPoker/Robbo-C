
#include <stdio.h>

#include <intuition/intuition.h>
#include <datatypes/pictureclass.h>
#include <libraries/iffparse.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <graphics/scale.h>
#include <graphics/videocontrol.h>
#include <hardware/intbits.h>

#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/layers_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/iffparse_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/asl_protos.h>

#include "Cell.h"
#include "Blitter.h"
#include "Window.h"
#include "Editor.h"
#include "debug.h"

#define RGB(c) ((c)|((c)<<8)|((c)<<16)|((c)<<24))

#define VERS 39L

struct Library *IntuitionBase, *GfxBase, *GadToolsBase, *LayersBase, *IFFParseBase, *AslBase;

WORD tiles[ VIEW_HEIGHT ][ VIEW_WIDTH ];

UWORD myPlanePick[ 60 ];

struct Interrupt is;

struct Task *sigTask;
WORD sigBit;

struct ViewPort *viewPort;

enum
{
    IMG_AMIGA,
    IMG_CHECK,
    IMG_CLEAR,
    IMG_LOAD,
    IMG_SAVE,
    IMAGES
};

enum
{
    MENU_GAME,
    MENU_SETTINGS,
    MENU_CONSTRUCT
};

enum
{
    GAME_RESTART,
    GAME_QUIT = GAME_RESTART + 2
};

enum
{
    SETTINGS_TITLE,
    SETTINGS_CONSTRUCT = SETTINGS_TITLE + 2,
};

enum
{
    CONSTRUCT_CLEAR,
    CONSTRUCT_LOAD,
    CONSTRUCT_SAVE,
    CONSTRUCT_TOOLS = CONSTRUCT_SAVE + 2,
    CONSTRUCT_CURSOR,
    CONSTRUCT_ROBBO = CONSTRUCT_CURSOR + 2,
    CONSTRUCT_DIR
};

enum
{
    LEFT_DIR,
    RIGHT_DIR,
    UP_DIR,
    DOWN_DIR
};

struct Image images[ IMAGES ];

struct NewMenu nm[] =
{
    { NM_TITLE, "Game", 0, 0, 0, 0 },
    { NM_ITEM, "Restart...", "A", 0, 0, 0 },
    { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
    { NM_ITEM, "Quit...   ", "Q", 0, 0, 0 },
    { NM_TITLE, "Settings", 0, 0, 0, 0 },
    { NM_ITEM, "Show Title    ", "T", MENUTOGGLE | CHECKIT, 0, 0 },
    { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
    { IM_ITEM, ( STRPTR )(images + IMG_CLEAR), "M", MENUTOGGLE | CHECKIT, 0, 0},
    { NM_TITLE, "Construct", 0, NM_MENUDISABLED, 0, 0 },
    { NM_ITEM, "Clear", 0, 0, 0, 0 },
    { IM_ITEM, ( STRPTR )(images + IMG_LOAD), "O", 0, 0, 0 },
    { IM_ITEM, ( STRPTR )(images + IMG_SAVE), "S", 0, 0, 0 },
    { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
    { NM_ITEM, "Show Tools ", "B", MENUTOGGLE | CHECKIT | NM_ITEMDISABLED, 0, 0 },
    { NM_ITEM, "Show Cursor", "C", MENUTOGGLE | CHECKIT, 0, 0 },
    { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
    { NM_ITEM, "Robbo      ", "H", 0, 0, 0 },
    { NM_ITEM, "Direction  ", 0, 0, 0, 0 },
    { NM_SUB, "Left ", "L", CHECKIT | CHECKED, ~01, 0 },
    { NM_SUB, "Right", "R", CHECKIT, ~02, 0 },
    { NM_SUB, "Up   ", "U", CHECKIT, ~04, 0 },
    { NM_SUB, "Down ", "D", CHECKIT, ~010, 0 },
#if 0
    { NM_TITLE, "Select", 0, NM_MENUDISABLED, 0, 0 },
    { NM_ITEM, "Space    ", "0", CHECKIT, ~01, 0 },
    { NM_ITEM, "Wall     ", "1", CHECKIT, ~02, 0 },
    { NM_ITEM, "Capsule  ", "2", CHECKIT, ~04, 0 },
    { NM_ITEM, "Screw    ", "3", CHECKIT | CHECKED, ~010, 0 },
    { NM_ITEM, "Key      ", "4", CHECKIT, ~020, 0 },
    { NM_ITEM, "Ammo     ", "5", CHECKIT, ~040, 0 },
    { NM_ITEM, "Door     ", "6", CHECKIT, ~0100, 0},
    { NM_ITEM, "Box      ", "7", CHECKIT, ~0200, 0 },
    { NM_ITEM, "Wheel Box", "8", CHECKIT, ~0400, 0 },
    { NM_ITEM, "Bomb     ", "9", CHECKIT, ~01000, 0 },
    { NM_ITEM, "Bat      ", "!", CHECKIT, ~02000, 0 },
    { NM_ITEM, "Cannon   ", "@", CHECKIT, ~04000, 0 },
    { NM_ITEM, "Laser    ", "#", CHECKIT, ~010000, 0 },
    { NM_ITEM, "Blaster  ", "$", CHECKIT, ~020000, 0 },
    { NM_ITEM, "Surprise ", "%", CHECKIT, ~040000, 0 },
    { NM_ITEM, "Debris   ", "^", CHECKIT, ~0100000, 0 },
    { NM_ITEM, "Magnet ] ", "&", CHECKIT, ~0200000, 0 },
    { NM_ITEM, "Magnet [ ", "*", CHECKIT, ~0400000, 0 },
#endif
    { NM_END }
};

WORD pens[ NUMDRIPENS + 1 ] = { 0, 12, 0, 2, 0, 11, 2, 12, 2, 0, 12, 0, ~0 };

struct Gadget dragBar;

struct Window *openToolbox( struct Window *w, WORD mx, WORD my, ULONG *mask );

BOOL createImage( struct Image *img, struct BitMap *bm, WORD x, WORD y, WORD width, WORD height )
{
    UWORD pick;
    UBYTE planes, p;
    UWORD wordWidth;
    UWORD planeSize, *ptr;
    struct BitMap dest;

    pick = planePick( bm, x, y, width, height, FALSE );

    img->Width = width;
    img->Height = height;
    img->NextImage = NULL;
    img->PlanePick = pick & 0xff;
    img->PlaneOnOff = pick >> 8;
    img->Depth = bm->Depth;

    pick &= 0xff;

    for( planes = 0; pick; pick >>= 1 )
    {
        if( pick & 1 )
        {
            planes++;
        }
    }

    pick = img->PlanePick;

    wordWidth = ( width + 15 ) >> 4;
    planeSize = wordWidth * height;

    if( img->ImageData = ptr = AllocVec( planeSize * sizeof( WORD ) * planes, MEMF_CHIP ) )
    {
        InitBitMap( &dest, bm->Depth, width, height );

        for( p = 0; pick; p++, pick >>= 1 )
        {
            if( pick & 1 )
            {
                dest.Planes[ p ] = ( PLANEPTR )ptr;
                ptr += planeSize;
            }
        }
        drawTileStd( bm, x, y, &dest, 0, 0, width, height, 0xc0, img->PlanePick );
        return( TRUE );
    }
    return( FALSE );
}

void deleteImage( struct Image *img )
{
    FreeVec( img->ImageData );
}

struct Window *openWindow( struct Screen *s, struct Region **reg )
{
    struct Window *w;

    if( w = OpenWindowTags( NULL,
        WA_CustomScreen, s,
        WA_Left, 0,
        WA_Top, 0,
        WA_Width, s->Width,
        WA_Height, s->Height,
        WA_ScreenTitle, "RobboC: Game mode",
        WA_IDCMP, IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK,
        WA_ReportMouse, TRUE,
        WA_Activate, TRUE,
        WA_SimpleRefresh, TRUE,
        WA_BackFill, LAYERS_NOBACKFILL,
        WA_Borderless, TRUE,
        WA_Backdrop, TRUE,
        WA_RMBTrap, FALSE,
        WA_NewLookMenus, TRUE,
        WA_Checkmark, images + IMG_CHECK,
        WA_AmigaKey, images + IMG_AMIGA,
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
#if 0
    BltBitMap( map.back, ( tile % 20 ) * TILE_WIDTH, ( tile / 20 ) * TILE_HEIGHT, w->RPort->BitMap, w->BorderLeft + x, w->BorderTop + y, TILE_WIDTH, TILE_HEIGHT, 0xc0, 0xff, NULL );
#endif
    drawTileRastPort( map.gfxBlit, ( tile % 20 ) * TILE_WIDTH, ( tile / 20 ) * TILE_HEIGHT, w->RPort, w->BorderLeft + x, w->BorderTop + y, TILE_WIDTH, TILE_HEIGHT, 0xc0 );

}

VOID drawEdMap( struct Window *w, WORD dx, WORD dy, BOOL force )
{
    WORD x, y;

    for( y = 0; y < VIEW_HEIGHT; y++ )
    {
        for( x = 0; x < VIEW_WIDTH; x++ )
        {
            EdCell *cell = &map.ed.map[ y + dy ][ x + dx ];
            Type *ptr = types + ( map.ed.head.x == x + dx && map.ed.head.y == y + dy ? T_ROBBO : ed[ cell->type ] );
            WORD tile = ptr->base;

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
                SetWriteMask( w->RPort, 0xff );
                pasteTile( cell->type, tile, w, x * TILE_WIDTH, y * TILE_HEIGHT );
            }
        }
    }

    SetWriteMask( w->RPort, 0xff );
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
#if 0
                UWORD prev = myPlanePick[ tiles[ y ][ x ] ], cur = myPlanePick[ tile ];

                SetWriteMask( w->RPort, ( prev & 0xff ) | ( cur & 0xff ) | ( ( prev >> 8 ) ^ ( cur >> 8 ) ) );
#endif

                tiles[ y ][ x ] = tile;
                pasteTile( cell->type, tile, w, x * TILE_WIDTH, y * TILE_HEIGHT );
            }
        }
    }

    SetWriteMask( w->RPort, 0xff );
}

VOID testMap( VOID )
{
    WORD i, j;

    char *p[] =
    {
        "################",
        "#....#...SN....#",
        "#.R..#...#..$$$#",
        "#........#.#####",
        "###.##...#.#...#",
        "#........S....A#",
        "#>..GGGGG#######",
        "#.............[#",
        "##D######.######",
        "#.GGG...#.#...L#",
        "#GGG....#......#",
        "#....K..########",
        "#.B.K.S........#",
        "#..............#",
        "#.......#.#$$#.#",
        "#SSSGGSS#+####+#",
        "#.......###..###",
        "#..............#",
        "#M............C#",
        "################",
    };

    for( i = 0; i < HEIGHT; i++ )
    {
        for( j = 0; j < WIDTH; j++ )
        {
            WORD type = E_SPACE, dir = 0;
            switch( p[ i ][ j ] )
            {
            case '#': type = E_WALL; break;
            case '.': type = E_SPACE; break;
            case 'R': map.ed.head.x = j; map.ed.head.y = i; break;
            case 'S': type = E_BOX; break;
            case 'K': type = E_WHEELED_BOX; break;
            case 'G': type = E_DEBRIS; break;
            case '[': type = E_LASER; dir = LEFT; break;
            case '>': type = E_CANNON; dir = RIGHT; break;
            case '+': type = E_CANNON; dir = UP; break;
            case '$': type = E_SCREW; break;
            case 'D': type = E_DOOR; break;
            case 'L': type = E_KEY; break;
            case 'A': type = E_AMMO; break;
            case 'N': type = E_BAT; dir = DOWN; break;
            case 'B': type = E_BOMB; break;
            case 'M': type = E_MAGNET_RIGHT; break;
            case 'C': type = E_CAPSULE; break;
            }
            map.ed.map[ i ][ j ].type = type;
            map.ed.map[ i ][ j ].dir = dir;
        }
    }
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

struct Screen *openScreen( struct IFFHandle *iff, struct BitMapHeader *bmhd )
{
    struct StoredProperty *sp;
    ULONG srcID, monID, modeID;
    UBYTE *cmap;
    WORD count;
    struct Screen *pubs, *s = NULL;

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
                    if( s = OpenScreenTags( NULL,
                        SA_DisplayID, modeID,
                        SA_Width, 320,
                        SA_Height, 256,
                        SA_Depth, bmhd->bmh_Depth,
                        SA_Interleaved, TRUE,
                        SA_ColorMapEntries, count,
                        SA_Quiet, FALSE,
                        SA_BackFill, LAYERS_NOBACKFILL,
                        SA_Title, "RobboC (c)2025 Robert Szacki",
                        SA_ShowTitle, FALSE,
                        SA_Exclusive, TRUE,
                        SA_Behind, TRUE,
                        SA_BlockPen, 0,
                        SA_DetailPen, 12,
                        SA_Pens, pens,
                        SA_SysFont, 1,
                        TAG_DONE ) )
                    {
                        WORD i;
                        for( i = 0; i < count; i++ )
                        {
                            UBYTE red = *cmap++, green = *cmap++, blue = *cmap++;

                            SetRGB32CM( s->ViewPort.ColorMap, i, RGB( red ), RGB( green ), RGB( blue ) );
                        }
                        MakeScreen( s );
                        RethinkDisplay();
                    }
                }
            }
        }
    }
    return( s );
}

struct BitMap *readPicture( STRPTR name, struct Screen **s )
{
    struct IFFHandle *iff;
    LONG props[] = { ID_ILBM, ID_BMHD, ID_ILBM, ID_CMAP, ID_ILBM, ID_CAMG };
    struct BitMap *bm = NULL;
    struct StoredProperty *sp;
    struct BitMapHeader *bmhd;
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
                        if( StopOnExit( iff, ID_ILBM, ID_FORM ) == 0 )
                        {
                            if( ParseIFF( iff, IFFPARSE_SCAN ) == 0 )
                            {
                                if( sp = FindProp( iff, ID_ILBM, ID_BMHD ) )
                                {
                                    bmhd = ( struct BitMapHeader * )sp->sp_Data;

                                    if( cn = CurrentChunk( iff ) )
                                    {
                                        size = cn->cn_Size;
                                        if( buffer = AllocMem( size, MEMF_PUBLIC ) )
                                        {
                                            if( ReadChunkBytes( iff, buffer, size ) == size )
                                            {
                                                if( bm = unpackPicture( bmhd, buffer, size ) )
                                                {
                                                    if( s )
                                                    {
                                                        if( !( *s = openScreen( iff, bmhd ) ) )
                                                        {
                                                            FreeBitMap( bm );
                                                            bm = NULL;
                                                        }
                                                    }
                                                }
                                            }
                                            FreeMem( buffer, size );
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
    drawTileRastPort( map.back, 256, 0, w->RPort, 256, 0, 64, 256, 0xc0 );
    drawTileRastPort( map.back, 0, 192, w->RPort, 0, 192, 256, 64, 0xc0 );
}

VOID updatePanel( struct Window *w )
{
    struct RastPort *rp = w->RPort;
    UBYTE text[ 4 ];
    static WORD collected = -1, keys = -1, ammo = -1;

    SetABPenDrMd( rp, 0, 26, JAM2 );

    if( collected != map.collected )
    {
        Move( rp, VIEW_WIDTH * ( TILE_WIDTH + 1 ) + 2, ( TILE_HEIGHT )+4 + rp->Font->tf_Baseline );
        sprintf( text, "%3d", map.collected );
        Text( rp, text, 3 );
        collected = map.collected;
    }

    if( keys != map.keys )
    {
        Move( rp, VIEW_WIDTH * ( TILE_WIDTH + 1 ) + 2, 2 * ( TILE_HEIGHT )+4 + rp->Font->tf_Baseline );
        sprintf( text, "%3d", map.keys );
        Text( rp, text, 3 );
        keys = map.keys;
    }

    if( ammo != map.ammo )
    {
        Move( rp, VIEW_WIDTH * ( TILE_WIDTH + 1 ) + 2, 3 * ( TILE_HEIGHT )+4 + rp->Font->tf_Baseline );
        sprintf( text, "%3d", map.ammo );
        Text( rp, text, 3 );
        ammo = map.ammo;
    }
}

VOID prepBG( VOID )
{
    WORD i;

    for( i = 1; i < 60; i++ )
    {
        setBG( map.gfx, 0, 0, map.gfx, ( i % 20 ) * TILE_WIDTH, ( i / 20 ) * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT, ABC | ANBC | NABC | NABNC, 0xff );
    }

    planePickArray( map.gfx, TILE_WIDTH, TILE_HEIGHT, 20, 3, myPlanePick, FALSE );
}

VOID menuPick( struct Window *w, struct Menu *menu, UWORD code, BOOL *done, WORD *type, WORD *dir, BOOL *construct, struct Window **toolbox, ULONG *mask, WORD *toolx, WORD *tooly, VOID( **draw )( struct Window *w, WORD dx, WORD dy, BOOL force ), ULONG *total )
{
    struct MenuItem *item;
    UWORD menuNum, itemNum, subNum;
    WORD dirs[] = { LEFT, RIGHT, UP, DOWN };

    while( code != MENUNULL )
    {
        item = ItemAddress( menu, code );

        menuNum = MENUNUM( code );
        itemNum = ITEMNUM( code );
        subNum = SUBNUM( code );

        if( menuNum == MENU_GAME )
        {
            if( itemNum == GAME_QUIT )
            {
                *done = TRUE;
                return;
            }
        }
        else if( menuNum == MENU_CONSTRUCT )
        {
            if( itemNum == CONSTRUCT_CLEAR )
            {
                initMap();
            }
            else if( itemNum == CONSTRUCT_DIR )
            {
                *dir = dirs[ subNum ];
            }
        }
        else if( menuNum == MENU_SETTINGS )
        {
            if( itemNum == SETTINGS_TITLE )
            {
                ShowTitle( w->WScreen, item->Flags & CHECKED );
            }
            else if( itemNum == SETTINGS_CONSTRUCT )
            {
                if( *construct = item->Flags & CHECKED )
                {
                    if( !*toolbox )
                    {
                        *toolbox = openToolbox( w, *toolx, *tooly, mask );
                        *total |= mask[ 1 ];
                        SetWindowTitles( *toolbox, ( UBYTE * )~0, "RobboC: Constructor" );
                        SetWindowTitles( w, ( UBYTE * )~0, "RobboC: Constructor" );
                    }

                    OnMenu( w, FULLMENUNUM( MENU_CONSTRUCT, NOITEM, NOITEM ) );
                    *draw = drawEdMap;
                }
                else
                {
                    if( *toolbox )
                    {
                        *toolx = ( *toolbox )->LeftEdge;
                        *tooly = ( *toolbox )->TopEdge;
                        CloseWindow( *toolbox );
                        *total &= ~mask[ 1 ];
                        mask[ 1 ] = 0L;
                        *toolbox = NULL;
                    }
                    SetWindowTitles( w, ( UBYTE * )~0, "RobboC: Game mode" );
                    OffMenu( w, FULLMENUNUM( MENU_CONSTRUCT, NOITEM, NOITEM ) );
                    convertMap();
                    *draw = drawMap;
                }
            }
        }

        code = item->NextSelect;
    }
}

CPTR hookFunc( LONG type, CPTR obj, struct FileRequester *fr )
{
    struct IntuiMessage *msg;
    struct Window *w;
    switch( type )
    {
    case FILF_DOMSGFUNC:
        /* We got a message meant for the window */
        msg = ( struct IntuiMessage * )obj;
        w = msg->IDCMPWindow;
        if( msg->Class == IDCMP_REFRESHWINDOW )
        {
            WORD i, j;
            BeginRefresh( w );
#if 0
            drawPanel( w );
            drawMap( w, map.dx, map.dy, TRUE );
            updatePanel( w );
#endif
            drawTileRastPort( map.rp->BitMap, 0, 0, w->RPort, 0, 0, 320, 256, 0xc0 );

            EndRefresh( w, TRUE );
        }

        return( obj );
        break;
    }
    return( 0 );
}

struct Window *openToolbox( struct Window *w, WORD mx, WORD my, ULONG *mask )
{
    struct Window *toolbox;

    if( toolbox = OpenWindowTags( NULL,
        WA_CustomScreen, w->WScreen,
        WA_Left, mx,
        WA_Top, my,
        WA_Width, 1 + 5 * ( TILE_WIDTH + 1 ),
        WA_Height, 1 + 5 * ( TILE_HEIGHT + 1 ) + 8,
        WA_Borderless, TRUE,
        WA_SimpleRefresh, TRUE,
        WA_BackFill, LAYERS_NOBACKFILL,
        WA_IDCMP, IDCMP_MOUSEBUTTONS | IDCMP_REFRESHWINDOW,
        WA_Activate, TRUE,
        WA_RMBTrap, FALSE,
        WA_ScreenTitle, "RobboC: Toolbox active",
        WA_Gadgets, &dragBar,
        TAG_DONE ) )
    {
        mask[ 1 ] = 1L << toolbox->UserPort->mp_SigBit;

        drawTileRastPort( map.back, 0, 102 - 8, toolbox->RPort, 0, 0, toolbox->Width, toolbox->Height, 0xc0 );
        LendMenus( toolbox, w );
        return( toolbox );
    }
    return( NULL );
}

void closeToolbox( struct Window *toolbox, ULONG *mask )
{
    CloseWindow( toolbox );
    mask[ 1 ] = 0L;

}

#ifndef WIN32
__saveds
#endif

LONG myVBlank(void )
{
    if( !( viewPort->Modes & VP_HIDE ) )
    {
        Signal( sigTask, 1L << sigBit );
    }
    return( 0 );
}

int main( void )
{
    struct Screen *s;
    struct Window *w, *toolbox = NULL;
    struct Region *reg;
    struct VisualInfo *vi;
    struct Menu *menu;
    void( *draw )( struct Window *w, WORD dx, WORD dy, BOOL force ) = drawMap;
    UBYTE set = 0, clear = 0; /* Directions set and clared */
    UBYTE scrollDelay = 0;

    if( ( sigBit = AllocSignal( -1 ) ) == -1 )
    {
        return( RETURN_FAIL );
    }

    sigTask = FindTask( NULL );

    is.is_Code = ( void( * )( ) )myVBlank;
    is.is_Data = NULL;
    is.is_Node.ln_Pri = 0;

    dragBar.Activation = GACT_IMMEDIATE;
    dragBar.Flags = GFLG_GADGHNONE;
    dragBar.Width = 5 * TILE_WIDTH;
    dragBar.Height = 8;
    dragBar.GadgetType = GTYP_WDRAGGING;

    if( IntuitionBase = OpenLibrary( "intuition.library", VERS ) )
    {
        if( GfxBase = OpenLibrary( "graphics.library", VERS ) )
        {
            if( GadToolsBase = OpenLibrary( "gadtools.library", VERS ) )
            {
                if( LayersBase = OpenLibrary( "layers.library", VERS ) )
                {
                    if( IFFParseBase = OpenLibrary( "iffparse.library", VERS ) )
                    {
                        if( AslBase = OpenLibrary( "asl.library", VERS ) )
                        {
                            if( map.back = readPicture( "Back.iff", &s ) )
                            {
                                D( bug( "Back.iff Picture read.\n" ) );
                                map.rp = &s->RastPort;
                                viewPort = &s->ViewPort;

                                AddIntServer( INTB_VERTB, &is );

                                if( vi = GetVisualInfo( s, TAG_DONE ) )
                                {
                                    if( map.gfx = readPicture( "Gfx.iff", NULL ) )
                                    {
                                        D( bug( "Gfx.iff picture read.\n" ) );
                                        prepBG();
                                        D( bug( "Background prepared.\n" ) );;
                                        if( map.gfxBlit = AllocBitMap( 320, 128, map.back->Depth, BMF_INTERLEAVED, NULL ) )
                                        {
                                            BltBitMap( map.gfx, 0, 0, map.gfxBlit, 0, 0, 320, 128, 0xc0, 0xff, NULL );
                                            createImage( images + IMG_CLEAR, map.gfxBlit, 0, 48, TILE_WIDTH, TILE_HEIGHT );
                                            createImage( images + IMG_LOAD, map.gfxBlit, 16, 48, TILE_WIDTH, TILE_HEIGHT );
                                            createImage( images + IMG_SAVE, map.gfxBlit, 32, 48, TILE_WIDTH, TILE_HEIGHT );
                                            createImage( images + IMG_AMIGA, map.gfxBlit, 48, 48, TILE_WIDTH, 10 );
                                            createImage( images + IMG_CHECK, map.gfxBlit, 64, 48, TILE_WIDTH, 10 );
                                            D( bug( "Images created.\n" ) );
                                            if( w = openWindow( s, &reg ) )
                                            {
                                                D( bug( "Window opened.\n" ) );
                                                if( menu = CreateMenus( nm, TAG_DONE ) )
                                                {
                                                    if( LayoutMenus( menu, vi, GTMN_NewLookMenus, TRUE, TAG_DONE ) )
                                                    {
                                                        BOOL done = FALSE, construct = FALSE;
                                                        WORD dx = 0, dy = 0, tx = 0, ty = 0;
                                                        WORD i;
                                                        BOOL view = FALSE, paint = FALSE;
                                                        WORD px, py;
                                                        BYTE dir = 0;
                                                        WORD curType = E_SCREW, curDir = LEFT;
                                                        ULONG mask[ 3 ] = { 0 }, total = 0L;
                                                        WORD toolx = 0, tooly = s->Height;

                                                        total = mask[ 0 ] = 1L << w->UserPort->mp_SigBit;
                                                        total |= mask[ 2 ] = 1L << sigBit;

                                                        SetMenuStrip( w, menu );

                                                        initTypes();
                                                        initMap();
                                                        D( bug( "Edit map created.\n" ) );
                                                        testMap();
                                                        D( bug( "Created test map.\n" ) );
                                                        convertMap();

                                                        drawPanel( w );

                                                        draw( w, dx, dy, TRUE );
                                                        D( bug( "Map drawn.\n" ) );
                                                        drawSelection( w );

                                                        WaitBlit();
                                                        ScreenToFront( s );
                                                        while( !done && !map.done )
                                                        {
                                                            struct IntuiMessage *msg;

                                                            ULONG result = Wait( total );

                                                            if( result & mask[ 2 ] )
                                                            {
                                                                if( clear )
                                                                {
                                                                    if( clear & ( 1 << LEFT_DIR ) )
                                                                    {
                                                                        set &= ~( 1 << LEFT_DIR );
                                                                    }
                                                                    if( clear & ( 1 << RIGHT_DIR ) )
                                                                    {
                                                                        set &= ~( 1 << RIGHT_DIR );
                                                                    }
                                                                    if( clear & ( 1 << UP_DIR ) )
                                                                    {
                                                                        set &= ~( 1 << UP_DIR );
                                                                    }
                                                                    if( clear & ( 1 << DOWN_DIR ) )
                                                                    {
                                                                        set &= ~( 1 << DOWN_DIR );
                                                                    }
                                                                    clear = 0;
                                                                }

                                                                if( set )
                                                                {
                                                                    if( set & ( 1 << LEFT_DIR ) )
                                                                    {
                                                                        dir = LEFT;
                                                                    }
                                                                    else if( set & ( 1 << RIGHT_DIR ) )
                                                                    {
                                                                        dir = RIGHT;
                                                                    }
                                                                    else if( set & ( 1 << UP_DIR ) )
                                                                    {
                                                                        dir = UP;
                                                                    }
                                                                    else if( set & ( 1 << DOWN_DIR ) )
                                                                    {
                                                                        dir = DOWN;
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    dir = 0;
                                                                }

                                                                if( !view && !construct )
                                                                {
                                                                    map.dir = dir;
                                                                }

                                                                if( scrollDelay > 0 )
                                                                {
                                                                    scrollDelay--;
                                                                }
                                                                else if( scrollDelay == 0 )
                                                                {
                                                                    if( dir )
                                                                    {
                                                                        scrollDelay = DELAY;
                                                                    }
                                                                    if( view || construct )
                                                                    {
                                                                        ty += dir / WIDTH;
                                                                    }
                                                                    else if( ( ( map.pos / WIDTH ) - dy ) < 1 || ( ( map.pos / WIDTH ) - dy ) > VIEW_HEIGHT - 2 )
                                                                    {
                                                                        ty = ( map.pos / WIDTH ) - ( VIEW_HEIGHT / 2 );
                                                                    }
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

                                                                if( !construct )
                                                                {
                                                                    scanMap();
                                                                }

                                                                draw( w, dx, dy, FALSE );
                                                                updatePanel( w );
                                                            }

                                                            if( result & mask[ 1 ] )
                                                            {
                                                                while( ( msg = ( struct IntuiMessage * )GetMsg( toolbox->UserPort ) ) )
                                                                {
                                                                    ULONG cls = msg->Class;
                                                                    UWORD code = msg->Code;
                                                                    WORD mx = msg->MouseX;
                                                                    WORD my = msg->MouseY;
                                                                    ReplyMsg( ( struct Message * )msg );

                                                                    if( cls == IDCMP_REFRESHWINDOW )
                                                                    {
                                                                        drawTileRastPort( map.back, 0, 102 - 8, toolbox->RPort, 0, 0, toolbox->Width, toolbox->Height, 0xc0 );
                                                                    }
                                                                    else if( cls == IDCMP_MOUSEBUTTONS )
                                                                    {
                                                                        if( code == ( IECODE_LBUTTON | IECODE_UP_PREFIX ) )
                                                                        {
                                                                            WORD type = ( ( my - 1 - 8 ) / ( TILE_HEIGHT + 1 ) ) * 5 + ( ( mx - 1 ) / ( TILE_WIDTH + 1 ) );
                                                                            if( type >= 0 && type < E_COUNT )
                                                                            {
                                                                                curType = type;
                                                                            }
                                                                        }
                                                                        else if( code == IECODE_RBUTTON )
                                                                        {
                                                                            CloseWindow( toolbox );
                                                                            total &= ~mask[ 1 ];
                                                                            mask[ 1 ] = 0L;
                                                                            toolbox = NULL;
                                                                            break;
                                                                        }
                                                                    }
                                                                }
                                                            }

                                                            if( result & mask[ 0 ] )
                                                            {
                                                                while( msg = ( struct IntuiMessage * )GetMsg( w->UserPort ) )
                                                                {
                                                                    ULONG cls = msg->Class;
                                                                    UWORD code = msg->Code;
                                                                    WORD mx = msg->MouseX;
                                                                    WORD my = msg->MouseY;
                                                                    UWORD qual = msg->Qualifier;

                                                                    if( cls == IDCMP_REFRESHWINDOW )
                                                                    {
                                                                        BeginRefresh( w );
                                                                        drawPanel( w );
                                                                        draw( w, dx, dy, TRUE );
                                                                        updatePanel( w );
                                                                        EndRefresh( w, TRUE );
                                                                    }
                                                                    else if( cls == IDCMP_MENUPICK )
                                                                    {
                                                                        menuPick( w, menu, code, &done, &curType, &curDir, &construct, &toolbox, mask, &toolx, &tooly, &draw, &total );
                                                                    }
                                                                    else if( cls == IDCMP_MOUSEBUTTONS )
                                                                    {
                                                                        WORD x = mx / TILE_WIDTH, y = my / TILE_HEIGHT;

                                                                        if( code == IECODE_LBUTTON )
                                                                        {
                                                                            if( construct )
                                                                            {
                                                                                if( x >= 0 && x < VIEW_WIDTH && y >= 0 && y < VIEW_HEIGHT )
                                                                                {
                                                                                    WORD dir;

                                                                                    paint = TRUE;

                                                                                    if( curType == E_CANNON || curType == E_LASER || curType == E_BLASTER || curType == E_BAT )
                                                                                    {
                                                                                        dir = curDir;
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        dir = 0;
                                                                                    }
                                                                                    setCell( &map.ed.map[ y + dy ][ x + dx ], curType, dir, 0 );
                                                                                    px = x;
                                                                                    py = y;
                                                                                }
                                                                            }
                                                                        }
                                                                        else if( code == ( IECODE_LBUTTON | IECODE_UP_PREFIX ) )
                                                                        {
                                                                            paint = FALSE;
                                                                        }
                                                                        else if( code == IECODE_RBUTTON )
                                                                        {
                                                                            if( toolbox )
                                                                            {
                                                                                toolx = toolbox->LeftEdge;
                                                                                tooly = toolbox->TopEdge;
                                                                                CloseWindow( toolbox );
                                                                                total &= ~mask[ 1 ];
                                                                                mask[ 1 ] = 0L;
                                                                                toolbox = NULL;
                                                                            }
                                                                        }
                                                                        else if( code == MENUUP && construct )
                                                                        {
                                                                            if( toolbox )
                                                                            {
                                                                                MoveWindow( toolbox, mx - toolbox->LeftEdge, my - toolbox->TopEdge );
                                                                                toolx = toolbox->LeftEdge;
                                                                                tooly = toolbox->TopEdge;
                                                                            }
                                                                            else
                                                                            {
                                                                                toolbox = openToolbox( w, mx, my, mask );
                                                                                toolx = toolbox->LeftEdge;
                                                                                tooly = toolbox->TopEdge;
                                                                            }
                                                                        }
                                                                    }
                                                                    else if( cls == IDCMP_MOUSEMOVE )
                                                                    {
                                                                        WORD x = mx / TILE_WIDTH, y = my / TILE_HEIGHT;

                                                                        if( construct && paint )
                                                                        {
                                                                            if( x >= 0 && x < VIEW_WIDTH && y >= 0 && y < VIEW_HEIGHT )
                                                                            {
                                                                                if( x != px || y != py )
                                                                                {
                                                                                    WORD dir;

                                                                                    if( curType == E_CANNON || curType == E_LASER || curType == E_BLASTER || curType == E_BAT )
                                                                                    {
                                                                                        dir = curDir;
                                                                                    }
                                                                                    else
                                                                                    {
                                                                                        dir = 0;
                                                                                    }
                                                                                    setCell( &map.ed.map[ y + dy ][ x + dx ], curType, dir, 0 );
                                                                                    px = x;
                                                                                    py = y;
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                    else if( cls == IDCMP_RAWKEY )
                                                                    {
                                                                        if( code == ESC_KEY )
                                                                        {
                                                                            done = TRUE;
                                                                        }
                                                                        else if( code == F1_KEY )
                                                                        {

                                                                        }
                                                                        else if( code == F1_KEY + 1 )
                                                                        {
                                                                            if( construct )
                                                                            {
                                                                                struct FileRequester *fr;
                                                                                struct Screen *auxs;
                                                                                //WORD pens[ NUMDRIPENS + 1 ] = { 0, 3, 1, 2, 1, 3, 2, 0, 2, 1, 2, 1, ~0 };
                                                                                struct ColorSpec colspec[] =
                                                                                {
                                                                                    { 0, 0, 0, 0 },
                                                                                    { -1 }
                                                                                };
                                                                                struct BitMap *auxbm;
                                                                                struct Window *auxw;
                                                                                UBYTE depth = s->ViewPort.RasInfo->BitMap->Depth;
                                                                                WORD i, j;

                                                                                if( auxbm = AllocBitMap( 320, 256, depth, BMF_DISPLAYABLE | BMF_INTERLEAVED, NULL ) )
                                                                                {
                                                                                    if( auxs = OpenScreenTags( NULL,
                                                                                        SA_LikeWorkbench, TRUE,
                                                                                        SA_DisplayID, GetVPModeID( &s->ViewPort ),
                                                                                        SA_Width, 320,
                                                                                        SA_Height, 256,
                                                                                        SA_BitMap, auxbm,
                                                                                        SA_Parent, s,
                                                                                        SA_Exclusive, TRUE,
                                                                                        SA_Draggable, FALSE,
                                                                                        SA_BackFill, LAYERS_NOBACKFILL,
                                                                                        SA_Title, "Robbo Disk Operations",
                                                                                        SA_Pens, pens,
                                                                                        SA_Behind, TRUE,
                                                                                        SA_BlockPen, 0,
                                                                                        SA_DetailPen, 12,
                                                                                        SA_SysFont, 1,
                                                                                        TAG_DONE ) )
                                                                                    {
                                                                                        if( auxw = OpenWindowTags( NULL,
                                                                                            WA_CustomScreen, auxs,
                                                                                            WA_Left, 0,
                                                                                            WA_Top, 0,
                                                                                            WA_Width, auxs->Width,
                                                                                            WA_Height, auxs->Height,
                                                                                            WA_Backdrop, TRUE,
                                                                                            WA_SimpleRefresh, TRUE,
                                                                                            WA_BackFill, LAYERS_NOBACKFILL,
                                                                                            WA_RMBTrap, TRUE,
                                                                                            WA_Borderless, TRUE,
                                                                                            WA_IDCMP, IDCMP_REFRESHWINDOW,
                                                                                            TAG_DONE ) )
                                                                                        {

                                                                                            for( i = 0; i < auxs->ViewPort.ColorMap->Count; i++ )
                                                                                            {
                                                                                                ULONG color[ 3 ], red, green, blue;

                                                                                                GetRGB32( s->ViewPort.ColorMap, i, 1, color );

                                                                                                red = color[ 0 ] >> 24;
                                                                                                green = color[ 1 ] >> 24;
                                                                                                blue = color[ 2 ] >> 24;

                                                                                                SetRGB32CM( auxs->ViewPort.ColorMap, i, RGB( red ), RGB( green ), RGB( blue ) );

                                                                                            }





                                                                                            drawTileRastPort( s->RastPort.BitMap, 0, 0, auxw->RPort, 0, 0, 320, 256, 0xc0 );



                                                                                            WaitBlit();
                                                                                            ScreenDepth( auxs, SDEPTH_TOFRONT | SDEPTH_INFAMILY, NULL );


                                                                                            map.dx = dx;
                                                                                            map.dy = dy;
                                                                                            if( fr = AllocAslRequestTags( ASL_FileRequest,
                                                                                                ASLFR_InitialLeftEdge, auxs->Width / 8,
                                                                                                ASLFR_InitialTopEdge, auxs->Height / 8,
                                                                                                ASLFR_InitialWidth, ( 3 * auxs->Width ) / 4,
                                                                                                ASLFR_InitialHeight, ( 3 * auxs->Height ) / 4,
                                                                                                ASLFR_TitleText, "Select Map file to load",
                                                                                                ASLFR_PositiveText, "Load",
                                                                                                ASLFR_DoSaveMode, FALSE,
                                                                                                ASLFR_InitialPattern, "#?.robbo",
                                                                                                ASLFR_Window, auxw,
                                                                                                ASLFR_DoPatterns, TRUE,
                                                                                                ASLFR_SleepWindow, TRUE,
                                                                                                ASL_HookFunc, ( ULONG )hookFunc,
                                                                                                ASL_FuncFlags, FILF_DOMSGFUNC,
                                                                                                TAG_DONE ) )
                                                                                            {
                                                                                                AslRequestTags( fr, TAG_DONE );
                                                                                                FreeAslRequest( fr );
                                                                                            }
                                                                                            CloseWindow( auxw );
                                                                                        }
                                                                                        CloseScreen( auxs );
                                                                                    }
                                                                                    FreeBitMap( auxbm );
                                                                                }
                                                                            }
                                                                        }
                                                                        else if( code == LEFT_KEY )
                                                                        {
                                                                            set |= 1 << LEFT_DIR;
                                                                        }
                                                                        else if( code == ( LEFT_KEY | IECODE_UP_PREFIX ) )
                                                                        {
                                                                            clear |= 1 << LEFT_DIR;
                                                                        }
                                                                        else if( code == RIGHT_KEY )
                                                                        {
                                                                            set |= 1 << RIGHT_DIR;
                                                                        }
                                                                        else if( code == ( RIGHT_KEY | IECODE_UP_PREFIX ) )
                                                                        {                                                                                                                                                        
                                                                            clear |= 1 << RIGHT_DIR;
                                                                        }
                                                                        else if( code == UP_KEY )
                                                                        {
                                                                            set |= 1 << UP_DIR;
                                                                        }
                                                                        else if( code == ( UP_KEY | IECODE_UP_PREFIX ) )
                                                                        {
                                                                            clear |= 1 << UP_DIR;
                                                                        }
                                                                        else if( code == DOWN_KEY )
                                                                        {
                                                                            set |= 1 << DOWN_DIR;
                                                                        }
                                                                        else if( code == ( DOWN_KEY | IECODE_UP_PREFIX ) )
                                                                        {                                                                                                                                                        
                                                                            clear |= 1 << DOWN_DIR;
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
                                                                        }
                                                                    }
                                                                    ReplyMsg( ( struct Message * )msg );
                                                                }
                                                            }
                                                        }
                                                        if( toolbox )
                                                        {
                                                            CloseWindow( toolbox );
                                                        }
                                                        ClearMenuStrip( w );
                                                    }
                                                    FreeMenus( menu );
                                                }
                                                deleteImage( images + IMG_SAVE );
                                                deleteImage( images + IMG_LOAD );
                                                deleteImage( images + IMG_CLEAR );
                                                closeWindow( w, reg );
                                            }
                                            FreeBitMap( map.gfxBlit );
                                        }
                                        FreeBitMap( map.gfx );
                                    }
                                    FreeVisualInfo( vi );
                                }
                                RemIntServer( INTB_VERTB, &is );
                                freePicture( map.back, s );
                            }
                            CloseLibrary( AslBase );
                        }
                        CloseLibrary( IFFParseBase );
                    }
                    CloseLibrary( LayersBase );
                }
                CloseLibrary( GadToolsBase );
            }
            CloseLibrary( GfxBase );
        }
        CloseLibrary( IntuitionBase );
    }    
    FreeSignal( sigBit );
    return( 0 );
}
