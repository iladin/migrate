/*
 * Copyright (C) 2001-2006 Keisuke Nishida
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1,
 * or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING.LIB.  If
 * not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 USA
 */

#include "config.h"
#include "defaults.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef	_WIN32
#include <io.h>
#include <fcntl.h>
#undef	HAVE_SIGNAL_H
#endif

#ifdef	HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef	HAVE_LOCALE_H
#include <locale.h>
#endif

#include "common.h"
#include "move.h"
#include "numeric.h"
#include "termio.h"
#include "fileio.h"
#include "call.h"
#include "screenio.h"
#include "intrinsic.h"
#include "lib/gettext.h"

struct cob_exception {
	const int	code;
	const char	*name;
	const int	critical;
};

static int		cob_argc = 0;
static char		**cob_argv = NULL;
static const unsigned char *old_sequence;

int			cob_initialized = 0;
int			cob_exception_code = 0;

cob_module		*cob_current_module = NULL;

const char		*cob_source_file = NULL;
const char		*cob_source_statement = NULL;
const char		*cob_orig_statement = NULL;
const char		*cob_current_program_id = NULL;
const char		*cob_current_section = NULL;
const char		*cob_current_paragraph = NULL;
const char		*cob_orig_program_id = NULL;
const char		*cob_orig_section = NULL;
const char		*cob_orig_paragraph = NULL;
unsigned int		cob_source_line = 0;
unsigned int		cob_orig_line = 0;

int			cob_call_params = 0;
int			cob_initial_external = 0;
int			cob_got_exception = 0;

#ifdef	HAVE_SIGNAL_H
typedef void (*cob_sighandler_t) (int);
static cob_sighandler_t	hupsig = NULL;
static cob_sighandler_t	intsig = NULL;
static cob_sighandler_t	qutsig = NULL;
#endif

static cob_field_attr	all_attr = { COB_TYPE_ALPHANUMERIC_ALL, 0, 0, 0, NULL };

cob_field		cob_zero = { 1, (ucharptr)"0", &all_attr };
cob_field		cob_space = { 1, (ucharptr)" ", &all_attr };
cob_field		cob_high = { 1, (ucharptr)"\xff", &all_attr };
cob_field		cob_low = { 1, (ucharptr)"\0", &all_attr };
cob_field		cob_quote = { 1, (ucharptr)"\"", &all_attr };

const int		cob_exp10[10] = {
	1,
	10,
	100,
	1000,
	10000,
	100000,
	1000000,
	10000000,
	100000000,
	1000000000
};

const long long		cob_exp10LL[19] = {
	1LL,
	10LL,
	100LL,
	1000LL,
	10000LL,
	100000LL,
	1000000LL,
	10000000LL,
	100000000LL,
	1000000000LL,
	10000000000LL,
	100000000000LL,
	1000000000000LL,
	10000000000000LL,
	100000000000000LL,
	1000000000000000LL,
	10000000000000000LL,
	100000000000000000LL,
	1000000000000000000LL
};

/* Generated by codegen - ASCII to EBCDIC MF like
const unsigned char	cob_a2e[256] = {
	0x00, 0x01, 0x02, 0x03, 0x1D, 0x19, 0x1A, 0x1B,
	0x0F, 0x04, 0x16, 0x06, 0x07, 0x08, 0x09, 0x0A,
	0x0B, 0x0C, 0x0D, 0x0E, 0x1E, 0x1F, 0x1C, 0x17,
	0x10, 0x11, 0x20, 0x18, 0x12, 0x13, 0x14, 0x15,
	0x21, 0x27, 0x3A, 0x36, 0x28, 0x30, 0x26, 0x38,
	0x24, 0x2A, 0x29, 0x25, 0x2F, 0x2C, 0x22, 0x2D,
	0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
	0x7B, 0x7C, 0x35, 0x2B, 0x23, 0x39, 0x32, 0x33,
	0x37, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D,
	0x5E, 0x5F, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
	0x67, 0x68, 0x69, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x7D, 0x6A, 0x7E, 0x7F, 0x31,
	0x34, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41,
	0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4A, 0x4B, 0x4C, 0x4E, 0x4F, 0x50, 0x51, 0x52,
	0x53, 0x54, 0x55, 0x56, 0x2E, 0x60, 0x4D, 0x05,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
	0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
	0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
	0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
	0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};
end comment */

/* Full Table ASCII to EBCDIC
const unsigned char	cob_a2e[256] = {
	0x00, 0x01, 0x02, 0x03, 0x37, 0x2D, 0x2E, 0x2F,
	0x16, 0x05, 0x25, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x3C, 0x3D, 0x32, 0x26,
	0x18, 0x19, 0x3F, 0x27, 0x1C, 0x1D, 0x1E, 0x1F,
	0x40, 0x5A, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D,
	0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F,
	0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
	0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
	0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
	0xE7, 0xE8, 0xE9, 0xAD, 0xE0, 0xBD, 0x5F, 0x6D,
	0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
	0xA7, 0xA8, 0xA9, 0xC0, 0x6A, 0xD0, 0xA1, 0x07,
	0x68, 0xDC, 0x51, 0x42, 0x43, 0x44, 0x47, 0x48,
	0x52, 0x53, 0x54, 0x57, 0x56, 0x58, 0x63, 0x67,
	0x71, 0x9C, 0x9E, 0xCB, 0xCC, 0xCD, 0xDB, 0xDD,
	0xDF, 0xEC, 0xFC, 0xB0, 0xB1, 0xB2, 0x3E, 0xB4,
	0x45, 0x55, 0xCE, 0xDE, 0x49, 0x69, 0x9A, 0x9B,
	0xAB, 0x9F, 0xBA, 0xB8, 0xB7, 0xAA, 0x8A, 0x8B,
	0xB6, 0xB5, 0x62, 0x4F, 0x64, 0x65, 0x66, 0x20,
	0x21, 0x22, 0x70, 0x23, 0x72, 0x73, 0x74, 0xBE,
	0x76, 0x77, 0x78, 0x80, 0x24, 0x15, 0x8C, 0x8D,
	0x8E, 0x41, 0x06, 0x17, 0x28, 0x29, 0x9D, 0x2A,
	0x2B, 0x2C, 0x09, 0x0A, 0xAC, 0x4A, 0xAE, 0xAF,
	0x1B, 0x30, 0x31, 0xFA, 0x1A, 0x33, 0x34, 0x35,
	0x36, 0x59, 0x08, 0x38, 0xBC, 0x39, 0xA0, 0xBF,
	0xCA, 0x3A, 0xFE, 0x3B, 0x04, 0xCF, 0xDA, 0x14,
	0xE1, 0x8F, 0x46, 0x75, 0xFD, 0xEB, 0xEE, 0xED,
	0x90, 0xEF, 0xB3, 0xFB, 0xB9, 0xEA, 0xBB, 0xFF
};
end comment */

/* Not needed at the moment - EBCDIC to ASCII MF like 
const unsigned char	cob_e2a[256] = {
	0x00, 0x01, 0x02, 0x03, 0x9C, 0x09, 0x86, 0x7F,
	0x97, 0x8D, 0x8E, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x9D, 0x85, 0x08, 0x87,
	0x18, 0x19, 0x92, 0x8F, 0x1C, 0x1D, 0x1E, 0x1F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x0A, 0x17, 0x1B,
	0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x05, 0x06, 0x07,
	0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04,
	0x98, 0x99, 0x9A, 0x9B, 0x14, 0x15, 0x9E, 0x1A,
	0x20, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
	0xA7, 0xA8, 0x5B, 0x2E, 0x3C, 0x28, 0x2B, 0x21,
	0x26, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0x5D, 0x24, 0x2A, 0x29, 0x3B, 0x5E,
	0x2D, 0x2F, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
	0xB8, 0xB9, 0x7C, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,
	0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1,
	0xC2, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,
	0xC3, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
	0xCA, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
	0x71, 0x72, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
	0xD1, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7A, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
	0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED,
	0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
	0x51, 0x52, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3,
	0x5C, 0x9F, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5A, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};
end comment */

/* Not needed at the moment - EBCDIC to ASCII Full table
const unsigned char	cob_e2a[256] = {
	0x00, 0x01, 0x02, 0x03, 0xEC, 0x09, 0xCA, 0x7F,
	0xE2, 0xD2, 0xD3, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0xEF, 0xC5, 0x08, 0xCB,
	0x18, 0x19, 0xDC, 0xD8, 0x1C, 0x1D, 0x1E, 0x1F,
	0xB7, 0xB8, 0xB9, 0xBB, 0xC4, 0x0A, 0x17, 0x1B,
	0xCC, 0xCD, 0xCF, 0xD0, 0xD1, 0x05, 0x06, 0x07,
	0xD9, 0xDA, 0x16, 0xDD, 0xDE, 0xDF, 0xE0, 0x04,
	0xE3, 0xE5, 0xE9, 0xEB, 0x14, 0x15, 0x9E, 0x1A,
	0x20, 0xC9, 0x83, 0x84, 0x85, 0xA0, 0xF2, 0x86,
	0x87, 0xA4, 0xD5, 0x2E, 0x3C, 0x28, 0x2B, 0xB3,
	0x26, 0x82, 0x88, 0x89, 0x8A, 0xA1, 0x8C, 0x8B,
	0x8D, 0xE1, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x5E,
	0x2D, 0x2F, 0xB2, 0x8E, 0xB4, 0xB5, 0xB6, 0x8F,
	0x80, 0xA5, 0x7C, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,
	0xBA, 0x90, 0xBC, 0xBD, 0xBE, 0xF3, 0xC0, 0xC1,
	0xC2, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,
	0xC3, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0xAE, 0xAF, 0xC6, 0xC7, 0xC8, 0xF1,
	0xF8, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
	0x71, 0x72, 0xA6, 0xA7, 0x91, 0xCE, 0x92, 0xA9,
	0xE6, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7A, 0xAD, 0xA8, 0xD4, 0x5B, 0xD6, 0xD7,
	0x9B, 0x9C, 0x9D, 0xFA, 0x9F, 0xB1, 0xB0, 0xAC,
	0xAB, 0xFC, 0xAA, 0xFE, 0xE4, 0x5D, 0xBF, 0xE7,
	0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0xE8, 0x93, 0x94, 0x95, 0xA2, 0xED,
	0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
	0x51, 0x52, 0xEE, 0x96, 0x81, 0x97, 0xA3, 0x98,
	0x5C, 0xF0, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5A, 0xFD, 0xF5, 0x99, 0xF7, 0xF6, 0xF9,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0xDB, 0xFB, 0x9A, 0xF4, 0xEA, 0xFF
};
end comment */

static const struct cob_exception	cob_exception_table[] = {
	{0, NULL, 0},		/* COB_EC_ZERO */
#undef COB_EXCEPTION
#define COB_EXCEPTION(CODE,TAG,NAME,CRITICAL) { 0x##CODE, NAME, CRITICAL },
#include "exception.def"
	{0, NULL, 0}		/* COB_EC_MAX */
};

#define EXCEPTION_TAB_SIZE	sizeof(cob_exception_table) / sizeof(struct cob_exception)

static int		cob_switch[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/*
 * General functions
 */

char *
cob_get_exception_name (const int exception_code)
{
	size_t	n;

	for (n = 0; n < EXCEPTION_TAB_SIZE; n++) {
		if (exception_code == cob_exception_table[n].code) {
			return (char *)cob_exception_table[n].name;
		}
	}
	return NULL;
}

void
cob_set_exception (const int id)
{
	cob_exception_code = cob_exception_table[id].code;
	if (cob_exception_code) {
		cob_got_exception = 1;
		cob_orig_statement = cob_source_statement;
		cob_orig_line = cob_source_line;
		cob_orig_program_id = cob_current_program_id;
		cob_orig_section = cob_current_section;
		cob_orig_paragraph = cob_current_paragraph;
	}
}


/* static sighandler_t	oldsig; */

#ifdef	HAVE_SIGNAL_H
static void
cob_sig_handler (int sig)
{
	fprintf (stderr, "Abnormal termination - File contents may not be correct\n");
	fflush (stderr);
	cob_screen_terminate ();
	cob_exit_fileio ();
	switch ( sig ) {
	case SIGHUP:
		if ( (hupsig != SIG_IGN) && (hupsig != SIG_DFL) ) {
			(*hupsig)(SIGHUP);
		}
		break;
	case SIGINT:
		if ( (intsig != SIG_IGN) && (intsig != SIG_DFL) ) {
			(*intsig)(SIGINT);
		}
		break;
	case SIGQUIT:
		if ( (qutsig != SIG_IGN) && (qutsig != SIG_DFL) ) {
			(*qutsig)(SIGQUIT);
		}
		break;
	}
	exit (sig);
}
#endif

void
cob_set_signal ()
{
#ifdef	HAVE_SIGNAL_H
	if ((intsig = signal(SIGINT, cob_sig_handler)) == SIG_IGN) {
		(void)signal(SIGINT, SIG_IGN);
	}
	if ((hupsig = signal(SIGHUP, cob_sig_handler)) == SIG_IGN) {
		(void)signal(SIGHUP, SIG_IGN);
	}
	if ((qutsig = signal(SIGQUIT, cob_sig_handler)) == SIG_IGN) {
		(void)signal(SIGQUIT, SIG_IGN);
	}
#endif
}

void
cob_init (int argc, char **argv)
{
	char	*s;
	int	i;
	char	buff[32];

	if (!cob_initialized) {
		cob_argc = argc;
		cob_argv = argv;

#ifdef	HAVE_SETLOCALE
		setlocale (LC_ALL, "");
		setlocale (LC_NUMERIC, "C");
#endif
#ifdef	ENABLE_NLS
		bindtextdomain (PACKAGE, LOCALEDIR);
		textdomain (PACKAGE);
#endif

/* Dirty hack until we implement something better */
#if defined(_WIN32) && !defined(_MSC_VER)
		_setmode (_fileno (stdin), _O_BINARY);
		_setmode (_fileno (stdout), _O_BINARY);
		_setmode (_fileno (stderr), _O_BINARY);
#endif

		cob_init_numeric ();
#if 0
		cob_init_termio ();
#endif
		cob_init_fileio ();
		cob_init_call ();
		cob_init_intrinsic ();

		for (i = 0; i < 8; i++) {
			memset (buff, 0, sizeof (buff));
			sprintf (buff, "COB_SWITCH_%d", i + 1);
			s = getenv (buff);
			if (s && strcasecmp (s, "ON") == 0) {
				cob_switch[i] = 1;
			}
		}

		cob_initialized = 1;
	}
}

void
cob_module_enter (cob_module *module)
{
	if (!cob_initialized) {
		fputs (_("warning: cob_init expected in the main program\n"), stderr);
		cob_init (0, NULL);
	}

	module->next = cob_current_module;
	cob_current_module = module;
}

void
cob_module_leave (cob_module *module)
{
	cob_current_module = cob_current_module->next;
}

void
cob_fatal_error (const enum cob_enum_error fatal_error)
{
	fputs ("ERROR - ", stderr);
	switch (fatal_error) {
	case COB_FERROR_INITIALIZED:
		fputs ("cob_init() has not been called\n", stderr);
		break;
	case COB_FERROR_CODEGEN:
		fputs ("Codegen error - Please report this\n", stderr);
		break;
	case COB_FERROR_CHAINING:
		fputs ("ERROR - Recursive call of chained program\n", stderr);
		break;
	case COB_FERROR_STACK:
		fputs ("Stack overflow, possible PERFORM depth exceeded\n", stderr);
		break;
	default:
		fputs ("Unknown failure\n", stderr);
		break;
	}
	cob_stop_run (1);
}

void
cob_stop_run (const int status)
{
	cob_screen_terminate ();
	cob_exit_fileio ();
	exit (status);
}

void
cob_check_version (const char *prog, const char *packver, const int patchlev)
{
	if (strcmp (packver, PACKAGE_VERSION) || patchlev != PATCH_LEVEL) {
		cob_runtime_error (_("Error - Version mismatch"));
		cob_runtime_error (_("%s has version/patch level %s/%d"), prog, packver,
				   patchlev);
		cob_runtime_error (_("Library has version/patch level %s/%d"), PACKAGE_VERSION,
				   PATCH_LEVEL);
		cob_stop_run (1);
	}
	return;
}

/*
 * Sign
 */

static int
cob_get_sign_ebcdic (unsigned char *p)
{
	switch (*p) {
	case '{':
		*p = (unsigned char)'0';
		return 1;
	case 'A':
		*p = (unsigned char)'1';
		return 1;
	case 'B':
		*p = (unsigned char)'2';
		return 1;
	case 'C':
		*p = (unsigned char)'3';
		return 1;
	case 'D':
		*p = (unsigned char)'4';
		return 1;
	case 'E':
		*p = (unsigned char)'5';
		return 1;
	case 'F':
		*p = (unsigned char)'6';
		return 1;
	case 'G':
		*p = (unsigned char)'7';
		return 1;
	case 'H':
		*p = (unsigned char)'8';
		return 1;
	case 'I':
		*p = (unsigned char)'9';
		return 1;
	case '}':
		*p = (unsigned char)'0';
		return -1;
	case 'J':
		*p = (unsigned char)'1';
		return -1;
	case 'K':
		*p = (unsigned char)'2';
		return -1;
	case 'L':
		*p = (unsigned char)'3';
		return -1;
	case 'M':
		*p = (unsigned char)'4';
		return -1;
	case 'N':
		*p = (unsigned char)'5';
		return -1;
	case 'O':
		*p = (unsigned char)'6';
		return -1;
	case 'P':
		*p = (unsigned char)'7';
		return -1;
	case 'Q':
		*p = (unsigned char)'8';
		return -1;
	case 'R':
		*p = (unsigned char)'9';
		return -1;
	default:
		/* What to do here */
		*p = (unsigned char)'0';
		return 1;
	}
/* NOT REACHED */
	return 1;
}

static void
cob_put_sign_ebcdic (unsigned char *p, const int sign)
{
	if (sign < 0) {
		switch (*p) {
		case '0':
			*p = (unsigned char)'}';
			return;
		case '1':
			*p = (unsigned char)'J';
			return;
		case '2':
			*p = (unsigned char)'K';
			return;
		case '3':
			*p = (unsigned char)'L';
			return;
		case '4':
			*p = (unsigned char)'M';
			return;
		case '5':
			*p = (unsigned char)'N';
			return;
		case '6':
			*p = (unsigned char)'O';
			return;
		case '7':
			*p = (unsigned char)'P';
			return;
		case '8':
			*p = (unsigned char)'Q';
			return;
		case '9':
			*p = (unsigned char)'R';
			return;
		default:
			/* What to do here */
			*p = (unsigned char)'}';
			return;
		}
	}
	switch (*p) {
	case '0':
		*p = (unsigned char)'{';
		return;
	case '1':
		*p = (unsigned char)'A';
		return;
	case '2':
		*p = (unsigned char)'B';
		return;
	case '3':
		*p = (unsigned char)'C';
		return;
	case '4':
		*p = (unsigned char)'D';
		return;
	case '5':
		*p = (unsigned char)'E';
		return;
	case '6':
		*p = (unsigned char)'F';
		return;
	case '7':
		*p = (unsigned char)'G';
		return;
	case '8':
		*p = (unsigned char)'H';
		return;
	case '9':
		*p = (unsigned char)'I';
		return;
	default:
		/* What to do here */
		*p = (unsigned char)'{';
		return;
	}
/* NOT REACHED */
}

int
cob_real_get_sign (cob_field *f)
{
	switch (COB_FIELD_TYPE (f)) {
	case COB_TYPE_NUMERIC_DISPLAY:
	{
		unsigned char *p;

		/* locate sign */
		if (unlikely(COB_FIELD_SIGN_LEADING (f))) {
			p = f->data;
		} else {
			p = f->data + f->size - 1;
		}

		/* get sign */
		if (unlikely(COB_FIELD_SIGN_SEPARATE (f))) {
			return (*p == '+') ? 1 : -1;
		} else {
			if (*p >= '0' && *p <= '9') {
				return 1;
			}
			switch (cob_current_module->display_sign) {
			case COB_DISPLAY_SIGN_ASCII:
				GET_SIGN_ASCII (*p);
				break;
			case COB_DISPLAY_SIGN_ASCII10:
				GET_SIGN_ASCII10 (*p);
				break;
			case COB_DISPLAY_SIGN_ASCII20:
				GET_SIGN_ASCII20 (*p);
				break;
			case COB_DISPLAY_SIGN_EBCDIC:
				return cob_get_sign_ebcdic (p);
			default:
				cob_runtime_error (_("Invalid display sign '%d'"),
						cob_current_module->display_sign);
				cob_stop_run (1);
			}
			return -1;
		}
	}
	case COB_TYPE_NUMERIC_PACKED:
	{
		unsigned char *p = f->data + f->attr->digits / 2;

		return ((*p & 0x0f) == 0x0d) ? -1 : 1;
	}
	default:
		return 0;
	}
}

void
cob_real_put_sign (cob_field *f, const int sign)
{
	switch (COB_FIELD_TYPE (f)) {
	case COB_TYPE_NUMERIC_DISPLAY:
	{
		unsigned char *p;

		/* locate sign */
		if (unlikely(COB_FIELD_SIGN_LEADING (f))) {
			p = f->data;
		} else {
			p = f->data + f->size - 1;
		}

		/* put sign */
		if (unlikely(COB_FIELD_SIGN_SEPARATE (f))) {
			int c = (sign < 0) ? '-' : '+';

			if (*p != c) {
				*p = c;
			}
		} else if (unlikely(cob_current_module->display_sign == COB_DISPLAY_SIGN_EBCDIC)) {
			cob_put_sign_ebcdic (p, sign);
		} else if (sign < 0) {
			switch (cob_current_module->display_sign) {
			case COB_DISPLAY_SIGN_ASCII:
				PUT_SIGN_ASCII (*p);
				break;
			case COB_DISPLAY_SIGN_ASCII10:
				PUT_SIGN_ASCII10 (*p);
				break;
			case COB_DISPLAY_SIGN_ASCII20:
				PUT_SIGN_ASCII20 (*p);
				break;
			default:
				cob_runtime_error (_("Invalid display sign '%d'"),
						cob_current_module->display_sign);
				cob_stop_run (1);
			}
		}
		return;
	}
	case COB_TYPE_NUMERIC_PACKED:
	{
		unsigned char *p = f->data + f->attr->digits / 2;

		if (sign < 0) {
			*p = (*p & 0xf0) | 0x0d;
		} else {
			*p = (*p & 0xf0) | 0x0c;
		}
		return;
	}
	default:
		return;
	}
}

char *
cob_field_to_string (cob_field *f, char *s)
{
	int	i;

	memcpy (s, f->data, f->size);
	for (i = (int) f->size - 1; i >= 0; i--) {
		if (s[i] != ' ') {
			break;
		}
	}
	s[i + 1] = '\0';
	return s;
}

/*
 * Switch
 */

int
cob_get_switch (int n)
{
	return cob_switch[n];
}

void
cob_set_switch (int n, int flag)
{
	cob_switch[n] = flag;
}

/*
 * Comparison
 */

static int
cmpc (unsigned char *s1, unsigned char c, size_t size)
{
	size_t			i;
	int			ret = 0;
	const unsigned char	*s = cob_current_module->collating_sequence;

	if (s) {
		for (i = 0; i < size; i++) {
			if ((ret = s[s1[i]] - s[c]) != 0) {
				return ret;
			}
		}
	} else {
		for (i = 0; i < size; i++) {
			if ((ret = s1[i] - c) != 0) {
				return ret;
			}
		}
	}
	return ret;
}

static int
cmps (unsigned char *s1, unsigned char *s2, size_t size)
{
	size_t			i;
	int			ret = 0;
	const unsigned char	*s = cob_current_module->collating_sequence;

	if (s) {
		for (i = 0; i < size; i++) {
			if ((ret = s[s1[i]] - s[s2[i]]) != 0) {
				return ret;
			}
		}
	} else {
		for (i = 0; i < size; i++) {
			if ((ret = s1[i] - s2[i]) != 0) {
				return ret;
			}
		}
	}
	return ret;
}

static int
cob_cmp_char (cob_field *f, unsigned char c)
{
	int	sign = cob_get_sign (f);
	int	ret = cmpc (f->data, c, f->size);

	if (COB_FIELD_TYPE (f) != COB_TYPE_NUMERIC_PACKED) {
		cob_put_sign (f, sign);
	}
	return ret;
}

static int
cob_cmp_all (cob_field *f1, cob_field *f2)
{
	int		ret = 0;
	int		sign = cob_get_sign (f1);
	size_t		size = f1->size;
	unsigned char	*data = f1->data;

	while (size >= f2->size) {
		if ((ret = cmps (data, f2->data, f2->size)) != 0) {
			goto end;
		}
		size -= f2->size;
		data += f2->size;
	}
	if (size > 0) {
		ret = cmps (data, f2->data, size);
	}

end:
	if (COB_FIELD_TYPE (f1) != COB_TYPE_NUMERIC_PACKED) {
		cob_put_sign (f1, sign);
	}
	return ret;
}

static int
cob_cmp_alnum (cob_field *f1, cob_field *f2)
{
	int		ret = 0;
	int		sign1 = cob_get_sign (f1);
	int		sign2 = cob_get_sign (f2);
	size_t		min = (f1->size < f2->size) ? f1->size : f2->size;

	/* compare common substring */
	if ((ret = cmps (f1->data, f2->data, min)) != 0) {
		goto end;
	}

	/* compare the rest (if any) with spaces */
	if (f1->size > f2->size) {
		if ((ret = cmpc (f1->data + min, ' ', f1->size - min)) != 0) {
			goto end;
		}
	} else {
		if ((ret = -cmpc (f2->data + min, ' ', f2->size - min)) != 0) {
			goto end;
		}
	}

end:
	if (COB_FIELD_TYPE (f1) != COB_TYPE_NUMERIC_PACKED) {
		cob_put_sign (f1, sign1);
	}
	if (COB_FIELD_TYPE (f2) != COB_TYPE_NUMERIC_PACKED) {
		cob_put_sign (f2, sign2);
	}
	return ret;
}

int
cob_cmp (cob_field *f1, cob_field *f2)
{
	if (COB_FIELD_TYPE (f2) == COB_TYPE_ALPHANUMERIC_ALL) {
		if (f2 == &cob_zero && COB_FIELD_IS_NUMERIC (f1)) {
			return cob_cmp_int (f1, 0);
		} else if (f2->size == 1) {
			return cob_cmp_char (f1, f2->data[0]);
		} else {
			return cob_cmp_all (f1, f2);
		}
	} else if (COB_FIELD_TYPE (f1) == COB_TYPE_ALPHANUMERIC_ALL) {
		if (f1 == &cob_zero && COB_FIELD_IS_NUMERIC (f2)) {
			return -cob_cmp_int (f2, 0);
		} else if (f1->size == 1) {
			return -cob_cmp_char (f2, f1->data[0]);
		} else {
			return -cob_cmp_all (f2, f1);
		}
	} else {
		cob_field	temp;
		cob_field_attr	attr;
		unsigned char	buff[48];

		if (COB_FIELD_IS_NUMERIC (f1) && COB_FIELD_IS_NUMERIC (f2)) {
			return cob_numeric_cmp (f1, f2);
		}
		if (COB_FIELD_IS_NUMERIC (f1)
		    && COB_FIELD_TYPE (f1) != COB_TYPE_NUMERIC_DISPLAY) {
/* Seems like struct inits generate worse code
			temp = (cob_field) {f1->attr->digits, buff, &attr};
*/
			temp.size = f1->attr->digits;
			temp.data = buff;
			temp.attr = &attr;
			attr = *f1->attr;
			attr.type = COB_TYPE_NUMERIC_DISPLAY;
			attr.flags &= ~COB_FLAG_HAVE_SIGN;
			cob_move (f1, &temp);
			f1 = &temp;
		}
		if (COB_FIELD_IS_NUMERIC (f2)
		    && COB_FIELD_TYPE (f2) != COB_TYPE_NUMERIC_DISPLAY) {
/* Seems like struct inits generate worse code
			temp = (cob_field) {f2->attr->digits, buff, &attr};
*/
			temp.size = f2->attr->digits;
			temp.data = buff;
			temp.attr = &attr;
			attr = *f2->attr;
			attr.type = COB_TYPE_NUMERIC_DISPLAY;
			attr.flags &= ~COB_FLAG_HAVE_SIGN;
			cob_move (f2, &temp);
			f2 = &temp;
		}
		return cob_cmp_alnum (f1, f2);
	}
}

/*
 * Class check
 */

int
cob_is_numeric (cob_field *f)
{
	switch (COB_FIELD_TYPE (f)) {
	case COB_TYPE_NUMERIC_BINARY:
	case COB_TYPE_NUMERIC_FLOAT:
	case COB_TYPE_NUMERIC_DOUBLE:
		return 1;
	case COB_TYPE_NUMERIC_PACKED:
	{
		size_t	i;
		int	sign;

		/* check digits */
		for (i = 0; i < f->size - 1; i++) {
			if ((f->data[i] & 0xf0) > 0x90 || (f->data[i] & 0x0f) > 0x09) {
				return 0;
			}
		}
		if ((f->data[i] & 0xf0) > 0x90) {
			return 0;
		}
		/* check sign */
		sign = f->data[i] & 0x0f;
		if (sign == 0x0f) {
			return 1;
		}
		if (COB_FIELD_HAVE_SIGN (f)) {
			if (sign == 0x0c || sign == 0x0d) {
				return 1;
			}
		}
		return 0;
	}
	case COB_TYPE_NUMERIC_DISPLAY:
	{
		int		i;
		int		sign = cob_get_sign (f);
		int		size = (int) COB_FIELD_SIZE (f);
		unsigned char	*data = COB_FIELD_DATA (f);

		for (i = 0; i < size; i++) {
			if (!isdigit (data[i])) {
				cob_put_sign (f, sign);
				return 0;
			}
		}
		cob_put_sign (f, sign);
		return 1;
	}
	default:
	{
		size_t	i;

		for (i = 0; i < f->size; i++) {
			if (!isdigit (f->data[i])) {
				return 0;
			}
		}
		return 1;
	}
	}
}

int
cob_is_alpha (cob_field *f)
{
	size_t	i;

	for (i = 0; i < f->size; i++) {
		if (!isspace (f->data[i]) && !isalpha (f->data[i])) {
			return 0;
		}
	}
	return 1;
}

int
cob_is_upper (cob_field *f)
{
	size_t	i;

	for (i = 0; i < f->size; i++) {
		if (!isspace (f->data[i]) && !isupper (f->data[i])) {
			return 0;
		}
	}
	return 1;
}

int
cob_is_lower (cob_field *f)
{
	size_t	i;
	for (i = 0; i < f->size; i++) {
		if (!isspace (f->data[i]) && !islower (f->data[i])) {
			return 0;
		}
	}
	return 1;
}

/*
 * Table sort
 */

static int		sort_nkeys;
static cob_file_key	*sort_keys;
static cob_field	*sort_base;

static int
sort_compare (const void *data1, const void *data2)
{
	int	i, cmp;

	for (i = 0; i < sort_nkeys; i++) {
		cob_field f1 = *sort_keys[i].field;
		cob_field f2 = *sort_keys[i].field;
		f1.data += ((unsigned char *)data1) - sort_base->data;
		f2.data += ((unsigned char *)data2) - sort_base->data;
		cmp = cob_cmp (&f1, &f2);
		if (cmp != 0) {
			return (sort_keys[i].flag == COB_ASCENDING) ? cmp : -cmp;
		}
	}
	return 0;
}

void
cob_table_sort_init (int nkeys, const unsigned char *collating_sequence)
{
	sort_nkeys = 0;
	sort_keys = cob_malloc (nkeys * sizeof (cob_file_key));
	old_sequence = cob_current_module->collating_sequence;
	if (collating_sequence) {
		cob_current_module->collating_sequence = collating_sequence;
	}
}

void
cob_table_sort_init_key (int flag, cob_field *field)
{
	sort_keys[sort_nkeys].flag = flag;
	sort_keys[sort_nkeys].field = field;
	sort_nkeys++;
}

void
cob_table_sort (cob_field *f, int n)
{
	sort_base = f;
	qsort (f->data, (size_t) n, f->size, sort_compare);
	cob_current_module->collating_sequence = old_sequence;
}

/* Runtime error handling */
static struct handlerlist {
	struct handlerlist	*next;
	int			(*proc)(char *s);
} *hdlrs = NULL;

int CBL_ERROR_PROC(char *x, int (**p)(char *s))
{
	struct handlerlist *hp = NULL;
	struct handlerlist *h = hdlrs;

	if (!p || !*p) {
		return -1;
	}
	/* remove handler anyway */
	while (h != NULL) {
		if (h->proc == *p) {
			if (hp != NULL) {
				hp->next = h->next;
			} else {
				hdlrs = h->next;
			}
			free (hp);
			break;
		}
		hp = h;
		h = h->next;
	}
	if (*x != 0) {	/* remove handler */
		return 0;
	}
	h = cob_malloc (sizeof(struct handlerlist));
	h->next = hdlrs;
	h->proc = *p;
	hdlrs = h;
	return 0;
}

/*
 * Run-time error checking
 */

void
cob_runtime_error (const char *fmt, ...)
{
	va_list ap;

	if (hdlrs != NULL) {
		struct handlerlist	*h = hdlrs;
		char			*p;
		char			str[COB_MEDIUM_BUFF];

		p = str;
		if (cob_source_file) {
			sprintf (str, "%s:%d: ", cob_source_file, cob_source_line);
			p = str + strlen(str);
		}
		va_start (ap, fmt);
		vsprintf (p, fmt, ap);
		va_end (ap);
		while (h != NULL) {
			h->proc(str);
			h = h->next;
		}
	}
	/* prefix */
	if (cob_source_file) {
		fprintf (stderr, "%s:%d: ", cob_source_file, cob_source_line);
	}
	fputs ("libcob: ", stderr);

	/* body */
	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end (ap);

	/* postfix */
	fputs ("\n", stderr);
	fflush (stderr);
}

void
cob_check_numeric (cob_field *f, const char *name)
{
	if (!cob_is_numeric (f)) {
		size_t		i;
		unsigned char	*data = f->data;
		char		buff[COB_SMALL_BUFF];
		char		*p = buff;

		for (i = 0; i < f->size; i++) {
			if (isprint (data[i])) {
				*p++ = data[i];
			} else {
				p += sprintf (p, "\\%03o", data[i]);
			}
		}
		*p = '\0';
		cob_runtime_error (_("'%s' not numeric: '%s'"), name, buff);
		cob_stop_run (1);
	}
}

void
cob_check_odo (int i, int min, int max, const char *name)
{
	/* check the OCCURS DEPENDING ON item */
	if (i < min || max < i) {
		COB_SET_EXCEPTION (COB_EC_BOUND_ODO);
		cob_runtime_error (_("OCCURS DEPENDING ON '%s' out of bounds: %d"), name, i);
		cob_stop_run (1);
	}
}

void
cob_check_subscript (int i, int min, int max, const char *name)
{
	/* check the subscript */
	if (i < min || max < i) {
		COB_SET_EXCEPTION (COB_EC_BOUND_SUBSCRIPT);
		cob_runtime_error (_("subscript of '%s' out of bounds: %d"), name, i);
		cob_stop_run (1);
	}
}

void
cob_check_ref_mod (int offset, int length, int size, const char *name)
{
	/* check the offset */
	if (offset < 1 || offset > size) {
		COB_SET_EXCEPTION (COB_EC_BOUND_REF_MOD);
		cob_runtime_error (_("offset of '%s' out of bounds: %d"), name, offset);
		cob_stop_run (1);
	}

	/* check the length */
	if (length < 1 || offset + length - 1 > size) {
		COB_SET_EXCEPTION (COB_EC_BOUND_REF_MOD);
		cob_runtime_error (_("length of '%s' out of bounds: %d"), name, length);
		cob_stop_run (1);
	}
}

unsigned char *
cob_external_addr (const char *exname, int exlength)
{
	static cob_external *basext = NULL;

	cob_external *eptr;

	for (eptr = basext; eptr; eptr = eptr->next) {
		if (!strcmp (exname, eptr->ename)) {
			if (exlength > eptr->esize) {
				cob_runtime_error (_("EXTERNAL item '%s' has size > %d"),
						   exname, exlength);
				cob_stop_run (1);
			}
			cob_initial_external = 0;
			return (ucharptr)eptr->ext_alloc;
		}
	}
	eptr = (cob_external *) cob_malloc (sizeof (cob_external));
	eptr->next = basext;
	eptr->esize = exlength;
	eptr->ename = cob_malloc (strlen (exname) + 1);
	strcpy (eptr->ename, exname);
	eptr->ext_alloc = cob_malloc ((size_t)exlength);
	basext = eptr;
	cob_initial_external = 1;
	return (ucharptr)eptr->ext_alloc;
}

void *
cob_malloc (const size_t size)
{
	void *mptr;

	mptr = malloc (size);
	if (unlikely(!mptr)) {
		cob_runtime_error (_("Cannot acquire %d bytes of memory - Aborting"), size);
		cob_stop_run (1);
	}
	memset (mptr, 0, size);
	return mptr;
}

void *
cob_strdup (const void *stptr)
{
	void	*mptr;
	size_t	len;

	if (unlikely(!stptr)) {
		cob_runtime_error (_("cob_strdup called with NULL pointer"));
		cob_stop_run (1);
	}
	len = strlen (stptr);
	if (unlikely(len < 1 || len > 2147483647)) {
		cob_runtime_error (_("cob_strdup called with invalid string"));
		cob_stop_run (1);
	}
	len++;
	mptr = cob_malloc (len);
	memcpy (mptr, stptr, len);
	return mptr;
}

/* Extended ACCEPT/DISPLAY */

void
cob_accept_date (cob_field *f)
{
	time_t	t = time (NULL);
	char	s[7];

	strftime (s, 7, "%y%m%d", localtime (&t));
	cob_memcpy (f, (ucharptr)s, 6);
}

void
cob_accept_date_yyyymmdd (cob_field *f)
{
	time_t	t = time (NULL);
	char	s[9];

	strftime (s, 9, "%Y%m%d", localtime (&t));
	cob_memcpy (f, (ucharptr)s, 8);
}

void
cob_accept_day (cob_field *f)
{
	time_t	t = time (NULL);
	char	s[6];

	strftime (s, 6, "%y%j", localtime (&t));
	cob_memcpy (f, (ucharptr)s, 5);
}

void
cob_accept_day_yyyyddd (cob_field *f)
{
	time_t	t = time (NULL);
	char	s[8];

	strftime (s, 8, "%Y%j", localtime (&t));
	cob_memcpy (f, (ucharptr)s, 7);
}

void
cob_accept_day_of_week (cob_field *f)
{
	time_t	t = time (NULL);
	char	s[2];

#if defined(_MSC_VER)
	sprintf(s, "%d", localtime(&t)->tm_wday + 1);
#else
	strftime (s, 2, "%u", localtime (&t));
#endif
	cob_memcpy (f, (ucharptr)s, 1);
}

void
cob_accept_time (cob_field *f)
{
	time_t	t = time (NULL);
	char	s[9];

	strftime (s, 9, "%H%M%S00", localtime (&t));
	cob_memcpy (f, (ucharptr)s, 8);
}

void
cob_accept_command_line (cob_field *f)
{
	int	i, size = 0;
	char	buff[COB_LARGE_BUFF] = "";

	for (i = 1; i < cob_argc; i++) {
		int len = (int) strlen (cob_argv[i]);
		if (size + len >= COB_LARGE_BUFF) {
			/* overflow */
			break;
		}
		memcpy (buff + size, cob_argv[i], len);
		size += len;
		buff[size++] = ' ';
	}

	cob_memcpy (f, (ucharptr)buff, size);
}

/*
 * Argument number
 */

static int current_arg = 1;

void
cob_display_arg_number (cob_field *f)
{
	int		n;
	cob_field_attr	attr = { COB_TYPE_NUMERIC_BINARY, 9, 0, 0, 0 };
	cob_field	temp = { 4, (unsigned char *)&n, &attr };

	cob_move (f, &temp);
	if (n < 0 || n >= cob_argc) {
		COB_SET_EXCEPTION (COB_EC_IMP_DISPLAY);
		return;
	}
	current_arg = n;
}

void
cob_accept_arg_number (cob_field *f)
{
	int		n = cob_argc - 1;
	cob_field_attr	attr = { COB_TYPE_NUMERIC_BINARY, 9, 0, 0, 0 };
	cob_field	temp = { 4, (unsigned char *)&n, &attr };

	cob_move (&temp, f);
}

void
cob_accept_arg_value (cob_field *f)
{
	if (current_arg >= cob_argc) {
		COB_SET_EXCEPTION (COB_EC_IMP_ACCEPT);
		return;
	}
	cob_memcpy (f, (ucharptr)cob_argv[current_arg], (int) strlen (cob_argv[current_arg]));
	current_arg++;
}

/*
 * Environment variable
 */

static char *env = NULL;

void
cob_set_environment (cob_field *f1, cob_field *f2)
{
	cob_display_environment (f1);
	cob_display_env_value (f2);
}

void
cob_display_environment (cob_field *f)
{
	if (!env) {
		env = cob_malloc (COB_SMALL_BUFF);
	}
	if (f->size > COB_SMALL_BUFF - 1) {
		COB_SET_EXCEPTION (COB_EC_IMP_DISPLAY);
		return;
	}
	cob_field_to_string (f, env);
}

void
cob_display_env_value (cob_field *f)
{
	char *p;
	char env1[COB_SMALL_BUFF];
	char env2[COB_SMALL_BUFF];

	if (!env) {
		COB_SET_EXCEPTION (COB_EC_IMP_DISPLAY);
		return;
	}
	if (!*env) {
		COB_SET_EXCEPTION (COB_EC_IMP_DISPLAY);
		return;
	}
	cob_field_to_string (f, env2);
	if (strlen (env) + strlen (env2) + 2 > COB_SMALL_BUFF) {
		COB_SET_EXCEPTION (COB_EC_IMP_DISPLAY);
		return;
	}
	strcpy (env1, env);
	strcat (env1, "=");
	strcat (env1, env2);
	p = cob_strdup (env1);
	if (putenv (p) != 0) {
		COB_SET_EXCEPTION (COB_EC_IMP_DISPLAY);
	}
}

void
cob_accept_environment (cob_field *f)
{
	char *p = NULL;

	if (env) {
		p = getenv (env);
	}
	if (!p) {
		COB_SET_EXCEPTION (COB_EC_IMP_ACCEPT);
		p = "";
	}
	cob_memcpy (f, (ucharptr)p, (int) strlen (p));
}

void
cob_chain_setup (void *data, const int parm, const int size)
{
	int	len;

	memset (data, ' ', size);
	if (parm <= cob_argc - 1) {
		len = strlen (cob_argv[parm]);
		if (len <= size) {
			memcpy (data, cob_argv[parm], len);
		} else {
			memcpy (data, cob_argv[parm], size);
		}
	}
}
