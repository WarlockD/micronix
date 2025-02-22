/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Copyright (C) 2014-2017 by Udo Munk
 *
 * This module of the simulator contains the I/O simulation
 * for a Cromemco Z-1 system
 *
 * History:
 * 15-DEC-14 first version
 * 20-DEC-14 added 4FDC emulation and machine boots CP/M 2.2
 * 28-DEC-14 second version with 16FDC, CP/M 2.2 boots
 * 01-JAN-15 fixed 16FDC, machine now also boots CDOS 2.58 from 8" and 5.25"
 * 01-JAN-15 fixed frontpanel switch settings, added boot flag to fp switch
 * 12-JAN-15 fdc and tu-art improvements, implemented banked memory
 * 02-FEB-15 modified MMU, implemented timers and interrupts
 * 20-FEB-15 bug fix for release 1.25
 * 10-MAR-15 added support for two parallel port lpt devices on TU-ART
 * 26-MAR-15 added support for two serial port tty devices on TU-ART
 * 29-APR-15 added Cromemco DAZZLER to the machine
 * 06-DEC-16 implemented status display and stepping for all machine cycles
 * 22-DEC-16 moved MMU out to the new memory interface module
 * 15-AUG-17 modified index pulse handling
 */

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include "sim.h"
#include "simglb.h"
#include "../../iodevices/unix_network.h"
#include "memory.h"

/*
 *	Forward declarations for I/O functions
 */
static BYTE io_trap_in(void);
static void io_trap_out(BYTE);
#ifdef notdef
static BYTE fp_in(void);
static void fp_out(BYTE);
#endif
static BYTE mmu_in(void);
static void mmu_out(BYTE);

/*
 *	Forward declarations for support function
 */
static void *timing(void *);
static void interrupt(int);

static int rtc;			/* flag for 512ms RTC interrupt */

/* network connections for serial ports on the TU-ART's */
struct net_connectors ncons[NUMNSOC];

/*
 *	This array contains function pointers for every
 *	input I/O port (0 - 255), to do the required I/O.
 */
static BYTE (*port_in[256]) (void) = {
	io_trap_in,	/* port 0 */
	io_trap_in,	/* port 1 */
	io_trap_in,			/* port 2 */
	io_trap_in,	/* port 3 */
	io_trap_in,		/* port 4 */
	io_trap_in,			/* port 5 */
	io_trap_in,			/* port 6 */
	io_trap_in,			/* port 7 */
	io_trap_in,			/* port 8 */
	io_trap_in,			/* port 9 */
	io_trap_in,			/* port 10 */
	io_trap_in,			/* port 11 */
	io_trap_in,			/* port 12 */
	io_trap_in,			/* port 13 */
	io_trap_in,	/* port 14 */
	io_trap_in,			/* port 15 */
	io_trap_in,			/* port 16 */
	io_trap_in,			/* port 17 */
	io_trap_in,			/* port 18 */
	io_trap_in,			/* port 19 */
	io_trap_in,			/* port 20 */
	io_trap_in,			/* port 21 */
	io_trap_in,			/* port 22 */
	io_trap_in,			/* port 23 */
	io_trap_in,			/* port 24 */
	io_trap_in,			/* port 25 */
	io_trap_in,			/* port 26 */
	io_trap_in,			/* port 27 */
	io_trap_in,			/* port 28 */
	io_trap_in,			/* port 29 */
	io_trap_in,			/* port 30 */
	io_trap_in,			/* port 31 */
	io_trap_in,	/* port 32 */
	io_trap_in,	/* port 33 */
	io_trap_in,			/* port 34 */
	io_trap_in,	/* port 35 */
	io_trap_in,	/* port 36 */
	io_trap_in,			/* port 37 */
	io_trap_in,			/* port 38 */
	io_trap_in,			/* port 39 */
	io_trap_in,			/* port 40 */
	io_trap_in,			/* port 41 */
	io_trap_in,			/* port 42 */
	io_trap_in,			/* port 43 */
	io_trap_in,			/* port 44 */
	io_trap_in,			/* port 45 */
	io_trap_in,			/* port 46 */
	io_trap_in,			/* port 47 */
	io_trap_in,		/* port 48 */
	io_trap_in,		/* port 49 */
	io_trap_in,		/* port 50 */
	io_trap_in,		/* port 51 */
	io_trap_in,	/* port 52 */
	io_trap_in,			/* port 53 */
	io_trap_in,			/* port 54 */
	io_trap_in,			/* port 55 */
	io_trap_in,			/* port 56 */
	io_trap_in,			/* port 57 */
	io_trap_in,			/* port 58 */
	io_trap_in,			/* port 59 */
	io_trap_in,			/* port 60 */
	io_trap_in,			/* port 61 */
	io_trap_in,			/* port 62 */
	io_trap_in,			/* port 63 */
	mmu_in,				/* port 64 */
	io_trap_in,			/* port 65 */
	io_trap_in,			/* port 66 */
	io_trap_in,			/* port 67 */
	io_trap_in,			/* port 68 */
	io_trap_in,			/* port 69 */
	io_trap_in,			/* port 70 */
	io_trap_in,			/* port 71 */
	io_trap_in,			/* port 72 */
	io_trap_in,			/* port 73 */
	io_trap_in,			/* port 74 */
	io_trap_in,			/* port 75 */
	io_trap_in,			/* port 76 */
	io_trap_in,			/* port 77 */
	io_trap_in,			/* port 78 */
	io_trap_in,			/* port 79 */
	io_trap_in,	/* port 80 */
	io_trap_in,	/* port 81 */
	io_trap_in,			/* port 82 */
	io_trap_in,	/* port 83 */
	io_trap_in,	/* port 84 */
	io_trap_in,			/* port 85 */
	io_trap_in,			/* port 86 */
	io_trap_in,			/* port 87 */
	io_trap_in,			/* port 88 */
	io_trap_in,			/* port 89 */
	io_trap_in,			/* port 90 */
	io_trap_in,			/* port 91 */
	io_trap_in,			/* port 92 */
	io_trap_in,			/* port 93 */
	io_trap_in,			/* port 94 */
	io_trap_in,			/* port 95 */
	io_trap_in,			/* port 96 */
	io_trap_in,			/* port 97 */
	io_trap_in,			/* port 98 */
	io_trap_in,			/* port 99 */
	io_trap_in,			/* port 100 */
	io_trap_in,			/* port 101 */
	io_trap_in,			/* port 102 */
	io_trap_in,			/* port 103 */
	io_trap_in,			/* port 104 */
	io_trap_in,			/* port 105 */
	io_trap_in,			/* port 106 */
	io_trap_in,			/* port 107 */
	io_trap_in,			/* port 108 */
	io_trap_in,			/* port 109 */
	io_trap_in,			/* port 110 */
	io_trap_in,			/* port 111 */
	io_trap_in,			/* port 112 */
	io_trap_in,			/* port 113 */
	io_trap_in,			/* port 114 */
	io_trap_in,			/* port 115 */
	io_trap_in,			/* port 116 */
	io_trap_in,			/* port 117 */
	io_trap_in,			/* port 118 */
	io_trap_in,			/* port 119 */
	io_trap_in,			/* port 120 */
	io_trap_in,			/* port 121 */
	io_trap_in,			/* port 122 */
	io_trap_in,			/* port 123 */
	io_trap_in,			/* port 124 */
	io_trap_in,			/* port 125 */
	io_trap_in,			/* port 126 */
	io_trap_in,			/* port 127 */
	io_trap_in,			/* port 128 */
	io_trap_in,			/* port 129 */
	io_trap_in,			/* port 130 */
	io_trap_in,			/* port 131 */
	io_trap_in,			/* port 132 */
	io_trap_in,			/* port 133 */
	io_trap_in,			/* port 134 */
	io_trap_in,			/* port 135 */
	io_trap_in,			/* port 136 */
	io_trap_in,			/* port 137 */
	io_trap_in,			/* port 138 */
	io_trap_in,			/* port 139 */
	io_trap_in,			/* port 140 */
	io_trap_in,			/* port 141 */
	io_trap_in,			/* port 142 */
	io_trap_in,			/* port 143 */
	io_trap_in,			/* port 144 */
	io_trap_in,			/* port 145 */
	io_trap_in,			/* port 146 */
	io_trap_in,			/* port 147 */
	io_trap_in,			/* port 148 */
	io_trap_in,			/* port 149 */
	io_trap_in,			/* port 150 */
	io_trap_in,			/* port 151 */
	io_trap_in,			/* port 152 */
	io_trap_in,			/* port 153 */
	io_trap_in,			/* port 154 */
	io_trap_in,			/* port 155 */
	io_trap_in,			/* port 156 */
	io_trap_in,			/* port 157 */
	io_trap_in,			/* port 158 */
	io_trap_in,			/* port 159 */
	io_trap_in,			/* port 160 */
	io_trap_in,			/* port 161 */
	io_trap_in,			/* port 162 */
	io_trap_in,			/* port 163 */
	io_trap_in,			/* port 164 */
	io_trap_in,			/* port 165 */
	io_trap_in,			/* port 166 */
	io_trap_in,			/* port 167 */
	io_trap_in,			/* port 168 */
	io_trap_in,			/* port 169 */
	io_trap_in,			/* port 170 */
	io_trap_in,			/* port 171 */
	io_trap_in,			/* port 172 */
	io_trap_in,			/* port 173 */
	io_trap_in,			/* port 174 */
	io_trap_in,			/* port 175 */
	io_trap_in,			/* port 176 */
	io_trap_in,			/* port 177 */
	io_trap_in,			/* port 178 */
	io_trap_in,			/* port 179 */
	io_trap_in,			/* port 180 */
	io_trap_in,			/* port 181 */
	io_trap_in,			/* port 182 */
	io_trap_in,			/* port 183 */
	io_trap_in,			/* port 184 */
	io_trap_in,			/* port 185 */
	io_trap_in,			/* port 186 */
	io_trap_in,			/* port 187 */
	io_trap_in,			/* port 188 */
	io_trap_in,			/* port 189 */
	io_trap_in,			/* port 190 */
	io_trap_in,			/* port 191 */
	io_trap_in,			/* port 192 */
	io_trap_in,			/* port 193 */
	io_trap_in,			/* port 194 */
	io_trap_in,			/* port 195 */
	io_trap_in,			/* port 196 */
	io_trap_in,			/* port 197 */
	io_trap_in,			/* port 198 */
	io_trap_in,			/* port 199 */
	io_trap_in,			/* port 200 */
	io_trap_in,			/* port 201 */
	io_trap_in,			/* port 202 */
	io_trap_in,			/* port 203 */
	io_trap_in,			/* port 204 */
	io_trap_in,			/* port 205 */
	io_trap_in,			/* port 206 */
	io_trap_in,			/* port 207 */
	io_trap_in,			/* port 208 */
	io_trap_in,			/* port 209 */
	io_trap_in,			/* port 210 */
	io_trap_in,			/* port 211 */
	io_trap_in,			/* port 212 */
	io_trap_in,			/* port 213 */
	io_trap_in,			/* port 214 */
	io_trap_in,			/* port 215 */
	io_trap_in,			/* port 216 */
	io_trap_in,			/* port 217 */
	io_trap_in,			/* port 218 */
	io_trap_in,			/* port 219 */
	io_trap_in,			/* port 220 */
	io_trap_in,			/* port 221 */
	io_trap_in,			/* port 222 */
	io_trap_in,			/* port 223 */
	io_trap_in,			/* port 224 */
	io_trap_in,			/* port 225 */
	io_trap_in,			/* port 226 */
	io_trap_in,			/* port 227 */
	io_trap_in,			/* port 228 */
	io_trap_in,			/* port 229 */
	io_trap_in,			/* port 230 */
	io_trap_in,			/* port 231 */
	io_trap_in,			/* port 232 */
	io_trap_in,			/* port 233 */
	io_trap_in,			/* port 234 */
	io_trap_in,			/* port 235 */
	io_trap_in,			/* port 236 */
	io_trap_in,			/* port 237 */
	io_trap_in,			/* port 238 */
	io_trap_in,			/* port 239 */
	io_trap_in,			/* port 240 */
	io_trap_in,			/* port 241 */
	io_trap_in,			/* port 242 */
	io_trap_in,			/* port 243 */
	io_trap_in,			/* port 244 */
	io_trap_in,			/* port 245 */
	io_trap_in,			/* port 246 */
	io_trap_in,			/* port 247 */
	io_trap_in,			/* port 248 */
	io_trap_in,			/* port 249 */
	io_trap_in,			/* port 250 */
	io_trap_in,			/* port 251 */
	io_trap_in,			/* port 252 */
	io_trap_in,			/* port 253 */
	io_trap_in,			/* port 254 */
	io_trap_in			/* port 255 */
};

/*
 *	This array contains function pointers for every
 *	output I/O port (0 - 255), to do the required I/O.
 */
static void (*port_out[256]) (BYTE) = {
	io_trap_in,	/* port 0 */
	io_trap_in,	/* port 1 */
	io_trap_in,	/* port 2 */
	io_trap_in,/* port 3 */
	io_trap_in,		/* port 4 */
	io_trap_in,	/* port 5 */
	io_trap_in,	/* port 6 */
	io_trap_in,	/* port 7 */
	io_trap_in,	/* port 8 */
	io_trap_in,	/* port 9 */
	io_trap_out,			/* port 10 */
	io_trap_out,			/* port 11 */
	io_trap_out,			/* port 12 */
	io_trap_out,			/* port 13 */
	io_trap_in,	/* port 14 */
	io_trap_in,	/* port 15 */
	io_trap_out,			/* port 16 */
	io_trap_out,			/* port 17 */
	io_trap_out,			/* port 18 */
	io_trap_out,			/* port 19 */
	io_trap_out,			/* port 20 */
	io_trap_out,			/* port 21 */
	io_trap_out,			/* port 22 */
	io_trap_out,			/* port 23 */
	io_trap_out,			/* port 24 */
	io_trap_out,			/* port 25 */
	io_trap_out,			/* port 26 */
	io_trap_out,			/* port 27 */
	io_trap_out,			/* port 28 */
	io_trap_out,			/* port 29 */
	io_trap_out,			/* port 30 */
	io_trap_out,			/* port 31 */
	io_trap_in,	/* port 32 */
	io_trap_in,	/* port 33 */
	io_trap_in,	/* port 34 */
	io_trap_in,/* port 35 */
	io_trap_in,	/* port 36 */
	io_trap_out,			/* port 37 */
	io_trap_out,			/* port 38 */
	io_trap_out,			/* port 39 */
	io_trap_out,			/* port 40 */
	io_trap_out,			/* port 41 */
	io_trap_out,			/* port 42 */
	io_trap_out,			/* port 43 */
	io_trap_out,			/* port 44 */
	io_trap_out,			/* port 45 */
	io_trap_out,			/* port 46 */
	io_trap_out,			/* port 47 */
	io_trap_in,		/* port 48 */
	io_trap_in,		/* port 49 */
	io_trap_in,	/* port 50 */
	io_trap_in,		/* port 51 */
	io_trap_in,	/* port 52 */
	io_trap_out,			/* port 53 */
	io_trap_out,			/* port 54 */
	io_trap_out,			/* port 55 */
	io_trap_out,			/* port 56 */
	io_trap_out,			/* port 57 */
	io_trap_out,			/* port 58 */
	io_trap_out,			/* port 59 */
	io_trap_out,			/* port 60 */
	io_trap_out,			/* port 61 */
	io_trap_out,			/* port 62 */
	io_trap_out,			/* port 63 */
	mmu_out,			/* port 64 */
	io_trap_out,			/* port 65 */
	io_trap_out,			/* port 66 */
	io_trap_out,			/* port 67 */
	io_trap_out,			/* port 68 */
	io_trap_out,			/* port 69 */
	io_trap_out,			/* port 70 */
	io_trap_out,			/* port 71 */
	io_trap_out,			/* port 72 */
	io_trap_out,			/* port 73 */
	io_trap_out,			/* port 74 */
	io_trap_out,			/* port 75 */
	io_trap_out,			/* port 76 */
	io_trap_out,			/* port 77 */
	io_trap_out,			/* port 78 */
	io_trap_out,			/* port 79 */
	io_trap_in,	/* port 80 */
	io_trap_in,	/* port 81 */
	io_trap_in,	/* port 82 */
	io_trap_in,/* port 83 */
	io_trap_in,	/* port 84 */
	io_trap_out,			/* port 85 */
	io_trap_out,			/* port 86 */
	io_trap_out,			/* port 87 */
	io_trap_out,			/* port 88 */
	io_trap_out,			/* port 89 */
	io_trap_out,			/* port 90 */
	io_trap_out,			/* port 91 */
	io_trap_out,			/* port 92 */
	io_trap_out,			/* port 93 */
	io_trap_out,			/* port 94 */
	io_trap_out,			/* port 95 */
	io_trap_out,			/* port 96 */
	io_trap_out,			/* port 97 */
	io_trap_out,			/* port 98 */
	io_trap_out,			/* port 99 */
	io_trap_out,			/* port 100 */
	io_trap_out,			/* port 101 */
	io_trap_out,			/* port 102 */
	io_trap_out,			/* port 103 */
	io_trap_out,			/* port 104 */
	io_trap_out,			/* port 105 */
	io_trap_out,			/* port 106 */
	io_trap_out,			/* port 107 */
	io_trap_out,			/* port 108 */
	io_trap_out,			/* port 109 */
	io_trap_out,			/* port 110 */
	io_trap_out,			/* port 111 */
	io_trap_out,			/* port 112 */
	io_trap_out,			/* port 113 */
	io_trap_out,			/* port 114 */
	io_trap_out,			/* port 115 */
	io_trap_out,			/* port 116 */
	io_trap_out,			/* port 117 */
	io_trap_out,			/* port 118 */
	io_trap_out,			/* port 119 */
	io_trap_out,			/* port 120 */
	io_trap_out,			/* port 121 */
	io_trap_out,			/* port 122 */
	io_trap_out,			/* port 123 */
	io_trap_out,			/* port 124 */
	io_trap_out,			/* port 125 */
	io_trap_out,			/* port 126 */
	io_trap_out,			/* port 127 */
	io_trap_out,			/* port 128 */
	io_trap_out,			/* port 129 */
	io_trap_out,			/* port 130 */
	io_trap_out,			/* port 131 */
	io_trap_out,			/* port 132 */
	io_trap_out,			/* port 133 */
	io_trap_out,			/* port 134 */
	io_trap_out,			/* port 135 */
	io_trap_out,			/* port 136 */
	io_trap_out,			/* port 137 */
	io_trap_out,			/* port 138 */
	io_trap_out,			/* port 139 */
	io_trap_out,			/* port 140 */
	io_trap_out,			/* port 141 */
	io_trap_out,			/* port 142 */
	io_trap_out,			/* port 143 */
	io_trap_out,			/* port 144 */
	io_trap_out,			/* port 145 */
	io_trap_out,			/* port 146 */
	io_trap_out,			/* port 147 */
	io_trap_out,			/* port 148 */
	io_trap_out,			/* port 149 */
	io_trap_out,			/* port 150 */
	io_trap_out,			/* port 151 */
	io_trap_out,			/* port 152 */
	io_trap_out,			/* port 153 */
	io_trap_out,			/* port 154 */
	io_trap_out,			/* port 155 */
	io_trap_out,			/* port 156 */
	io_trap_out,			/* port 157 */
	io_trap_out,			/* port 158 */
	io_trap_out,			/* port 159 */
	io_trap_out,			/* port 160 */
	io_trap_out,			/* port 161 */
	io_trap_out,			/* port 162 */
	io_trap_out,			/* port 163 */
	io_trap_out,			/* port 164 */
	io_trap_out,			/* port 165 */
	io_trap_out,			/* port 166 */
	io_trap_out,			/* port 167 */
	io_trap_out,			/* port 168 */
	io_trap_out,			/* port 169 */
	io_trap_out,			/* port 170 */
	io_trap_out,			/* port 171 */
	io_trap_out,			/* port 172 */
	io_trap_out,			/* port 173 */
	io_trap_out,			/* port 174 */
	io_trap_out,			/* port 175 */
	io_trap_out,			/* port 176 */
	io_trap_out,			/* port 177 */
	io_trap_out,			/* port 178 */
	io_trap_out,			/* port 179 */
	io_trap_out,			/* port 180 */
	io_trap_out,			/* port 181 */
	io_trap_out,			/* port 182 */
	io_trap_out,			/* port 183 */
	io_trap_out,			/* port 184 */
	io_trap_out,			/* port 185 */
	io_trap_out,			/* port 186 */
	io_trap_out,			/* port 187 */
	io_trap_out,			/* port 188 */
	io_trap_out,			/* port 189 */
	io_trap_out,			/* port 190 */
	io_trap_out,			/* port 191 */
	io_trap_out,			/* port 192 */
	io_trap_out,			/* port 193 */
	io_trap_out,			/* port 194 */
	io_trap_out,			/* port 195 */
	io_trap_out,			/* port 196 */
	io_trap_out,			/* port 197 */
	io_trap_out,			/* port 198 */
	io_trap_out,			/* port 199 */
	io_trap_out,			/* port 200 */
	io_trap_out,			/* port 201 */
	io_trap_out,			/* port 202 */
	io_trap_out,			/* port 203 */
	io_trap_out,			/* port 204 */
	io_trap_out,			/* port 205 */
	io_trap_out,			/* port 206 */
	io_trap_out,			/* port 207 */
	io_trap_out,			/* port 208 */
	io_trap_out,			/* port 209 */
	io_trap_out,			/* port 210 */
	io_trap_out,			/* port 211 */
	io_trap_out,			/* port 212 */
	io_trap_out,			/* port 213 */
	io_trap_out,			/* port 214 */
	io_trap_out,			/* port 215 */
	io_trap_out,			/* port 216 */
	io_trap_out,			/* port 217 */
	io_trap_out,			/* port 218 */
	io_trap_out,			/* port 219 */
	io_trap_out,			/* port 220 */
	io_trap_out,			/* port 221 */
	io_trap_out,			/* port 222 */
	io_trap_out,			/* port 223 */
	io_trap_out,			/* port 224 */
	io_trap_out,			/* port 225 */
	io_trap_out,			/* port 226 */
	io_trap_out,			/* port 227 */
	io_trap_out,			/* port 228 */
	io_trap_out,			/* port 229 */
	io_trap_out,			/* port 230 */
	io_trap_out,			/* port 231 */
	io_trap_out,			/* port 232 */
	io_trap_out,			/* port 233 */
	io_trap_out,			/* port 234 */
	io_trap_out,			/* port 235 */
	io_trap_out,			/* port 236 */
	io_trap_out,			/* port 237 */
	io_trap_out,			/* port 238 */
	io_trap_out,			/* port 239 */
	io_trap_out,			/* port 240 */
	io_trap_out,			/* port 241 */
	io_trap_out,			/* port 242 */
	io_trap_out,			/* port 243 */
	io_trap_out,			/* port 244 */
	io_trap_out,			/* port 245 */
	io_trap_out,			/* port 246 */
	io_trap_out,			/* port 247 */
	io_trap_out,			/* port 248 */
	io_trap_out,			/* port 249 */
	io_trap_out,			/* port 250 */
	io_trap_out,			/* port 251 */
	io_trap_out,			/* port 252 */
	io_trap_out,			/* port 253 */
	io_trap_out,			/* port 254 */
	io_trap_out			/* port 255 */
};

/*
 *	This function is to initiate the I/O devices.
 *	It will be called from the CPU simulation before
 *	any operation with the CPU is possible.
 */
void init_io(void)
{
	register int i;
	pthread_t thread;
	static struct itimerval tim;
	static struct sigaction newact;

	/* create the thread for timer and interrupt handling */
	if (pthread_create(&thread, NULL, timing, (void *) NULL)) {
		printf("can't create timing thread\n");
		exit(1);
	}
	//pthread_setschedprio(thread, sched_get_priority_max(SCHED_RR));

	/* start 10ms interrupt timer, delayed! */
	newact.sa_handler = interrupt;
	sigaction(SIGALRM, &newact, NULL);
	tim.it_value.tv_sec = 5;
	tim.it_value.tv_usec = 0;
	tim.it_interval.tv_sec = 0;
	tim.it_interval.tv_usec = 10000;
	setitimer(ITIMER_REAL, &tim, NULL);

	printf("\n");
}

/*
 *	This function is to stop the I/O devices. It is
 *	called from the CPU simulation on exit.
 */
void exit_io(void)
{
	register int i;
	static struct itimerval tim;
	static struct sigaction newact;

	/* close network connections */
	for (i = 0; i < NUMNSOC; i++) {
		if (ncons[i].ssc)
			close(ncons[i].ssc);
	}

	/* stop 10ms interrupt timer */
	newact.sa_handler = SIG_IGN;
	memset((void *) &newact.sa_mask, 0, sizeof(newact.sa_mask));
	newact.sa_flags = 0;
	sigaction(SIGALRM, &newact, NULL);
	tim.it_value.tv_sec = 0;
	tim.it_value.tv_usec = 0;
	tim.it_interval.tv_sec = 0;
	tim.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &tim, NULL);
}

/*
 *	This is the main handler for all IN op-codes,
 *	called by the simulator. It calls the input
 *	function for port addr.
 */
BYTE io_in(BYTE addrl, BYTE addrh)
{
	extern void wait_step(void);

	io_port = addrl;
	io_data = (*port_in[addrl]) ();
	//printf("input %02x from port %02x\r\n", io_data, io_port);

	cpu_bus = CPU_WO | CPU_INP;

#ifdef notdef
	fp_clock += 3;
	fp_led_address = (addrh << 8) + addrl;
	fp_led_data = io_data;
	fp_sampleData();
	wait_step();
#endif
	return(io_data);
}

/*
 *	This is the main handler for all OUT op-codes,
 *	called by the simulator. It calls the output
 *	function for port addr.
 */
void io_out(BYTE addrl, BYTE addrh, BYTE data)
{
	extern void wait_step();

	io_port = addrl;
	io_data = data;
	(*port_out[addrl]) (data);
	//printf("output %02x to port %02x\r\n", io_data, io_port);

	cpu_bus = CPU_OUT;

#ifdef notdef
	fp_clock += 6;
	fp_led_address = (addrh << 8) + addrl;
	fp_led_data = 0xff;
	fp_sampleData();
	wait_step();
#endif
}

/*
 *	I/O input trap function
 *	This function should be added into all unused
 *	entries of the input port array. It can stop the
 *	emulation with an I/O error.
 */
static BYTE io_trap_in(void)
{
	//printf("I/O trap in port %02x\r\n", io_port);
	if (i_flag) {
		cpu_error = IOTRAPIN;
		cpu_state = STOPPED;
	}
	return((BYTE) 0xff);
}

/*
 *	I/O output trap function
 *	This function should be added into all unused
 *	entries of the output port array. It can stop the
 *	emulation with an I/O error.
 */
static void io_trap_out(BYTE data)
{
	data = data; /* to avoid compiler warning */

	//printf("I/O trap out port %02x\r\n", io_port);

	if (i_flag) {
		cpu_error = IOTRAPOUT;
		cpu_state = STOPPED;
	}
}

#ifdef notdef
/*
 *	Read input from front panel switches
 */
static BYTE fp_in(void)
{
	return(address_switch >> 8);
}

/*
 *	Write output to front panel lights
 */
static void fp_out(BYTE data)
{
	fp_led_output = data;
}
#endif

/*
 *	read MMU register
 */
static BYTE mmu_in(void)
{
	return(bankio);
}

/*
 *	write MMU register
 */
static void mmu_out(BYTE data)
{
	int sel;

	//printf("mmu select bank %02x\r\n", data);
	bankio = data;

	/* set banks */
	switch (data) {
	case 0x00:
	case 0x01:
		sel = 0;
		common = 0;
		break;
	case 0x02:
		sel = 1;
		common = 0;
		break;
	case 0x04:
		sel = 2;
		common = 0;
		break;
	case 0x08:
		sel = 3;
		common = 0;
		break;
	case 0x10:
		sel = 4;
		common = 0;
		break;
	case 0x20:
		sel = 5;
		common = 0;
		break;
	case 0x40:
		sel = 6;
		common = 0;
		break;
	case 0x80:
	case 0x81:
		sel = 0;
		common = 1;
		break;
	default:
		printf("Not supported bank select = %02x\r\n", data);
		cpu_error = IOHALT;
		cpu_state = STOPPED;
		return;
	}

	selbnk = sel;
}

/*
 *	Thread for timing and interrupts
 */
void *timing(void *arg)
{
	struct timespec timer;	/* sleep timer */
	struct timeval t1, t2, tdiff;

	arg = arg;	/* to avoid compiler warning */
	//printf("timing thread started\r\n");
	gettimeofday(&t1, NULL);

	while (1) {	/* 1 msec per loop iteration */

		/* check for interrupts from highest priority to lowest */

		/* if last interrupt not acknowledged by CPU no new one yet */
		if (int_int)
			goto next;

next:
		/* compute time used for processing */
		gettimeofday(&t2, NULL);
		tdiff.tv_sec = t2.tv_sec - t1.tv_sec;
		tdiff.tv_usec = t2.tv_usec - t1.tv_usec;
		if (tdiff.tv_usec < 0) {
			--tdiff.tv_sec;
			tdiff.tv_usec += 1000000;
		}
	
		/* sleep for the difference to 1 msec */
		if ((tdiff.tv_sec == 0) && (tdiff.tv_usec < 1000)) {
			timer.tv_sec = 0;
			timer.tv_nsec = (long) ((1000 - tdiff.tv_usec) * 1000);
			nanosleep(&timer, NULL);
		}

		gettimeofday(&t1, NULL);
	}

	/* never reached, this thread is running endless */
	pthread_exit(NULL);
}

/*
 *	10ms interrupt handler
 */
void interrupt(int sig)
{
	static unsigned long counter = 0L;
	struct pollfd p[1];

	sig = sig;	/* to avoid compiler warning */

	counter++;

#ifdef notdef
	/* check for RDA */
	p[0].fd = fileno(stdin);
	p[0].events = POLLIN;
	p[0].revents = 0;
	poll(p, 1, 0);
	if (p[0].revents & POLLIN)
		uart0a_rda = 1;
	else
		uart0a_rda = 0;

	if (ncons[0].ssc != 0) {
		p[0].fd = ncons[0].ssc;
		p[0].events = POLLIN;
		p[0].revents = 0;
		poll(p, 1, 0);
		if (p[0].revents & POLLHUP) {
			close(ncons[0].ssc);
			ncons[0].ssc = 0;
			uart1a_rda = 0;
		} else if (p[0].revents & POLLIN) {
			uart1a_rda = 1;
		} else {
			uart1a_rda = 0;
		}
	}

	if (ncons[1].ssc != 0) {
		p[0].fd = ncons[1].ssc;
		p[0].events = POLLIN;
		p[0].revents = 0;
		poll(p, 1, 0);
		if (p[0].revents & POLLHUP) {
			close(ncons[1].ssc);
			ncons[1].ssc = 0;
			uart1b_rda = 0;
		} else if (p[0].revents & POLLIN) {
			uart1b_rda = 1;
		} else {
			uart1b_rda = 0;
		}
	}
#endif
}
