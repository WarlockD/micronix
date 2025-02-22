/*
 * kernel tty handler data structures
 *
 * include/sys/tty.h
 * Changed: <2021-12-23 14:36:14 curt>
 */

/*
 * These parameters control the size of the tty structure input and output
 * queues.
 */
#define CSIZE	900
#define TTYHOG	200             /* Queue size in bytes (not more that 255) */
#define LOWATER	 16             /* Queue low water mark */
#define HIWATER (TTYHOG - 16)   /* Queue high water mark */

/*
 * Special characters
 */
#define BACKGRND 02             /* cntrl-b background signal */
#define EOT	004             /* End Of Transmission (cntrl D) */
#define XON	021             /* sent to input devices to restart input */
#define XOFF	023             /* sent by printers to hold up output */
#define STOP	033             /* Escape */
#define QUIT	034             /* Quit (cntrl \) */
#define BSLASH	0134            /* Back slash \ */
#define RUB	0177            /* Rubout or Delete */
#define HIGHBIT 0200            /* non-ascii */

/*
 * c - list element 2 bytes of pointer, 14 bytes of data
 */
struct cblock {
    struct cblock *next;
    char block[14];
};

/*
 * Terminal circular queue
 */
struct que {
    UINT8 count;
    char *first;
    char *last;
};

/*
 * Terminal control structure. The first 6 bytes are used by stty/gtty. The
 * dev and state members are used in mio.s.
 */
struct tty {
    UINT8 ispeed;               /* input baud rate */
    UINT8 ospeed;               /* output baud rate */
    UINT8 erase;                /* erase character */
    UINT8 kill;                 /* kill character */
    UINT mode;                  /* see below */
    UINT8 state;                /* see below */
    UINT8 col;                  /* printhead waiting */
    UINT dev;                   /* device number, tty + 8 */
    UINT8 line;                 /* first line after a return is 0 */
    int (*start) ();            /* enable output interrupt */
    int (*stop) ();             /* disable output interrupt */
    int (*put) ();              /* print a character */
    int (*set) ();              /* set new baud rate */
    UINT8 nextc;                /* for tab and newline expansion */
    UINT8 nbreak;
    struct que rawque;          /* raw input queue */
    struct que cokque;          /* cooked input queue */
    struct que outque;          /* output queue */
    UINT8 count;                /* see below */
    UINT8 mstate;               /* modem related state */
};

/*
 * Mode bits
 */
#define SHAKE	0100000         /* Use RS-232 clear-to-send line */
#define ALL8	0040000          /* Keep all 8 bits of input data */
#define CBREAK	0020000          /* raw with rub, quit, start, stop */
#define MORE	0010000          /* pause after 23 lines of output */
#define RAW	    0000040             /* Raw input mode */
#define MAPCR	0000020             /* cr->lf mapping */
#define ECHO	0000010             /* Echo input */
#define OLDTTY	0000004             /* Map uppercase->lower, etc. */
#define XTABS	0000002             /* Expand tabs via spaces */

/*
 * State bits
 */
#define LOSTOP	001             /* low level output stopped (esc) */
#define HOSLEEP 002             /* high level output sleeping */
#define HISLEEP 004             /* high level input sleeping */
#define STOPIN	010             /* send an XOFF at next opportunity */
#define INSTOP	020             /* an XOFF has been sent */
#define STARTIN 040             /* send an XON at next opportunity */
#define OPEN	0100            /* tty is open. See _mstop in mio.s */
#define ERROR	0200            /* input error - wrong baud rate */
#define WOPEN	0400            /* waiting for open */

/*
 * Modem control related state bits
 */
#define WOPEN	001             /* waiting for CARRIER DETECT */
#define CD      002             /* CARRIER DETECT is on */
#define DTR     004             /* assert DATA TERMINAL READY */
#define DIALER	040             /* don't wait for carrier */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
