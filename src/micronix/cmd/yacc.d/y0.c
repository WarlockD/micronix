#include "dextern"
#include "files"

int tbitset;					/* size of lookahed sets */
int nolook;						/* flag to suppress lookahead computations */

/* this file contains the definitions for most externally known data */

int nstate;						/* number of states */
struct item *pstate[_nstates + 2];	/* pointers to the descriptions of the states */
int tystate[_nstates];			/* contains type information about the states */
int stsize = _nstates;			/* maximum number of states, at present */
int memsiz = _memsize;			/* maximum size for productions and states */
int mem0[_memsize];				/* production storage */
int *mem = mem0;
int amem[_actsize];				/* action table storage */
int actsiz = _actsize;			/* action table size */
int *memp = { amem };			/* next free action table position */

int nprod = 1;					/* number of productions */
int *prdptr[_nprod];			/* pointers to descriptions of productions */
int prdlim = _nprod;			/* the maximum number of productions */
	/* levprd - productions levels to break conflicts */
int levprd[_nprod] = { 0, 0 };

int nterms;						/* number of terminals */
int tlim = _nterms;				/* the maximum number of terminals */
/*	the ascii representations of the terminals	*/
int extval;						/* start of output values */
struct sxxx1 trmset[_nterms];
char tokname[_namesize];
char cnames[_cnamsz];
int cnamsz = _cnamsz;
char *cnamp;
int temp1[_tempsize];			/* temporary storage, indexed by terms + nterms or states */
int trmlev[_nterms];			/* vector with the precedence of the terminals */
	/* The levels are the same as for levprd, but bit 04 is always 0 */
/* the ascii representations of the nonterminals */
struct sxxx2 nontrst[_nnonterm];
int ntlim = _nnonterm;			/* limit to the number of nonterminals */
int indgo[_nstates];			/* index to the stored goto table */
int defact[_nstates];
int ***pres;					/* vector of pointers to the productions yielding each nonterminal */
struct looksets **pfirst;		/* vector of pointers to first sets for each nonterminal */
int *pempty;					/* table of nonterminals nontrivially deriving e */
int nnonter = -1;				/* the number of nonterminals */
int lastred;					/* the number of the last reduction of a state */

FILE *finput;					/* yacc input file */
FILE *faction;					/* file for saving actions */
FILE *fdefine;					/* file for # defines */
FILE *ftable;					/* y.tab.c file */
FILE *ftemp;					/* tempfile to pass 2 */
FILE *foutput;					/* y.output file */

struct wset *zzcwp = { wsets };

int zzgoent;
int zzgobest;
int zzacent;
int zzexcp;
int zzclose;
int zzsrconf;
int zzmemsz;
int zzrrconf;
int lineno = 1;					/* current input line number */
int tstates[_nterms];			/* states generated by terminal gotos */
int ntstates[_nnonterm];		/* states generated by nonterminal gotos */
int mstates[_nstates];			/* chain of overflows of term/nonterm generation lists  */

struct looksets clset;
struct looksets lkst[_lsetsize];
int nlset;						/* next lookahead set index */
int lsetsz = _lsetsize;			/* number of lookahead sets */

struct wset wsets[_wsetsize];
struct wset *cwp;
int wssize = _wsetsize;

int numbval;					/* the value of an input number */
char lflag = C;					/* language flag, default is C */

int ndefout = 3;				/* number of defined symbols output */
int nerrors;					/* number of errors */
int fatfl = 1;					/* if on, error is fatal */

char *ofiles[] = {				/* output filenames for the parsers */
	FILEC,
	FILER,
	FILEE
};

int oflags[] = {
	'c',
	'r',
	'e'
};

char *excfb[] = {				/* beginning of exception function */
	"int yyexca[] {\n",
	"integer function yyexcp( s, c )\ninteger s, c\n",
	"integer procedure yyexcp(yystat, yychar)\n\n\
integer yystat, yychar\n\n\tswitch( yystat ) {\n\n",
};

char *excfsb[] = {				/* entry upon beginning a state */
	"-1, %d,\n",
	"if( s == %d ){\n",
	"\t\tcase %d:\n",
};

char *excfc[] = {				/* when a state and a character have been found */
	"\t%d, %d,\n",
	"if( c== %d ) yyexcp= %d\n",
	"\t\t\tif( yychar == %d ) return( %d )\n",
};

char *excfse[] = {
	"\t-2, %d,\n",
	"else yyexcp = %d\nreturn\n}\n",
	"\t\t\treturn( %d )\n",
};

char *excfe[] = {
	"\t};\n",
	"return\nend\n",
	"\n\t}\n\n\tend\n\n",
};

char *parser[] = {
	CPARSER,
	RPARSER,
	EPARSER,
};

int (*warray[])() = {			/* pointers to functions which write arrays */
	carray,
	rarray,
	earray,
};

char *ndefs[] = {				/* output defines of numbers */
	"# define %s %d\n",
	"define %s %d\n",
	"define %s = %d\n",
};

char *dollar[] = {				/* what does $n turn into... */
	"yypvt[-%d]",
	"yyvalv(yypvt-%d)",
	"yyvalv(yypvt-%d)",
};

char *acts[] = {
	"\ncase %d:",
	"\n%d ",
	"\n\tcase %2d:\n\t\t",
};

char *acte[] = {
	" break;",
	" goto 999",
	"\n"
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
