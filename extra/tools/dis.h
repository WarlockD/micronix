/*
 * verbosity
 */
#define	V_BUF	0x1
#define	V_MATCH	0x2
#define	V_PARSE	0x4
#define	V_OUT	0x8
#define	V_TABLE	0x10
#define	V_SYM	0x20
#define	V_REF	0x40
#define	V_EXTRA	0x80

int bigendian;

/*
 * a format is a handle for special output forms
 */
struct format {
	char *name;
	char *prefix;
	char *suffix;
	int width;
	int flags;
#define	F_SIGNED	0x01
#define	F_HEX		0x02
#define	F_XREF		0x04
#define	F_PCREL		0x08
};
struct format *formats;
int n_formats;

/*
 * a field is a parcel in an instruction that may
 * have bound to it an array and/or a format
 */
struct field {
	char *name;
	unsigned char width;
	unsigned char bitoff;	/* least significant bit == 0 */
	unsigned char destbit;
	struct array *array;
	struct format *fmt;
	unsigned int value;
};

struct field *fields;
int n_fields;

struct operand {
	struct field *fld;
	struct operand *next;
};

#define	NUM_OPS		3
#define	NUM_VALS	3

/*
 * an iclass is a encoding form that describes an instruction
 * with variable fields
 */
struct iclass {
	unsigned long mask;
	unsigned long val;
	unsigned char ilen;

	struct operand *operand[NUM_OPS];

	char *opcode;

	struct field *opi;	  /* op name array index */
	struct array *opa;
};
struct iclass *instructions;
int n_instructions;

/* arrays of strings */
struct array {
	char *name;
	char **values;
	int count;
};
struct array *arrays;
int n_arrays;

struct array *lookup_array(char *name, int required);
struct field *lookup_field(char *name, int required);
struct format *lookup_format(char *name, int required);

extern unsigned long extract(struct field *fp, unsigned char *pc);
extern unsigned short bias(unsigned short base, unsigned short off);
extern unsigned long getlong(unsigned char *p);
extern int load_arch(char *arch);
void disas(unsigned char *destbuf, int startaddr, int size);
int decode(char *src, int size, unsigned char **dest, unsigned long *address);
extern int verbose;

/*
 * an instance of a cracked instruction
 */
struct inst {
	int addr;
	int len;

	char *op;	   /* the operation */

	struct iclass *ic;

	int vals;
	int opvals[NUM_OPS];	/* base index of op values */
	int values[NUM_VALS];
};

#define swap(a,b) { int tmp; tmp = (a); (a) = (b) ; (b) = tmp; }
#define bit_set(map, bit) ((map[(bit) / 8] & (1 << ((bit) % 8))) ? 1 : 0)
#define set_bit(map, bit) map[(bit) / 8] |= (1 << ((bit) % 8))

typedef unsigned char seg_t;		/* segment type */
typedef unsigned short addr_t;		/* offset into the output buffer */
typedef unsigned short segoff_t;	/* offset into the segment */

/*
 * a symbol is a name that may be defined or not.
 * these may be from the object file, or they may be invented labels
 * either way, symbols are identified by the offset, seg tuple
 */
struct symbol {
	struct seg *segp;
	segoff_t offset;
	char *name;
	char invented;
	struct symbol *next;
};

/* return the symbol that has this location */
char *getsymname(addr_t offset);

/*
 * a reference is an operand field that is an address
 * they always have names, either externally known symbols or invented labels
 * they may optionally have a bias added to them
 */
struct ref {
    addr_t offset;			/* where the reference is */
    struct symbol *sym;		/* what it references */
    unsigned short bias;	/* what might be added */
    struct ref *next;		/* */
};

struct ref *getref(addr_t val);	/* the reference to print here */

int is_code(addr_t addr);

/*
 * a segment is a notional hunk of memory
 * that is a portion of the output buffer
 * so, for a multi-module image, we will have many code and data
 * segments, each with a non-overlapping region of the output buffer
 * that goes from base for len bytes
 */
struct seg {
        int module;
		char *modname;
        addr_t base;
        int len;
        seg_t type;
#define S_CODE  1
#define S_DATA  2
#define S_COM   3
#define	S_UNDEF	4	/* a hack for undefined symbols */
        struct seg *next;
};
