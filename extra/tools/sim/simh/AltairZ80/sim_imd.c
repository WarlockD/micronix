/*************************************************************************
 *                                                                       *
 * $Id: sim_imd.c 1999 2008-07-22 04:25:28Z hharte $                     *
 *                                                                       *
 * Copyright (c) 2007-2008 Howard M. Harte.                              *
 * http://www.hartetec.com                                               *
 *                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining *
 * a copy of this software and associated documentation files (the       *
 * "Software"), to deal in the Software without restriction, including   *
 * without limitation the rights to use, copy, modify, merge, publish,   *
 * distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to *
 * the following conditions:                                             *
 *                                                                       *
 * The above copyright notice and this permission notice shall be        *
 * included in all copies or substantial portions of the Software.       *
 *                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                 *
 * NONINFRINGEMENT. IN NO EVENT SHALL HOWARD M. HARTE BE LIABLE FOR ANY  *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                *
 *                                                                       *
 * Except as contained in this notice, the name of Howard M. Harte shall *
 * not be used in advertising or otherwise to promote the sale, use or   *
 * other dealings in this Software without prior written authorization   *
 * Howard M. Harte.                                                      *
 *                                                                       *
 * SIMH Interface based on altairz80_hdsk.c, by Peter Schorn.            *
 *                                                                       *
 * Module Description:                                                   *
 *     ImageDisk (IMD) Disk Image File access module for SIMH.           *
 *     see: http://www.classiccmp.org/dunfield/img/index.htm             *
 *     for details on the ImageDisk format and other utilities.          *
 *                                                                       *
 * Environment:                                                          *
 *     User mode only                                                    *
 *                                                                       *
 *************************************************************************/

/* Change log:
     - 06-Aug-2008, Tony Nicholson, Add support for logical Head and
                    Cylinder maps in the .IMD image file (AGN)
*/

#include "sim_defs.h"
#include "sim_imd.h"
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>     /* for _chsize() */
#else
#include <unistd.h>
#endif
/* #define DBG_MSG */

#ifdef DBG_MSG
#define DBG_PRINT(args) printf args
#else
#define DBG_PRINT(args)
#endif

/* use NLP for new line printing while the simulation is running */
#if defined (__linux) || defined(__NetBSD__) || defined (__OpenBSD__) || defined (__FreeBSD__) || defined (__APPLE__)
#define UNIX_PLATFORM 1
#else
#define UNIX_PLATFORM 0
#endif

#if UNIX_PLATFORM
#define NLP "\r\n"
#else
#define NLP "\n"
#endif

#if (defined (__MWERKS__) && defined (macintosh)) || defined(__DECC)
#define __FUNCTION__ __FILE__
#endif

static t_stat commentParse(DISK_INFO *myDisk, uint8 comment[], uint32 buffLen);
static t_stat diskParse(DISK_INFO *myDisk, uint32 isVerbose);
static t_stat diskFormat(DISK_INFO *myDisk);

/* Open an existing IMD disk image.  It will be opened and parsed, and after this
 * call, will be ready for sector read/write. The result is the corresponding
 * DISK_INFO or NULL if an error occurred.
 */
DISK_INFO *diskOpen(FILE *fileref, uint32 isVerbose)
{
    DISK_INFO *myDisk = NULL;

    myDisk = (DISK_INFO *)malloc(sizeof(DISK_INFO));
    myDisk->file = fileref;

    if (diskParse(myDisk, isVerbose) != SCPE_OK) {
        free(myDisk);
        myDisk = NULL;
    }

    return myDisk;
}

/* Scans the IMD file for the comment string, and returns it in comment buffer.
 * After this function returns, the file pointer is placed after the comment and
 * the 0x1A "EOF" marker.
 *
 * The comment parameter is optional, and if NULL, then the ocmment will not
 * be extracted from the IMD file, but the file position will still be advanced
 * to the end of the comment.
 */
static t_stat commentParse(DISK_INFO *myDisk, uint8 comment[], uint32 buffLen)
{
    uint8 cData;
    uint32 commentLen = 0;

    /* rewind to the beginning of the file. */
    rewind(myDisk->file);
    cData = fgetc(myDisk->file);
    while ((!feof(myDisk->file)) && (cData != 0x1a)) {
        if ((comment != NULL) && (commentLen < buffLen)) {
            comment[commentLen++] = cData;
        }
        cData = fgetc(myDisk->file);
    }
    if (comment != NULL) {
        if (commentLen == buffLen)
            commentLen--;
        comment[commentLen] = 0;
    }
    return SCPE_OK;
}

static uint32 headerOk(IMD_HEADER imd) {
    return (imd.cyl < MAX_CYL) && (imd.head < MAX_HEAD);
}

/* Parse an IMD image.  This sets up sim_imd to be able to do sector read/write and
 * track write.
 */
static t_stat diskParse(DISK_INFO *myDisk, uint32 isVerbose)
{
    uint8 comment[256];
    uint8 sectorMap[256];
    uint8 sectorHeadMap[256];
    uint8 sectorCylMap[256];
    uint32 sectorSize, sectorHeadwithFlags, sectRecordType;
    uint32 i;
    uint8 start_sect;

    uint32 TotalSectorCount = 0;
    IMD_HEADER imd;

    if(myDisk == NULL) {
        return (SCPE_OPENERR);
    }

    memset(myDisk->track, 0, (sizeof(TRACK_INFO)*MAX_CYL*MAX_HEAD));

    if (commentParse(myDisk, comment, sizeof(comment)) != SCPE_OK) {
        return (SCPE_OPENERR);
    }

    if(isVerbose)
        printf("%s" NLP, comment);

    myDisk->nsides = 1;
    myDisk->ntracks = 0;
    myDisk->flags = 0;      /* Make sure all flags are clear. */

    if(feof(myDisk->file)) {
        printf("SIM_IMD: Disk image is blank, it must be formatted." NLP);
        return (SCPE_OPENERR);
    }

    do {
        DBG_PRINT(("start of track %d at file offset %ld" NLP, myDisk->ntracks, ftell(myDisk->file)));

        sim_fread(&imd, 1, 5, myDisk->file);
        if (feof(myDisk->file))
            break;
        sectorSize = 128 << imd.sectsize;
        sectorHeadwithFlags = imd.head; /*AGN save the head and flags */
        imd.head &= 1 ; /*AGN mask out flag bits to head 0 or 1 */

        DBG_PRINT(("Track %d:" NLP, myDisk->ntracks));
        DBG_PRINT(("\tMode=%d, Cyl=%d, Head=%d(%d), #sectors=%d, sectsize=%d (%d bytes)" NLP, imd.mode, imd.cyl, sectorHeadwithFlags, imd.head, imd.nsects, imd.sectsize, sectorSize));

        if (!headerOk(imd)) {
            printf("SIM_IMD: Corrupt header." NLP);
            return (SCPE_OPENERR);
        }

        if((imd.head + 1) > myDisk->nsides) {
            myDisk->nsides = imd.head + 1;
        }

        myDisk->track[imd.cyl][imd.head].mode = imd.mode;
        myDisk->track[imd.cyl][imd.head].nsects = imd.nsects;
        myDisk->track[imd.cyl][imd.head].sectsize = sectorSize;

        if (sim_fread(sectorMap, 1, imd.nsects, myDisk->file) != imd.nsects) {
            printf("SIM_IMD: Corrupt file [Sector Map]." NLP);
            return (SCPE_OPENERR);
        }
        myDisk->track[imd.cyl][imd.head].start_sector = imd.nsects;
        DBG_PRINT(("\tSector Map: "));
        for(i=0;i<imd.nsects;i++) {
            DBG_PRINT(("%d ", sectorMap[i]));
            if(sectorMap[i] < myDisk->track[imd.cyl][imd.head].start_sector) {
                myDisk->track[imd.cyl][imd.head].start_sector = sectorMap[i];
            }
        }
        DBG_PRINT((", Start Sector=%d", myDisk->track[imd.cyl][imd.head].start_sector));

        if(sectorHeadwithFlags & IMD_FLAG_SECT_HEAD_MAP) {
            if (sim_fread(sectorHeadMap, 1, imd.nsects, myDisk->file) != imd.nsects) {
                printf("SIM_IMD: Corrupt file [Sector Head Map]." NLP);
                return (SCPE_OPENERR);
            }
            DBG_PRINT(("\tSector Head Map: "));
            for(i=0;i<imd.nsects;i++) {
                DBG_PRINT(("%d ", sectorHeadMap[i]));
            }
            DBG_PRINT(("" NLP));
        } else {
            /* Default Head is physical head for each sector */
            for(i=0;i<imd.nsects;i++) {
                sectorHeadMap[i] = imd.head;
            };
        }

        if(sectorHeadwithFlags & IMD_FLAG_SECT_CYL_MAP) {
            if (sim_fread(sectorCylMap, 1, imd.nsects, myDisk->file) != imd.nsects) {
                printf("SIM_IMD: Corrupt file [Sector Cyl Map]." NLP);
                return (SCPE_OPENERR);
            }
            DBG_PRINT(("\tSector Cyl Map: "));
            for(i=0;i<imd.nsects;i++) {
                DBG_PRINT(("%d ", sectorCylMap[i]));
            }
            DBG_PRINT((NLP));
        } else {
            /* Default Cyl Map is physical cylinder for each sector */
            for(i=0;i<imd.nsects;i++) {
                sectorCylMap[i] = imd.cyl;
            }
        }

        DBG_PRINT((NLP "Sector data at offset 0x%08lx" NLP, ftell(myDisk->file)));

        /* Build the table with location 0 being the start sector. */
        start_sect = myDisk->track[imd.cyl][imd.head].start_sector;

        /* Now read each sector */
        for(i=0;i<imd.nsects;i++) {
            TotalSectorCount++;
            DBG_PRINT(("Sector Phys: %d/Logical: %d: %d bytes: ", i, sectorMap[i], sectorSize));
            sectRecordType = fgetc(myDisk->file);
            /* AGN Logical head mapping */
            myDisk->track[imd.cyl][imd.head].logicalHead[i] = sectorHeadMap[i];
            /* AGN Logical cylinder mapping */
            myDisk->track[imd.cyl][imd.head].logicalCyl[i] = sectorCylMap[i];
            switch(sectRecordType) {
                case SECT_RECORD_UNAVAILABLE:   /* Data could not be read from the original media */
                    if (sectorMap[i]-start_sect < MAX_SPT)
                        myDisk->track[imd.cyl][imd.head].sectorOffsetMap[sectorMap[i]-start_sect] = 0xBADBAD;
                    else {
                        printf("SIM_IMD: ERROR: Illegal sector offset %d" NLP, sectorMap[i]-start_sect);
                        return (SCPE_OPENERR);
                    }
                    break;
                case SECT_RECORD_NORM:          /* Normal Data */
                case SECT_RECORD_NORM_DAM:      /* Normal Data with deleted address mark */
                case SECT_RECORD_NORM_ERR:      /* Normal Data with read error */
                case SECT_RECORD_NORM_DAM_ERR:  /* Normal Data with deleted address mark with read error */
/*                  DBG_PRINT(("Uncompressed Data" NLP)); */
                    if (sectorMap[i]-start_sect < MAX_SPT) {
                        myDisk->track[imd.cyl][imd.head].sectorOffsetMap[sectorMap[i]-start_sect] = ftell(myDisk->file);
                        sim_fseek(myDisk->file, sectorSize, SEEK_CUR);
                    }
                    else {
                        printf("SIM_IMD: ERROR: Illegal sector offset %d" NLP, sectorMap[i]-start_sect);
                        return (SCPE_OPENERR);
                    }
                    break;
                case SECT_RECORD_NORM_COMP:     /* Compressed Normal Data */
                case SECT_RECORD_NORM_DAM_COMP: /* Compressed Normal Data with deleted address mark */
                case SECT_RECORD_NORM_COMP_ERR: /* Compressed Normal Data */
                case SECT_RECORD_NORM_DAM_COMP_ERR: /* Compressed Normal Data with deleted address mark */
                    if (sectorMap[i]-start_sect < MAX_SPT) {
                        myDisk->track[imd.cyl][imd.head].sectorOffsetMap[sectorMap[i]-start_sect] = ftell(myDisk->file);
                        myDisk->flags |= FD_FLAG_WRITELOCK; /* Write-protect the disk if any sectors are compressed. */
#ifdef VERBOSE_DEBUG
                        DBG_PRINT(("Compressed Data = 0x%02x" NLP, fgetc(myDisk->file)));
#else
                        fgetc(myDisk->file);
#endif
                    }
                    else {
                        printf("SIM_IMD: ERROR: Illegal sector offset %d" NLP, sectorMap[i]-start_sect);
                        return (SCPE_OPENERR);
                    }
                    break;
                default:
                    printf("SIM_IMD: ERROR: unrecognized sector record type %d" NLP, sectRecordType);
                    return (SCPE_OPENERR);
                    break;
            }
            DBG_PRINT((NLP));
        }

        myDisk->ntracks++;
    } while (!feof(myDisk->file));

    DBG_PRINT(("Processed %d sectors" NLP, TotalSectorCount));

#ifdef VERBOSE_DEBUG
    for(i=0;i<myDisk->ntracks;i++) {
        DBG_PRINT(("Track %02d: ", i));
        for(j=0;j<imd.nsects;j++) {
            DBG_PRINT(("0x%06x ", myDisk->track[i][0].sectorOffsetMap[j]));
        }
        DBG_PRINT((NLP));
    }
#endif
    if(myDisk->flags & FD_FLAG_WRITELOCK) {
        printf("Disk write-protected because the image contains compressed sectors. Use IMDU to uncompress." NLP);
    }

    return SCPE_OK;
}

/*
 * This function closes the IMD image.  After closing, the sector read/write operations are not
 * possible.
 *
 * The IMD file is not actually closed, we leave that to SIMH.
 */
t_stat diskClose(DISK_INFO **myDisk)
{
    if(*myDisk == NULL)
        return SCPE_OPENERR;
    free(*myDisk);
    *myDisk = NULL;
    return SCPE_OK;
}

#define MAX_COMMENT_LEN 256

/*
 * Create an ImageDisk (IMD) file.  This function just creates the comment header, and allows
 * the user to enter a comment.  After the IMD is created, it must be formatted with a format
 * program on the simulated operating system, ie CP/M, CDOS, 86-DOS.
 *
 * If the IMD file already exists, the user will be given the option of overwriting it.
 */
t_stat diskCreate(FILE *fileref, char *ctlr_comment)
{
    DISK_INFO *myDisk = NULL;
    char *comment;
    char *curptr;
    char *result;
    uint8 answer;
    int32 len, remaining;

    if(fileref == NULL) {
        return (SCPE_OPENERR);
    }

    if(sim_fsize(fileref) != 0) {
        printf("SIM_IMD: Disk image already has data, do you want to overwrite it? ");
        answer = getchar();

        if((answer != 'y') && (answer != 'Y')) {
            return (SCPE_OPENERR);
        }
    }

    if((curptr = comment = calloc(1, MAX_COMMENT_LEN)) == 0) {
        printf("Memory allocation failure.\n");
        return (SCPE_MEM);
    }

    printf("SIM_IMD: Enter a comment for this disk.\n"
           "SIM_IMD: Terminate with a '.' on an otherwise blank line.\n");
    remaining = MAX_COMMENT_LEN;
    do {
        printf("IMD> ");
        result = fgets(curptr, remaining - 3, stdin);
        if ((result == NULL) || (strcmp(curptr, ".\n") == 0)) {
            remaining = 0;
        } else {
            len = strlen(curptr) - 1;
            if (curptr[len] != '\n')
                len++;
            remaining -= len;
            curptr += len;
            *curptr++ = 0x0d;
            *curptr++ = 0x0a;
        }
    } while (remaining > 4);
    *curptr = 0x00;

    /* rewind to the beginning of the file. */
    rewind(fileref);

    /* Erase the contents of the IMD file in case we are overwriting an existing image. */
#ifdef _WIN32 /* This might work under UNIX and/or VMS since this POSIX, but I haven't tried it. */
    _chsize(_fileno(fileref), ftell (fileref));
#else
    if (ftruncate(fileno(fileref), ftell (fileref)) == -1) {
        printf("SIM_IMD: Error overwriting disk image.\n");
        return(SCPE_OPENERR);
    }
#endif

    fprintf(fileref, "IMD SIMH %s %s\n", __DATE__, __TIME__);
    fputs(comment, fileref);
    free(comment);
    fprintf(fileref, "\n\n$Id: sim_imd.c 1999 2008-07-22 04:25:28Z hharte $\n");
    fprintf(fileref, "%s\n", ctlr_comment);
    fputc(0x1A, fileref); /* EOF marker for IMD comment. */
    fflush(fileref);

    if((myDisk = diskOpen(fileref, 0)) == NULL) {
        printf("SIM_IMD: Error opening disk for format.\n");
        return(SCPE_OPENERR);
    }

    if(diskFormat(myDisk) != SCPE_OK) {
        printf("SIM_IMD: error formatting disk.\n");
    }

    return diskClose(&myDisk);
}


t_stat diskFormat(DISK_INFO *myDisk)
{
    uint8 i;
    uint8 sector_map[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};
    uint32 flags;

    printf("SIM_IMD: Formatting disk in IBM 3740 SS/SD Format.\n");

    for(i=0;i<77;i++) {
        if((trackWrite(myDisk, i, 0, 26, 128, sector_map, IMD_MODE_500K_FM, 0xE5, &flags)) != 0) {
            printf("SIM_IMD: Error formatting track %d\n", i);
            return SCPE_IOERR;
        } else {
            putchar('.');
        }
    }

    printf("\nSIM_IMD: Format Complete.\n");

    return SCPE_OK;
}

uint32 imdGetSides(DISK_INFO *myDisk)
{
    if(myDisk != NULL) {
        return(myDisk->nsides);
    }

    return (0);
}

uint32 imdIsWriteLocked(DISK_INFO *myDisk)
{
    if(myDisk != NULL) {
        return((myDisk->flags & FD_FLAG_WRITELOCK) ? 1 : 0);
    }

    return (0);
}

/* Check that the given track/sector exists on the disk */
t_stat sectSeek(DISK_INFO *myDisk,
             uint32 Cyl,
             uint32 Head)
{
    if(Cyl >= myDisk->ntracks) {
        return(SCPE_IOERR);
    }

    if(Head >= myDisk->nsides) {
        return(SCPE_IOERR);
    }

    if(myDisk->track[Cyl][Head].nsects == 0) {
        DBG_PRINT(("%s: invalid track/head" NLP, __FUNCTION__));
        return(SCPE_IOERR);
    }

    return(SCPE_OK);
}

/* Read a sector from an IMD image. */
t_stat sectRead(DISK_INFO *myDisk,
             uint32 Cyl,
             uint32 Head,
             uint32 Sector,
             uint8 *buf,
             uint32 buflen,
             uint32 *flags,
             uint32 *readlen)
{
    uint32 sectorFileOffset;
    uint8 sectRecordType;
    uint8 start_sect;
    *readlen = 0;
    *flags = 0;

    /* Check parameters */
    if(myDisk == NULL) {
        *flags |= IMD_DISK_IO_ERROR_GENERAL;
        return(SCPE_IOERR);
    }

    if(sectSeek(myDisk, Cyl, Head) != SCPE_OK) {
        *flags |= IMD_DISK_IO_ERROR_GENERAL;
        return(SCPE_IOERR);
    }

    if(Sector > myDisk->track[Cyl][Head].nsects) {
        DBG_PRINT(("%s: invalid sector" NLP, __FUNCTION__));
        *flags |= IMD_DISK_IO_ERROR_GENERAL;
        return(SCPE_IOERR);
    }

    if(buflen < myDisk->track[Cyl][Head].sectsize) {
        printf("%s: Reading C:%d/H:%d/S:%d, len=%d: user buffer too short, need %d" NLP, __FUNCTION__, Cyl, Head, Sector, buflen, myDisk->track[Cyl][Head].sectsize);
        *flags |= IMD_DISK_IO_ERROR_GENERAL;
        return(SCPE_IOERR);
    }

    start_sect = myDisk->track[Cyl][Head].start_sector;

    sectorFileOffset = myDisk->track[Cyl][Head].sectorOffsetMap[Sector-start_sect];

    DBG_PRINT(("Reading C:%d/H:%d/S:%d, len=%d, offset=0x%08x" NLP, Cyl, Head, Sector, buflen, sectorFileOffset));

    sim_fseek(myDisk->file, sectorFileOffset-1, 0);

    sectRecordType = fgetc(myDisk->file);
    switch(sectRecordType) {
        case SECT_RECORD_UNAVAILABLE:   /* Data could not be read from the original media */
            *flags |= IMD_DISK_IO_ERROR_GENERAL;
            break;
        case SECT_RECORD_NORM_ERR:      /* Normal Data with read error */
        case SECT_RECORD_NORM_DAM_ERR:  /* Normal Data with deleted address mark with read error */
            *flags |= IMD_DISK_IO_ERROR_CRC;
        case SECT_RECORD_NORM:          /* Normal Data */
        case SECT_RECORD_NORM_DAM:      /* Normal Data with deleted address mark */

/*          DBG_PRINT(("Uncompressed Data" NLP)); */
            if (sim_fread(buf, 1, myDisk->track[Cyl][Head].sectsize, myDisk->file) != myDisk->track[Cyl][Head].sectsize) {
                printf("SIM_IMD[%s]: sim_fread error for SECT_RECORD_NORM_DAM." NLP, __FUNCTION__);
            }
            *readlen = myDisk->track[Cyl][Head].sectsize;
            break;
        case SECT_RECORD_NORM_COMP_ERR: /* Compressed Normal Data */
        case SECT_RECORD_NORM_DAM_COMP_ERR: /* Compressed Normal Data with deleted address mark */
            *flags |= IMD_DISK_IO_ERROR_CRC;
        case SECT_RECORD_NORM_COMP:     /* Compressed Normal Data */
        case SECT_RECORD_NORM_DAM_COMP: /* Compressed Normal Data with deleted address mark */
/*          DBG_PRINT(("Compressed Data" NLP)); */
            memset(buf, fgetc(myDisk->file), myDisk->track[Cyl][Head].sectsize);
            *readlen = myDisk->track[Cyl][Head].sectsize;
            *flags |= IMD_DISK_IO_COMPRESSED;
            break;
        default:
            printf("ERROR: unrecognized sector record type %d" NLP, sectRecordType);
            break;
    }

    /* Set flags for deleted address mark. */
    switch(sectRecordType) {
        case SECT_RECORD_NORM_DAM:      /* Normal Data with deleted address mark */
        case SECT_RECORD_NORM_DAM_ERR:  /* Normal Data with deleted address mark with read error */
        case SECT_RECORD_NORM_DAM_COMP: /* Compressed Normal Data with deleted address mark */
        case SECT_RECORD_NORM_DAM_COMP_ERR: /* Compressed Normal Data with deleted address mark */
            *flags |= IMD_DISK_IO_DELETED_ADDR_MARK;
        default:
            break;
    }

    return(SCPE_OK);
}

/* Write a sector to an IMD image. */
t_stat sectWrite(DISK_INFO *myDisk,
              uint32 Cyl,
              uint32 Head,
              uint32 Sector,
              uint8 *buf,
              uint32 buflen,
              uint32 *flags,
              uint32 *writelen)
{
    uint32 sectorFileOffset;
    uint8 sectRecordType;
    uint8 start_sect;
    *writelen = 0;

    DBG_PRINT(("Writing C:%d/H:%d/S:%d, len=%d" NLP, Cyl, Head, Sector, buflen));

    /* Check parameters */
    if(myDisk == NULL) {
        *flags = IMD_DISK_IO_ERROR_GENERAL;
        return(SCPE_IOERR);
    }

    if(sectSeek(myDisk, Cyl, Head) != 0) {
        *flags = IMD_DISK_IO_ERROR_GENERAL;
        return(SCPE_IOERR);
    }

    if(Sector > myDisk->track[Cyl][Head].nsects) {
        DBG_PRINT(("%s: invalid sector" NLP, __FUNCTION__));
        *flags = IMD_DISK_IO_ERROR_GENERAL;
        return(SCPE_IOERR);
    }

    if(myDisk->flags & FD_FLAG_WRITELOCK) {
        printf("Disk write-protected because the image contains compressed sectors. Use IMDU to uncompress." NLP);
        *flags = IMD_DISK_IO_ERROR_WPROT;
        return(SCPE_IOERR);
    }

    if(buflen < myDisk->track[Cyl][Head].sectsize) {
        printf("%s: user buffer too short [buflen %i < sectsize %i]" NLP,
            __FUNCTION__, buflen, myDisk->track[Cyl][Head].sectsize);
        *flags = IMD_DISK_IO_ERROR_GENERAL;
        return(SCPE_IOERR);
    }

    start_sect = myDisk->track[Cyl][Head].start_sector;

    sectorFileOffset = myDisk->track[Cyl][Head].sectorOffsetMap[Sector-start_sect];

    sim_fseek(myDisk->file, sectorFileOffset-1, 0);

    if (*flags & IMD_DISK_IO_ERROR_GENERAL) {
        sectRecordType = SECT_RECORD_UNAVAILABLE;
    } else if (*flags & IMD_DISK_IO_ERROR_CRC) {
        if (*flags & IMD_DISK_IO_DELETED_ADDR_MARK)
            sectRecordType = SECT_RECORD_NORM_DAM_ERR;
        else
            sectRecordType = SECT_RECORD_NORM_ERR;
    } else {
        if (*flags & IMD_DISK_IO_DELETED_ADDR_MARK)
            sectRecordType = SECT_RECORD_NORM_DAM;
        else
        sectRecordType = SECT_RECORD_NORM;
    }

    fputc(sectRecordType, myDisk->file);
    sim_fwrite(buf, 1, myDisk->track[Cyl][Head].sectsize, myDisk->file);
    *writelen = myDisk->track[Cyl][Head].sectsize;

    return(SCPE_OK);
}

/* Format an entire track.  The new track to be formatted must be after any existing tracks on
 * the disk.
 *
 * This routine should be enhanced to re-format an existing track to the same format (this
 * does not involve changing the disk image size.)
 *
 * Any existing data on the disk image will be destroyed when Track 0, Head 0 is formatted.
 * At that time, the IMD file is truncated.  So for the trackWrite to be used to sucessfully
 * format a disk image, then format program must format tracks starting with Cyl 0, Head 0,
 * and proceed sequentially through all tracks/heads on the disk.
 *
 * Format programs that are known to work include:
 * Cromemco CDOS "INIT.COM"
 * ADC Super-Six (CP/M-80) "FMT8.COM"
 * 86-DOS "INIT.COM"
 *
 */
t_stat trackWrite(DISK_INFO *myDisk,
               uint32 Cyl,
               uint32 Head,
               uint32 numSectors,
               uint32 sectorLen,
               uint8 *sectorMap,
               uint8 mode,
               uint8 fillbyte,
               uint32 *flags)
{
    FILE *fileref;
    IMD_HEADER track_header;
    uint8 *sectorData;
    unsigned long i;
    unsigned long dataLen;

    *flags = 0;

    /* Check parameters */
    if(myDisk == NULL) {
        *flags |= IMD_DISK_IO_ERROR_GENERAL;
        return(SCPE_IOERR);
    }

    if(myDisk->flags & FD_FLAG_WRITELOCK) {
        printf("Disk write-protected, cannot format tracks." NLP);
        *flags |= IMD_DISK_IO_ERROR_WPROT;
        return(SCPE_IOERR);
    }

    fileref = myDisk->file;

    DBG_PRINT(("Formatting C:%d/H:%d/N:%d, len=%d, Fill=0x%02x" NLP, Cyl, Head, numSectors, sectorLen, fillbyte));

    /* Truncate the IMD file when formatting Cyl 0, Head 0 */
    if((Cyl == 0) && (Head == 0))
    {
        /* Skip over IMD comment field. */
        commentParse(myDisk, NULL, 0);

        /* Truncate the IMD file after the comment field. */
#ifdef _WIN32 /* This might work under UNIX and/or VMS since this POSIX, but I haven't tried it. */
        _chsize(_fileno(fileref), ftell (fileref));
#else
        if (ftruncate(fileno(fileref), ftell (fileref)) == -1) {
            printf("Disk truncation failed." NLP);
            *flags |= IMD_DISK_IO_ERROR_GENERAL;
            return(SCPE_IOERR);
        }
#endif
        /* Flush and re-parse the IMD file. */
        fflush(fileref);
        diskParse(myDisk, 0);
    }

    /* Check to make sure the Cyl / Head is not already formatted. */
    if(sectSeek(myDisk, Cyl, Head) == 0) {
        printf("SIM_IMD: ERROR: Not Formatting C:%d/H:%d, track already exists." NLP, Cyl, Head);
        *flags |= IMD_DISK_IO_ERROR_GENERAL;
        return(SCPE_IOERR);
    }

    track_header.mode = mode;
    track_header.cyl = Cyl;
    track_header.head = Head;
    track_header.nsects = numSectors;
    track_header.sectsize = sectorLen;

    /* Forward to end of the file, write track header and sector map. */
    sim_fseek(myDisk->file, 0, SEEK_END);
    sim_fwrite(&track_header, 1, sizeof(IMD_HEADER), fileref);
    sim_fwrite(sectorMap, 1, numSectors, fileref);

    /* Compute data length, and fill a sector buffer with the
     * sector record type as the first byte, and fill the sector
     * data with the fillbyte.
     */
    dataLen = (128 << sectorLen)+1;
    sectorData = malloc(dataLen);
    memset(sectorData, fillbyte, dataLen);
    sectorData[0] = SECT_RECORD_NORM;

    /* For each sector on the track, write the record type and sector data. */
    for(i=0;i<numSectors;i++) {
        sim_fwrite(sectorData, 1, dataLen, fileref);
    }

    /* Flush the file, and free the sector buffer. */
    fflush(fileref);
    free(sectorData);

    /* Now that the disk track/sector layout has been modified, re-parse the disk image. */
    diskParse(myDisk, 0);

    return(SCPE_OK);
}

/* Utility function to set the image type for a unit to the correct value.
 * Prints an error message in case a CPT image is presented and returns
 * SCPE_OPENERR in this case. Otherwise the return value is SCPE_OK
 */
t_stat assignDiskType(UNIT *uptr) {
    t_stat result = SCPE_OK;
    char header[4];
    if (fgets(header, 4, uptr->fileref) == NULL)
        uptr->u3 = IMAGE_TYPE_DSK;
    else if (strncmp(header, "IMD", 3) == 0)
        uptr->u3 = IMAGE_TYPE_IMD;
    else if(strncmp(header, "CPT", 3) == 0) {
        printf("CPT images not yet supported.\n");
        uptr->u3 = IMAGE_TYPE_CPT;
        result = SCPE_OPENERR;
    }
    else
        uptr->u3 = IMAGE_TYPE_DSK;
    return result;
}

