
#include <devices/console.h>
#include <devices/conunit.h>
#include <intuition/intuition.h>

#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>

struct Window *win;
UBYTE buffer[ 32 ];

struct TextFont *topazFont;

struct TextAttr topaz = { "topaz.font", 8, FS_NORMAL, FPF_ROMFONT | FPF_DESIGNED };

extern struct Library *ConsoleDevice;

void closeConsole( struct IOStdReq *io )
{
    struct MsgPort *mp = io->io_Message.mn_ReplyPort;

    CloseDevice( ( struct IORequest * )io );
    DeleteIORequest( io );
    DeleteMsgPort( mp );

    CloseWindow( win );
}

struct IOStdReq *openConsole( struct Screen *s )
{
    struct Window *w;

    if( w = OpenWindowTags( NULL,
        WA_CustomScreen, s,
        WA_Left, 12,
        WA_Top, 203,
        WA_Width, 218,
        WA_Height, 44,
        WA_Borderless, TRUE,
        WA_SimpleRefresh, TRUE,
        WA_BackFill, LAYERS_NOBACKFILL,
        WA_RMBTrap, TRUE,        
        TAG_DONE ) )
    {
        struct MsgPort *mp;

        if( topazFont = OpenFont( &topaz ) )
        {
            SetFont( w->RPort, topazFont );

            if( mp = CreateMsgPort() )
            {
                struct IOStdReq *io;

                if( io = CreateIORequest( mp, sizeof( *io ) ) )
                {
                    io->io_Data = w;
                    io->io_Length = sizeof( *w );
                    if( OpenDevice( "console.device", CONU_CHARMAP, ( struct IORequest * )io, CONFLAG_NODRAW_ON_NEWSIZE ) == 0 )
                    {
                        BYTE text[] = "\x1b[32;41;>1\x6dWhat's your name?\n";

                        ConsoleDevice = ( struct Library * )io->io_Device;

                        io->io_Data = text;
                        io->io_Length = sizeof( text ) - 1;
                        io->io_Command = CMD_WRITE;
                        DoIO( ( struct IORequest * )io );

                        io->io_Data = buffer;
                        io->io_Length = 32;
                        io->io_Command = CMD_READ;
                        SendIO( ( struct IORequest * )io );

                        win = w;
                        return( io );
                    }
                    DeleteIORequest( io );
                }
                DeleteMsgPort( mp );
            }
            CloseFont( topazFont );
        }
        CloseWindow( w );
    }
    return( NULL );
}
