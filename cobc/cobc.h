/*
 * Copyright (C) 2001-2003 Keisuke Nishida
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 */

#ifndef _COBC_H_
#define _COBC_H_

#include "config.h"

#include <stdio.h>

#define COBC_PACKAGE	"cobc (" PACKAGE_NAME ")"
#define COBC_VERSION	PACKAGE_VERSION
#define COBC_COPYRIGHT	"Copyright (C) 2001-2003 Keisuke Nishida\n"

/* have a main function */
extern int cobc_flag_main;

/* CALL statements are static */
extern int cobc_flag_call_static;

/* enable debugging lines */
extern int cobc_flag_debugging_line;

/* output line directives */
extern int cobc_flag_line_directive;

extern FILE *yyin;
extern FILE *cobc_out;

#undef COBC_WARNING
#define COBC_WARNING(sig,var,name,doc) extern int var;
#include "warning.def"

#endif /* _COBC_H_ */
