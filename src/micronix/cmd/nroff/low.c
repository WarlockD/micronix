static char *sccsid = "@(#) low.c v1.1p3 90/07/24 rosenkra\0            ";

/*
 *	low.c - misc low-level functions for nroff word processor
 *
 *	adapted for atariST/TOS by Bill Rosenkranz 11/89
 *	net:	rosenkra@convex.com
 *	CIS:	71460,17
 *	GENIE:	W.ROSENKRANZ
 *
 *	original author:
 *
 *	Stephen L. Browning
 *	5723 North Parker Avenue
 *	Indianapolis, Indiana 46220
 *
 *	history:
 *
 *	- Originally written in BDS C;
 *	- Adapted for standard C by W. N. Paul
 *	- Heavily hacked up to conform to "real" nroff by Bill Rosenkranz
 */

#undef NRO_MAIN					/* extern globals */

#include <stdio.h>
#include "nroff.h"



/*------------------------------*/
/*	atod			*/
/*------------------------------*/
atod (c)
char    c;
{

/*
 *	convert ascii character to decimal.
 */

	return (((c < '0') || (c > '9')) ? -1 : c - '0');
}





/*------------------------------*/
/*	robrk			*/
/*------------------------------*/
robrk ()
{

/*
 *	end current filled line
 */

	if (co.outp > 0)
	{
		/*
		 *   handle margin char (change bar) here for all filled lines
		 */
		co.outbuf[co.outp]   = '\r';
		co.outbuf[co.outp+1] = '\n';
		co.outbuf[co.outp+2] = EOS;

		do_mc (co.outbuf);

		put (co.outbuf);
	}
	co.outp   = 0;
	co.outw   = 0;
	co.outwds = 0;
	co.outesc = 0;
}




/*------------------------------*/
/*	ctod			*/
/*------------------------------*/
ctod (p)
register char  *p;
{

/*
 *	convert string to decimal. processes only positive values.
 *	this takes a constant like "1", "1.0i", etc. 
 */

	register long	val;
	register int	d;
	register char  *pp = p;
	register char  *ptmp;
	int		rside = 0;
	int		lside = 0;
	int		has_rside = 0;
	int		has_lside = 0;

	if (*p == EOS)
		return (0);

	ptmp = skipwd (pp);
	pp = --ptmp;
	
	switch (*pp)
	{
	case 'i':
	case 'c':
		val = 0L;
		while (*p != EOS && isdigit (*p))
		{
			has_lside++;
			lside = atod (*p);
			p++;
			if (lside == -1)
				break;
			val = 10L * val + (long) lside;
		}
		lside = (int) val;
		if (*p == '.')
		{
			p++;
			val = 0L;
			while (*p != EOS && isdigit (*p))
			{
				has_rside++;
				rside = atod (*p);
				p++;
				if (rside == -1)
					break;
				val = 10L * val + (long) rside;
				if (has_rside > 2)	/* more than enough */
					break;
			}
			rside = (int) val;
		}

		/*
		 *   now put it together. 1.0i -> 240, 1.50i -> 360, etc.
		 */
		val = 0L;
		if (has_lside)
		{
			val = (long) lside * BU_INCH;
		}
		switch (has_rside)
		{
		case 1:
			val = val + ((long) rside * BU_INCH / 10L);
			break;
		case 2:
			val = val + ((long) rside * BU_INCH / 100L);
			break;
		case 3:
			val = val + ((long) rside * BU_INCH / 1000L);
			break;
		default:
			break;
		}
		if (*pp == 'c')
			val = (val * BU_CM) / BU_INCH;

		/*
		 *   for now we convert to basic char size, 1 em...
		 */
		val = val / BU_EM;

		break;

	case 'P':
	case 'm':
	case 'n':
	case 'p':
	case 'u':
	case 'v':
		val = 0L;
		while (*p != EOS)
		{
			d = atod (*p);
			p++;
			if (d == -1)
				break;
			val = 10L * val + (long) d;
		}
		switch (*pp)
		{
		case 'P':
			val = val * BU_PICA;
			break;
		case 'p':
			val = val * BU_POINT;
			break;
		case 'u':
			val = val * BU_BU;
			break;
		case 'm':
			val = val * BU_EM;
			break;
		case 'n':
			val = val * BU_EN;
			break;
		case 'v':
			val = val * BU_EM;
			break;
		}

		/*
		 *   for now we convert to basic char size, 1 em...
		 */
		val = val / BU_EM;

		break;

	default:
		/*
		 *   this is the default behavior. it SHOULD make things
		 *   compatible with the old way...
		 */
		val = 0L;
		while (*p != EOS)
		{
			d = atod (*p);
			p++;
			if (d == -1)
				break;
			val = 10L * val + (long) d;
		}
		break;
	}

	return ((int) val);
}




/*------------------------------*/
/*	inptobu			*/
/*------------------------------*/
inptobu (ps)
char   *ps;
{

/*
 *	convert input units to b.u.
 */

	return;
}




/*------------------------------*/
/*	butochar		*/
/*------------------------------*/
butochar (ps)
char   *ps;
{

/*
 *	convert b.u. to char spaces
 */

	return;
}




/*------------------------------*/
/*	skipbl			*/
/*------------------------------*/
char   *skipbl (p)
register char  *p;
{

/*
 *	skip blanks and tabs in character buffer. return ptr to first
 *	non-space or non-tab char. this could mean EOS or \r or \n.
 *	also increments the arg ptr (side effect).
 */

	while ((*p != EOS) && (*p == ' ' || *p == '\t'))
		++p;
	return (p);
}




/*------------------------------*/
/*	skipwd			*/
/*------------------------------*/
char   *skipwd (p)
register char  *p;
{

/*
 *	skip over word and punctuation. anything but space,\t,\r,\n, and EOS
 *	is skipped. return ptr to the first of these found. also increments
 *	the arg ptr (side effect).
 */

	while (*p != EOS && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
		++p;
	return (p);
}





/*------------------------------*/
/*	space			*/
/*------------------------------*/
space (n)
int     n;
{

/*
 *	space vertically n lines. this does header and footer also.
 */

	robrk ();
	if (pg.lineno > pg.bottom)
		return;
	if (pg.lineno == 0)
		phead ();
	skip (min (n, pg.bottom + 1 - pg.lineno));
	pg.lineno += n;
	set_ireg ("ln", pg.lineno, 0);
	if (pg.lineno > pg.bottom)
		pfoot ();
}




/*------------------------------*/
/*	getfield		*/
/*------------------------------*/
char   *getfield (p, q, delim)
register char  *p;
register char  *q;
char		delim;
{

/*
 *	get field from title
 */

	while (*p != delim && *p != '\r' && *p != '\n' && *p != EOS)
	{
		*q++ = *p++;
	}
	*q = EOS;
	if (*p == delim)
		++p;
	return (p);
}





/*------------------------------*/
/*	getwrd			*/
/*------------------------------*/
getwrd (p0, p1)
register char  *p0;
register char  *p1;
{

/*
 *	get non-blank word from p0 into p1.
 *	return number of characters processed.
 */

	register int	i;
	register char  *p;
	char		c;

	/*
	 *   init counter...
	 */
	i = 0;


	/*
	 *   skip leading whitespace
	 */
	while (*p0 && (*p0 == ' ' || *p0 == '\t'))
	{
		++i;
		++p0;
	}


	/*
	 *   set ptr and start to look for end of word
	 */
	p = p0;
	while (*p0 != ' ' && *p0 != EOS && *p0 != '\t')
	{
		if (*p0 == '\n' || *p0 == '\r')
			break;
		*p1 = *p0++;
		++p1;
		++i;
	}

	c = *(p1 - 1);
	if (c == '"')
		c = *(p1 - 2);
	if (c == '?' || c == '!')
	{
		*p1++ = ' ';
		++i;
	}
	if (c == '.' && (*p0 == '\n' || *p0 == '\r' || islower (*p)))
	{
		*p1++ = ' ';
		++i;
	}
	*p1 = EOS;

	return (i);
}




/*------------------------------*/
/*	countesc		*/
/*------------------------------*/

#define ESC			27

countesc (p)
register char  *p;
{

/*
 *	count escape sequence characters in given null-terminated string.
 *	we try and interpret the escape sequence, and if we recognize it,
 *	we count the total number of chars in the sequence, including the
 *	ESC itself. e.g. "ESC [ 1 m" is 4. we do this for the entire string.
 *
 *	the results are used when adjusting the margins, so we ignore the
 *	additional chars added. the escape sequence should not be broken
 *	up by this process, since we only add extra space(s) next to a
 *	space.
 */

	register char  *pp;
	register int	num;


	/*
	 *  initialize string pointer and count
	 */
	pp  = p;
	num = 0;


	/*
	 *   do entire string...
	 */
	while (*pp != EOS)
	{
		/*
		 *   is current char an ESC?
		 */
		if (*pp == ESC)
		{
			/*
			 *   yes...
			 *
			 *   look at first char after the ESC and try to see
			 *   if we grok it...
			 */
			switch (*(pp+1))
			{
			/*
			 *   this is atari/vt52-specific. escapes are 2, 3,
			 *   or 4 chars long. others we do not grok.
			 *   generally only p, q, b, and c will show up.
			 *   if we grok, advance pp ptr (see note below)
			 */
			case 'A':			/* vt52: ESC x */
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'H':
			case 'I':
			case 'J':
			case 'K':
			case 'L':
			case 'M':
			case 'd':
			case 'e':
			case 'f':
			case 'j':
			case 'k':
			case 'l':
			case 'o':
			case 'p':
			case 'q':
			case 'v':
			case 'w':
				num += 2;
/*!!!*/				pp  += 1;
				break;
			case 'b':			/* vt52: ESC x x */
			case 'c':
				num += 3;
/*!!!*/				pp  += 1;
				break;
			case 'Y':			/* vt52: ESC x x x */
				num += 4;
/*!!!*/				pp  += 3;
				break;


			/*
			 *   the only escapes like this that nroff itself
			 *   generates are these. others we can only guess
			 *   at, and we guess ESC [ x x.
			 *
			 *   note: we advance pp to the end of the escape
			 *   to speed up the process. pp points to the ESC
			 *   initially. pp increment at the bottom of the
			 *   while will put us on the first char after the
			 *   escape...
			 */
			case '[':			/* ansi: ESC [ ... */
				if (*(pp+2) == 'm')	/*       ESC [ m   */
				{
					num += 3;
/*!!!*/					pp  += 2;
				}
				else			/*       ESC [ n m */
				{
					num += 4;
/*!!!*/					pp  += 3;
				}
				break;


			/*
			 *   it IS an escape, but we do not know it. so count
			 *   the escape itself, and treat what follows like
			 *   text
			 */
			default:			/* unknown */
				num += 1;
				break;
			}
		}


		/*
		 *   next char in string...
		 */
		pp++;
	}


	/*
	 *   give caller the count
	 */
	return (num);
}




/*------------------------------*/
/*	itoda			*/
/*------------------------------*/
itoda (value, p, size)
int		value;
register char  *p;
register int	size;
{

/*
 *	convert integer to decimal ascii string
 */

	register int	i;
	register int    j;
	register int	k;
	register int	aval;
	char		c[20];

	aval = abs (value);
	c[0] = EOS;
	i = 1;
	do
	{
		c[i++] = (aval % 10) + '0';
		aval /= 10;

	} while (aval > 0 && i <= size);

	if (value < 0 && i <= size)
		c[i++] = '-';
	for (j = 0; j < i; ++j)
		*p++ = c[i - j - 1];

	return (i);
}




/*------------------------------*/
/*	itoROMAN		*/
/*------------------------------*/
itoROMAN (value, p, size)
int		value;
register char  *p;
register int	size;
{

/*
 *	convert integer to upper roman. must be positive
 */

	register int	i;
	register int	j;
	register int	k;
	register int	aval;
	char		c[100];
	int		rem;

	aval = abs (value);
	c[0] = EOS;
	i = 1;

	/*
	 *   trivial case:
	 */
	if (aval == 0)
	{
		c[i++] = '0';
		goto done_100;
	}

	/*
	 *   temporarily mod 100...
	 */
	aval = aval % 100;

	if (aval > 0)
	{
		/*
		 *   build backward
		 *
		 *   | I|		1
		 *   | II|		2
		 *   | III|		3
		 *   | VI|		4
		 *   | V|		5
		 *   | IV|		6
		 *   | IIV|		7
		 *   | IIIV|		8
		 *   | XI|		9
		 *   | X|		0
		 *   | IX|		11
		 *   | IIX|		12
		 */
		if ((aval % 5 == 0) && (aval % 10 != 0))/* 5 */
			c[i++] = 'V';
		else
		{
			rem = aval % 10;
			if (rem == 9)			/* 9 */
			{
				c[i++] = 'X';
				c[i++] = 'I';
			}
			else if (rem == 8)		/* 8 */
			{
				c[i++] = 'I';
				c[i++] = 'I';
				c[i++] = 'I';
				c[i++] = 'V';
			}
			else if (rem == 7)		/* 7 */
			{
				c[i++] = 'I';
				c[i++] = 'I';
				c[i++] = 'V';
			}
			else if (rem == 6)		/* 6 */
			{
				c[i++] = 'I';
				c[i++] = 'V';
			}
			else if (rem == 4)		/* 4 */
			{
				c[i++] = 'V';
				c[i++] = 'I';
			}
			else				/* 3,2,1 */
			{
				for (j = 0; j < rem; j++)
					c[i++] = 'I';
			}
		}

		aval /= 10;
		if (aval == 0)
			goto done_100;

		rem = aval % 10;
		if (rem == 4)
		{
			c[i++] = 'L';
			c[i++] = 'X';
		}
		else if (rem == 5)
		{
			c[i++] = 'L';
		}
		else if (rem < 4)
		{
			for (j = 0; j < rem; j++)
				c[i++] = 'X';
		}
		else
		{
			for (j = 0; j < rem - 5; j++)
				c[i++] = 'X';
			c[i++] = 'L';
		}
	}


done_100:
	/*
	 *   divide by 100 (they are done) and temp mod by another 10
	 */
	aval  = abs (value);
	aval /= 100;

	if (aval > 0)
	{
		rem  = aval % 10;
		if (rem == 4)
		{
			c[i++] = 'D';
			c[i++] = 'C';
		}
		if (rem == 5)
		{
			c[i++] = 'D';
		}
		else if (rem < 4)
		{
			for (j = 0; j < rem; j++)
				c[i++] = 'C';
		}
		else if (rem == 9)
		{
			c[i++] = 'M';
			c[i++] = 'C';
		}
		else if (rem < 9)
		{
			for (j = 0; j < rem - 5; j++)
				c[i++] = 'C';
			c[i++] = 'D';
		}
	}


	aval /= 10;

	if (aval > 0)
	{
		rem  = aval % 10;
		if (rem < 4)
		{
			for (j = 0; j < rem; j++)
				c[i++] = 'M';
		}
	}


	if (value < 0)
		c[i++] = '-';

	for (j = 0; j < i; ++j)
		*p++ = c[i - j - 1];

	return (i);
}




/*------------------------------*/
/*	itoroman		*/
/*------------------------------*/
itoroman (value, p, size)
int     value;
char   *p;
int     size;
{

/*
 *	convert integer to lower roman
 */

	register int	i;
	register int	len;
	register int	aval;
	char		c[100];

	c[0] = EOS;
	len = itoROMAN (value, c, size);

	for (i = 0; i < len; i++)
	{
		p[i] = c[i];
		if (isalpha (p[i]))
			p[i] = tolower (c[i]);
	}

	return (len);
}




/*------------------------------*/
/*	itoLETTER		*/
/*------------------------------*/
itoLETTER (value, p, size)
int		value;
register char  *p;
register int	size;
{

/*
 *	convert integer to upper letter value: 0,A,B,C,...,AA,AB,AC,...
 */

	register int	i;
	register int	j;
	register int	k;
	register int	aval;
	int		rem;
	char		c[20];

	aval = abs (value);
	c[0] = EOS;
	i = 1;

	/*
	 *   1 based:
	 *
	 *   0	0
	 *   1	A
	 *   25	Z
	 *   26	AA
	 *   51 AZ
	 *   52 AAA
	 *   ...
	 */
	if (aval == 0)
		c[i++] = '0';
	else if (aval < 27)
	{
		c[i++] = aval - 1 + 'A';
	}
	else
	{
		do
		{
			c[i++] = ((aval - 1) % 26) + 'A';
			aval = (aval - 1)  / 26;
	
		} while (aval > 0 && i <= size);
	}

	if (value < 0 && i <= size)
		c[i++] = '-';

	for (j = 0; j < i; ++j)
		*p++ = c[i - j - 1];

	return (i);
}



/*------------------------------*/
/*	itoletter		*/
/*------------------------------*/
itoletter (value, p, size)
int		value;
register char  *p;
register int	size;
{

/*
 *	convert integer to upper letter value: 0,a,b,c,...,aa,ab,ac,...
 */

	register int	i;
	register int	j;
	register int	k;
	register int	aval;
	char		c[20];
	int		rem;

	aval = abs (value);
	c[0] = EOS;
	i = 1;

	/*
	 *   1 based:
	 *
	 *   0	0
	 *   1	A
	 *   25	Z
	 *   26	AA
	 *   51 AZ
	 *   52 AAA
	 *   ...
	 */
	if (aval == 0)
		c[i++] = '0';
	else if (aval < 27)
	{
		c[i++] = aval - 1 + 'a';
	}
	else
	{
		do
		{
			c[i++] = ((aval - 1) % 26) + 'a';
			aval = (aval - 1)  / 26;
	
		} while (aval > 0 && i <= size);
	}

	if (value < 0 && i <= size)
		c[i++] = '-';

	for (j = 0; j < i; ++j)
		*p++ = c[i - j - 1];

	return (i);
}



/*------------------------------*/
/*	min			*/
/*------------------------------*/

#ifdef min
#undef min
#endif

min (v1, v2)
register int	v1;
register int	v2;
{

/*
 *	find minimum of two integer ONLY
 */

	return ((v1 < v2) ? v1 : v2);
}





/*------------------------------*/
/*	max			*/
/*------------------------------*/

#ifdef max
#undef max
#endif

max (v1, v2)
register int	v1;
register int	v2;
{

/*
 *	find maximum of two integers ONLY
 */

	return ((v1 > v2) ? v1 : v2);
}





/*------------------------------*/
/*	err_exit		*/
/*------------------------------*/
err_exit (code)
{

/*
 *	exit cleanly on fatal error (close files, etc). also handles normal
 *	exit.
 */

	if (err_stream != stderr && err_stream != (FILE *) 0)
	{
		/*
		 *   not going to stderr (-o file)
		 */
		fflush (err_stream);
		fclose (err_stream);
	}
	if (debugging && dbg_stream != stderr && dbg_stream != (FILE *) 0)
	{
		fflush (dbg_stream);
		fclose (dbg_stream);
	}
	if (out_stream != stdout && out_stream != (FILE *) 0)
	{
		/*
		 *   not going to stdout (-l)
		 */
		fflush (out_stream);
		fclose (out_stream);
	}

	if (hold_screen)
	{
		wait_for_char ();
	}

	exit (code);
}



/*------------------------------*/
/*	wait_for_char		*/
/*------------------------------*/
#ifdef GEMDOS
#include <osbind.h>
#endif

wait_for_char ()
{
#ifdef GEMDOS
		printf ("enter any key..."); fflush (stdout);
		Cconin ();
#endif
}



