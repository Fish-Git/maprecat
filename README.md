-----------------------------------------------------------------------


# MAPRECAT


    NAME

        maprecat   --   maps or reclaims defective tracks

    SYNOPSIS

        maprecat.exe   {MAP|RECLAIM}   infile   [ outfile ]

    DESCRIPTION

        maprecat reports which tracks of the given dasd volume
        are marked defective with an alternate assigned or else
        reclaims such defective tracks.

    OPTIONS

        action      desired action. MAP will report which tracks
                    are defective whereas RECLAIM will attempt to
                    reclaim them.
        infile      name of emulated dasd image input file
        outfile     name of emulated dasd image output file

    NOTES

        Both input and output are presumed to be raw HDR-30 dasd
        image files as typically used in IBM ADCD distributions.

        The output file is only needed for the RECLAIM function;
        the input file is never modified.

    EXIT STATUS

        Zero if processing was successful. Otherwise the standard
        'C' Runtime errno is returned indicating what went wrong.

    AUTHOR

        "Fish" (David B. Trout)

    VERSION

        1.8  (June 27, 2015)


-----------------------------------------------------------------------


####                    Sample MAP Output


    C:\Users\Fish\> maprecat map dasd1.sanitized
    Run option = map
    Opening input file "dasd1.sanitized"...
    Track  45297 = Cyl 3019 (0BCB), Head 12 (0C) is flagged
    as being defective and has an alternate track assigned.
    Track  45625 = Cyl 3041 (0BE1), Head 10 (0A) is flagged
    as being defective and has an alternate track assigned.
    Track  45626 = Cyl 3041 (0BE1), Head 11 (0B) is flagged
    as being defective and has an alternate track assigned.
    [...]
    Track  45840 = Cyl 3056 (0BF0), Head  0 (00) is flagged
    as being defective and has an alternate track assigned.
    50085 tracks (3339 cylinders) read.
    Image currently has 217 defective tracks assigned to an alternate.



####                    Sample RECLAIM Output


    C:\Users\Fish> maprecat reclaim dasd1.sanitized dasd1.reclaimed
    Run option = reclaim
    Opening input file "dasd1.sanitized"...
    Opening output file "dasd1.reclaimed"...
    Reclaiming track  45297 = Cyl 3019 (0BCB), Head 12 (0C)...
    Reclaiming track  45625 = Cyl 3041 (0BE1), Head 10 (0A)...
    Reclaiming track  45626 = Cyl 3041 (0BE1), Head 11 (0B)...
    [...]
    Reclaiming track  45840 = Cyl 3056 (0BF0), Head  0 (00)...
    50085 tracks (3339 cylinders) read.
    50085 tracks (3339 cylinders) written.
    Image had 217 defective tracks assigned to an alternate.
    A total of 217 defective tracks were reclaimed from their assigned alternate.
    Reclaim was successful.


-----------------------------------------------------------------------
