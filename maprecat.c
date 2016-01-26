/*********************************************************************/
/*             MAPRECAT: map or reclaim defective tracks             */
/*********************************************************************/

#include "helpers.h"
#include "hexdumpe.h"





/*********************************************************************/
/*                          VERSION                                  */
/*********************************************************************/

#define VERSION     "1.8.1  (June 29, 2015)"






/*********************************************************************/
/*                      DEBUGGING OPTIONS                            */
/*********************************************************************/

#define  DEBUG_NONE         0x00000000
#define  DEBUG_POSITIONS    0x80000000
#define  DEBUG_TRACKDATA    0x40000000 /* See WARNING further below! */
#define  DEBUG_BOTH         (DEBUG_POSITIONS | DEBUG_TRACKDATA)


//#define  DEBUGOPT           DEBUG_NONE
//#define  DEBUGOPT           DEBUG_POSITIONS
//#define  DEBUGOPT           DEBUG_TRACKDATA   /** Warning! **/
//#define  DEBUGOPT           DEBUG_BOTH        /** Warning! **/


/*                      ***  WARNING!  ***

    Building with DEBUGOPT set to DEBUG_TRACKDATA (or DEBUG_BOTH)
    will create a FREAKING HUGE amount of output! (depending on
    on how many alternate tracks there actually are, of course).

                        USE AT YOUR OWN RISK!
*/

/*********************************************************************/

#define  FILEHDRSIZE        (512)
#define  TRACKSIZE          (56832)     /* CKD_P370 format */

#define  BAD_TRACK_SIG      0x56,0x45,0x52,0x30,0xF1,0xE2
//                          ASCII "VER0" + EBCDIC "1S"

#define  CYLS3390_1         (1113)
#define  CYLS3390_3         (CYLS3390_1 * 3)
#define  CYLS3390_9         (CYLS3390_1 * 9)

#define  TRACKSPERCYL       (15)
#define  TRKS3390_3         (CYLS3390_3 * TRACKSPERCYL)
#define  TRKS3390_9         (CYLS3390_9 * TRACKSPERCYL)





/*-------------------------------------------------------------------*\
    FIXME: make numtracks (or perhaps model number?) a command-line
    argument instead of hard coding it like we currently are below.
\*-------------------------------------------------------------------*/

#define  NUMTRACKS          (TRKS3390_3)    // today
//#define  NUMTRACKS          (TRKS3390_9)    // future?
#define  NUMCYLS            (NUMTRACKS ? (NUMTRACKS / TRACKSPERCYL) : 0)





U32 REMAINING = NUMTRACKS;  /* Total number of tracks to process */

#define  TRACKNUM           (NUMTRACKS - REMAINING)
#define  POS2TRACK( pos )   ((U32)(((pos) - FILEHDRSIZE) / TRACKSIZE))
#define  CYLNUM( trk )      ((trk) ? ((trk) / TRACKSPERCYL) : 0)
#define  HEADNUM( trk )     ((trk) - (CYLNUM( trk ) * TRACKSPERCYL))

/*********************************************************************/
/*             Issue error message and return code                   */
/*********************************************************************/
static int error( int rc, char *msg )
{
    fprintf( stderr, "ERROR: %s: %s\n", msg, strerror( rc ));

    if (EINVAL == rc)
    {
        /* Display syntax help */

        fprintf( stderr, "\n"

"    NAME\n\n"

"        maprecat   --   maps or reclaims defective tracks\n\n"

"    SYNOPSIS\n\n"

"        maprecat.exe   {MAP|RECLAIM}   infile   [ outfile ]\n\n"

"    DESCRIPTION\n\n"

"        maprecat reports which tracks of the given dasd volume\n"
"        are marked defective with an alternate assigned or else\n"
"        reclaims such defective tracks.\n\n"

"    OPTIONS\n\n"

"        action      desired action. MAP will report which tracks\n"
"                    are defective whereas RECLAIM will attempt to\n"
"                    reclaim them.\n"
"        infile      name of emulated dasd image input file\n"
"        outfile     name of emulated dasd image output file\n\n"

"    NOTES\n\n"

"        Both input and output are presumed to be raw 3390 model 3\n"
"        HDR-30 dasd image files as are typically used in IBM ADCD\n"
"        distributions. Support for other dasd types and models may\n"
"        be provided in a future release.\n\n"

"        The output file is only needed for the RECLAIM function;\n"
"        the input file is never modified.\n\n"

"    EXIT STATUS\n\n"

"        Zero if processing was successful. Otherwise the standard\n"
"        'C' Runtime errno is returned indicating what went wrong.\n\n"

"    AUTHOR\n\n"

"        \"Fish\" (David B. Trout)\n\n"

"    VERSION\n\n"

"        "VERSION"\n\n"


        );
    }

    PAUSEIFBEINGDEBUGGED();
    return rc;
}

/*********************************************************************/
/*    Copy dasd image file reclaiming defective tracks as we go      */
/*********************************************************************/
int main( int argc, char *argv[] )
{
    U64 nxtpos;                         /* file position of next read */
    U64 altpos;                         /* file position of alternate track */
    U64 orgpos;                         /* alternate track's file position of original track */
    U32 numbad = 0;                     /* Counts defective tracks */
    U32 reclaimed = 0;                  /* Counts reclaimed tracks */

    int infile  = -1;                   /* Input  file descriptor integer */
    int outfile = -1;                   /* output file descriptor integer */
    int cyl;                            /* Cylinder number */
    int head;                           /* Head number */
    int track;                          /* Track number */
    int rc;                             /* Return code */

    U8 trackbuf[TRACKSIZE];             /* Primary track buffer */
    U8 alttrack[TRACKSIZE];             /* Alternate track buffer */

    U8 altmark[6] = { BAD_TRACK_SIG };  /* Defective track signature */
    U8 reclaim = 0;                     /* false: map, true: reclaim */

    /*----------------*/
    /* Initialization */
    /*----------------*/

    if (argc < 3 || argc > 4)
        return error( EINVAL, "invalid number of arguments" );

    if (1
        && strcmp( argv[1], "MAP"     ) != 0
        && strcmp( argv[1], "map"     ) != 0
        && strcmp( argv[1], "RECLAIM" ) != 0
        && strcmp( argv[1], "reclaim" ) != 0
    )
        return error( EINVAL, "invalid 'action'" );

    reclaim = ((strcmp( argv[1], "RECLAIM" ) == 0) ||
               (strcmp( argv[1], "reclaim" ) == 0));

    if (reclaim && argc < 4)
        return error( EINVAL, "missing outfile argument" );

    printf( "Run option = %s\n", argv[1] );

    printf( "Opening input file \"%s\"...\n", argv[2] );

    if ((infile = HOPEN( argv[2], O_RDONLY | O_BINARY )) < 0)
        return error( errno, "could not open input file" );

    if (reclaim)
    {
        printf( "Opening output file \"%s\"...\n", argv[3] );

        if ((outfile = HOPEN( argv[3], O_CREAT | O_EXCL | O_WRONLY | O_BINARY,
            S_IRUSR | S_IWUSR | S_IRGRP )) < 0)
            return error( errno, "could not open output file" );
    }

    /*------------------*/
    /* Copy file header */
    /*------------------*/

    read( infile, trackbuf, FILEHDRSIZE );

    if (reclaim)
        write( outfile, trackbuf, FILEHDRSIZE );

c:  /*---------------------------*/
    /* Start of copy tracks loop */
    /*---------------------------*/

    track = TRACKNUM;
    cyl   = CYLNUM ( track );
    head  = HEADNUM( track );

    if (!head)
        printf( "Inspecting cylinder %d...\r", cyl );

    /* Read next track */

    if ((rc = read( infile, trackbuf, TRACKSIZE )) <= 0 || rc != TRACKSIZE)
        return error( errno, "I/O error reading input file" );

    /*------------------*/
    /* Defective track? */
    /*------------------*/

    if (memcmp( &trackbuf[TRACKSIZE-16], altmark, 6 ) == 0)
    {
        numbad++;     /* Count defective tracks */

        if (reclaim)
            printf( "Reclaiming track %6d = Cyl %4d (%4.4X), Head %2d (%2.2X)...\n",
                track, cyl, cyl, head, head );
        else
            printf( "Track %6d = Cyl %4d (%4.4X), Head %2d (%2.2X) is flagged\nas being defective and has an alternate track assigned.\n",
                track, cyl, cyl, head, head );

        FETCH_DW( altpos, &trackbuf[ TRACKSIZE-8 ] );   /* get alternate track position */
nxtpos= lseek( infile,   0,      SEEK_CUR  );           /* save current file position */
        lseek( infile, altpos,   SEEK_SET  );           /* position to alternate */
        read ( infile, alttrack, TRACKSIZE );           /* read alternate track */
        lseek( infile, nxtpos,   SEEK_SET  );           /* restore original file position */
        FETCH_DW( orgpos, &alttrack[ TRACKSIZE-8 ] );   /* get original track position */


        /*----------------------------------------------*/
#if (DEBUGOPT & DEBUG_POSITIONS)
        {
            int alttrk   = POS2TRACK( altpos );
            int altcyl   = CYLNUM ( alttrk );
            int althead  = HEADNUM( alttrk );

            int orgtrk   = POS2TRACK( orgpos );
            int orgcyl   = CYLNUM ( orgtrk );
            int orghead  = HEADNUM( orgtrk );

            printf("\n");

            printf( "\taltpos = %llx ==> track %6d = Cyl %4d (%4.4X), Head %2d (%2.2X)\n",
                altpos, alttrk, altcyl, altcyl, althead, althead );

            printf( "\torgpos = %llx ==> track %6d = Cyl %4d (%4.4X), Head %2d (%2.2X)\n",
                orgpos, orgtrk, orgcyl, orgcyl, orghead, orghead );
        }
#endif

#if (DEBUGOPT & DEBUG_TRACKDATA)
        {
            char *dump = NULL;

            printf("\n");

            hexdumpe( "\tPRI:  ", &dump, trackbuf, 0, TRACKSIZE, orgpos, 4, 4 );
            if (dump) printf( "%s\n", dump );

            hexdumpe( "\tALT:  ", &dump, alttrack, 0, TRACKSIZE, altpos, 4, 4 );
            if (dump) printf( "%s", dump );

            if (dump) free( dump );
        }
#endif

#if (DEBUGOPT & DEBUG_BOTH) /* if EITHER debug option */
        printf("\n");       /* if EITHER debug option */
#endif
        /*----------------------------------------------*/


        if (reclaim)
        {
            /* Reclaim the original track: the alternate tracks's
               position of the original defective track must match,
               and the track header containing the cylinder and track
               numbers must also match.
            */
            if (orgpos == (nxtpos-TRACKSIZE) &&
                memcmp( &alttrack[0], &trackbuf[0], 16 ) == 0)
            {
                /* copy alternate track data to original track */
                memcpy( trackbuf, alttrack, TRACKSIZE );

                /* indicate original track is no longer defective */
                memset( &trackbuf[TRACKSIZE-16], 0, 16 );

                reclaimed++;    /* Count reclaimed tracks */
            }
            else
                fprintf( stderr, "\t*** ERROR *** Reclaim failed!\n\n" );
        }
    }

    /*----------------------------*/
    /* Write track to output file */
    /*----------------------------*/

    if (reclaim)
        if ((rc = write( outfile, trackbuf, TRACKSIZE )) < 0 || rc != TRACKSIZE)
            return error( EIO, "I/O error writing output file" );

    /*------------------------------*/
    /* Loop until all tracks copied */
    /*------------------------------*/

    if (--REMAINING)
        goto c;

    /*-----------------------*/
    /* Print totals and exit */
    /*-----------------------*/

    if (infile  > 0) close( infile  );
    if (outfile > 0) close( outfile );

    printf( "%d tracks (%d cylinders) read.\n",    NUMTRACKS, NUMCYLS );
    if (reclaim)
    printf( "%d tracks (%d cylinders) written.\n", NUMTRACKS, NUMCYLS );

    if (!reclaim)
        printf( "Image currently has %d defective tracks assigned to an alternate.\n", numbad );
    else
    {
        printf( "Image had %d defective tracks assigned to an alternate.\n", numbad );
        printf( "A total of %d defective tracks were reclaimed from their assigned alternate.\n", reclaimed );
        printf( "Reclaim %s.\n", reclaimed >= numbad ? "was successful" : "function FAILED" );
    }

    PAUSEIFBEINGDEBUGGED();

    return (0);
}
