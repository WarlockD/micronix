/*
 * Copyright (c) 1985 by Morris Code Works
 *
 * cmd/Make/makeinit.c
 * Changed: <>
 */
#include	<stdio.h>
#include	"make.h"

char verbose INIT;
extern char execute;

extern struct target *targets;
extern struct macro *macros;
char *makefile INIT;

/*
 * initialize structures 
 */
init(argc, argv)
    int argc;
    char *argv[];
{
    int i;                      /* parameter index */

    /*
     * scan thru all supplied parameters 
     */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            /*
             * decode this option 
             */
            switch (toupper(argv[i][1])) {
            case 'V':
                verbose++;
                break;

            case 'F':
                if (++i < argc) {
                    makefile = argv[i];
                } else
                    usage();
                break;

            case 'N':
                execute = 0;
                break;

            default:
                usage();
                break;
            }
        } else
            /*
             * add this file to do list 
             */
            add_to(argv[i]);
    }

    if (makefile) {
        readmakefile(makefile, 1);
    } else {
        if (!readmakefile("makefile", 0))
            readmakefile("Makefile", 1);
    }
    if (verbose) {
        dumpdefs();
    }
}

/*
 * display all definitions 
 */
dumpdefs()
{
    struct macro *m;
    struct target *t;
    struct dep *d;
    struct command *cmd;
    char *s;
    int i, j;

    /*
     * first do the macro definitions 
     */
    fprintf(stderr, "\nMacros:\n");
    for (m = macros; m; m = m->next) {
        fprintf(stderr, "%s = ", m->name);
        s = m->text;
        for (s = m->text; *s ; s += j) {
            for (j = 0, i = 0;; i++) {
                if (s[i] == '\0') {
                    j = i; 
                    break;
                }
                if (s[i] == ' ') {
                    j = i;
                }
                if (i > 60) break;
            }
            i = s[j];
            s[j] = '\0';
            fprintf(stderr, "%s%s\n", 
                (s != m->text) ? "\t" : "", s);
            s[j] = i;
        }
        /* hexdump(m->text, strlen(m->text)); */
    }

    /*
     * now do the target definitions 
     */
    for (t = targets; t; t = t->next) {
        /*
         * tell which file we're talking about 
         */
        fprintf(stderr, "\nFile(%s): Modified(%s)\n  depends on:",
            t->name, PTime(t->modified));

        /*
         * display the dependencies 
         */
        i = 0;
        for (d = t->need; d; d = d->next) {
            fprintf(stderr, "%c%s", 
                (i == 0 ? '\t' : ' '), d->name);
            if ((i += strlen(d->name) + 1) > 44) {
                if (d->next)
                    fprintf(stderr, "\n\t");
                i = 0;
            }
        }

        /*
         * display the associated commands 
         */
        fprintf(stderr, "\n  commands:\n");
        for (cmd = t->recipe; cmd; cmd = cmd->next)
            fprintf(stderr, "\t%.70s\n", cmd->text);
    }
}

/*
 * explain how to use make 
 */
usage()
{
    fprintf(stderr,
        "usage: make [-v] [-n] [-f makefile] [target ...]\n");
    exit(1);
}

/*
 * add_to : add name to do_list 
 */
add_to(s)
    char *s;
{
    struct work *w;             /* ptr to this 'do' record */
    extern struct work *dolist; /* list of names to 'make' */
    char *lc();                 /* convert to lowercase */

    /*
     * get a pointer to newly allocated structure 
     */
    if (!(w = (struct work *) calloc(1, sizeof(struct work))))
        OutOfMem();

    w->name = lc(s);

    w->next = dolist;
    dolist = w; 
}

char *
lc(s)                           /* convert string to lower case */
    char *s;
{
    char *str;

    for (str = s; *str = tolower(*str); ++str);

    return (s);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
