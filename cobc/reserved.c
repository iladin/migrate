/*
 * Copyright (C) 2001  Keisuke Nishida
 * Copyright (C) 2000  Rildo Pragana, Alan Cox, Andrew Cameron,
 *		      David Essex, Glen Colbert, Jim Noeth.
 * Copyright (C) 1999  Rildo Pragana, Alan Cox, Andrew Cameron, David Essex.
 * Copyright (C) 1991, 1993  Rildo Pragana.
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "codegen.h"
#include "parser.h"
#include "reserved.h"

#define HASH_SIZE 133

static struct reserved_item {
  const char *name;
  struct reserved_word *word;
  struct reserved_item *next;
} *reserved_table[HASH_SIZE];

static struct reserved_word reserved_words[] = {
  {"ACCEPT", ACCEPT, 0},
  {"ACCESS", ACCESS, 0},
  {"ADD", ADD, 0},
  {"ADDRESS", ADDRESS, 0},
  {"ADVANCING", ADVANCING, 0},
  {"AFTER", AFTER, 0},
  {"ALL", ALL, 0},
  {"ALPHABETIC", ALPHABETIC, 0},
  {"ALPHABETIC-LOWER", ALPHABETIC_LOWER, 0},
  {"ALPHABETIC-UPPER", ALPHABETIC_UPPER, 0},
  {"ALSO", ALSO, 0},
  {"ALTERNATE", ALTERNATE, 0},
  {"AND", AND, 0},
  {"ANY", ANY, 0},
  {"ARE", ARE, 0},
  {"AREA", AREA, 0},
  {"ASCENDING", ASCENDING, 0},
  {"ASSIGN", ASSIGN, 0},
  {"AT", AT, 0},
  {"AUTHOR", AUTHOR, 0},
  {"AUTO", AUTO, 0},
  {"BACKGROUND-COLOR", BACKGROUNDCOLOR, 0},
  {"BEFORE", BEFORE, 0},
  {"BELL", BELL, 0},
  {"BINARY", BINARY, 0},
  {"BLANK", BLANK, 0},
  {"BLINK", BLINK, 0},
  {"BLOCK", BLOCK, 0},
  {"BY", BY, 0},
  {"CALL", CALL, 0},
  {"CANCEL", CANCEL, 0},
  {"CHARACTER", CHARACTER, 0},
  {"CHARACTERS", CHARACTERS, 0},
  {"CLOSE", CLOSE, 0},
  {"COL", COLUMN, 0},
  {"COLUMN", COLUMN, 0},
  {"COM1", PORTNUM, 8},
  {"COM2", PORTNUM, 1},
  {"COM3", PORTNUM, 2},
  {"COM4", PORTNUM, 3},
  {"COMMA", COMMA, 0},
  {"COMMAND-LINE", COMMAND_LINE, 0},
  {"COMMON", COMMON, 0},
  {"COMP", BINARY, 0},
  {"COMP-1", FLOAT_SHORT, 0},
  {"COMP-2", FLOAT_LONG, 0},
  {"COMP-3", PACKED_DECIMAL, 0},
  {"COMP-5", BINARY, 0},
  {"COMPUTATIONAL", BINARY, 0},
  {"COMPUTATIONAL-1", FLOAT_SHORT, 0},
  {"COMPUTATIONAL-2", FLOAT_LONG, 0},
  {"COMPUTATIONAL-3", PACKED_DECIMAL, 0},
  {"COMPUTATIONAL-5", BINARY, 0},
  {"COMPUTE", COMPUTE, 0},
  {"CONFIGURATION", CONFIGURATION, 0},
  {"CONSOLE", CONSOLE, 0},
  {"CONTAINS", CONTAINS, 0},
  {"CONTENT", CONTENT, 0},
  {"CONTINUE", CONTINUE, 0},
  {"CONTROL", CONTROL, 0},
  {"CONTROLS", CONTROL, 1},
  {"CONVERTING", CONVERTING, 0},
  {"CORR", CORRESPONDING, 0},
  {"CORRESPONDING", CORRESPONDING, 0},
  {"COUNT", COUNT, 0},
  {"CURRENCY", CURRENCY, 0},
  {"DATA", DATA, 0},
  {"DATE", DATE, 0},
  {"DATE-COMPILED", DATE_COMPILED, 0},
  {"DATE-WRITTEN", DATE_WRITTEN, 0},
  {"DAY", DAY, 0},
  {"DAY-OF-WEEK", DAY_OF_WEEK, 0},
  {"DECIMAL-POINT", DECIMAL_POINT, 0},
  {"DELETE", DELETE, 0},
  {"DELIMITED", DELIMITED, 0},
  {"DELIMITER", DELIMITER, 0},
  {"DEPENDING", DEPENDING, 0},
  {"DESCENDING", DESCENDING, 0},
  {"DETAIL", DETAIL, 0},
  {"DISK", PORTNUM, 0},
  {"DISPLAY", DISPLAY, 0},
  {"DIVIDE", DIVIDE, 0},
  {"DIVISION", DIVISION, 0},
  {"DOWN", DOWN, 0},
  {"DUPLICATES", DUPLICATES, 0},
  {"DYNAMIC", DYNAMIC, 0},
  {"ELSE", ELSE, 0},
  {"END", END, 0},
  {"END-ADD", END_ADD, 0},
  {"END-CALL", END_CALL, 0},
  {"END-COMPUTE", END_COMPUTE, 0},
  {"END-DELETE", END_DELETE, 0},
  {"END-DIVIDE", END_DIVIDE, 0},
  {"END-EVALUATE", END_EVALUATE, 0},
  {"END-IF", END_IF, 0},
  {"END-MULTIPLY", END_MULTIPLY, 0},
  {"END-PERFORM", END_PERFORM, 0},
  {"END-READ", END_READ, 0},
  {"END-RETURN", END_RETURN, 0},
  {"END-REWRITE", END_REWRITE, 0},
  {"END-SEARCH", END_SEARCH, 0},
  {"END-START", END_START, 0},
  {"END-STRING", END_STRING, 0},
  {"END-SUBTRACT", END_SUBTRACT, 0},
  {"END-UNSTRING", END_UNSTRING, 0},
  {"END-WRITE", END_WRITE, 0},
  {"ENVIRONMENT", ENVIRONMENT_TOK, 0},
  {"ENVIRONMENT-VARIABLE", ENVIRONMENT_VARIABLE, 0},
  {"EOL", EOL_TOK, 0},
  {"EOS", EOS_TOK, 0},
  {"EQUAL", EQUAL, 0},
  {"ERASE", ERASE, 0},
  {"ERROR", TOK_ERROR, 0},
  {"EVALUATE", EVALUATE, 0},
  {"EXCEPTION", EXCEPTION, 0},
  {"EXIT", EXIT, 0},
  {"EXTEND", EXTEND, 0},
  {"EXTERNAL", EXTERNAL, 0},
  {"FALSE", TOK_FALSE, 0},
  {"FD", FD, 0},
  {"FILE", FILEN, 0},
  {"FILE-CONTROL", FILE_CONTROL, 0},
  {"FILE-ID", FILE_ID, 0},
  {"FILLER", FILLER, 0},
  {"FINAL", FINAL, 0},
  {"FIRST", FIRSTTOK, 0},
  {"FLOAT-LONG", FLOAT_LONG, 0},
  {"FLOAT-SHORT", FLOAT_SHORT, 0},
  {"FOOTING", FOOTING, 0},
  {"FOR", FOR, 0},
  {"FOREGROUND-COLOR", FOREGROUNDCOLOR, 0},
  {"FROM", FROM, 0},
  {"FULL", FULL, 0},
  {"FUNCTION", FUNCTION, 0},
  {"GENERATE", GENERATE, 0},
  {"GIVING", GIVING, 0},
  {"GLOBAL", GLOBAL, 0},
  {"GO", GO, 0},
  {"GREATER", GREATER, 0},
  {"HEADING", HEADING, 0},
  {"HIGH-VALUE", HIGH_VALUES, 0},
  {"HIGH-VALUES", HIGH_VALUES, 0},
  {"HIGHLIGHT", HIGHLIGHT, 0},
  {"I-O", I_O, 0},
  {"I-O-CONTROL", I_O_CONTROL, 0},
  {"IDENTIFICATION", IDENTIFICATION_TOK, 0},
  {"IF", IF, 0},
  {"IN", IN, 0},
  {"INDEX", BINARY, 0},
  {"INDEXED", INDEXED, 0},
  {"INITIAL", INITIALTOK, 0},
  {"INITIALIZE", INITIALIZE, 0},
  {"INITIATE", INITIATE, 0},
  {"INKEY", INKEY, 0},
  {"INPUT", INPUT, 0},
  {"INPUT-OUTPUT", INPUT_OUTPUT, 0},
  {"INSPECT", INSPECT, 0},
  {"INSTALLATION", INSTALLATION, 0},
  {"INTO", INTO, 0},
  {"INVALID", INVALID, 0},
  {"IS", IS, 0},
  {"JUST", JUSTIFIED, 0},
  {"JUSTIFIED", JUSTIFIED, 0},
  {"KEY", KEY, 0},
  {"LABEL", LABEL, 0},
  {"LAST", TOKLAST, 0},
  {"LEADING", LEADING, 0},
  {"LEFT", LEFT, 0},
  {"LESS", LESS, 0},
  {"LIMIT", LIMIT, 0},
  {"LINE", LINE, 0},
  {"LINES", LINE, 0},
  {"LINKAGE", LINKAGE, 0},
  {"LOW-VALUE", LOW_VALUES, 0},
  {"LOW-VALUES", LOW_VALUES, 0},
  {"LOWLIGHT", LOWLIGHT, 0},
  {"LPT1", PORTNUM, 4},
  {"LPT2", PORTNUM, 5},
  {"LPT3", PORTNUM, 6},
  {"LPT4", PORTNUM, 7},
  {"MINUS", MINUS, 0},
  {"MODE", MODE, 0},
  {"MOVE", MOVE, 0},
  {"MULTIPLY", MULTIPLY, 0},
  {"NEGATIVE", NEGATIVE, 0},
  {"NEXT", NEXT, 0},
  {"NO", NO, 0},
  {"NO-ECHO", NOECHO, 0},
  {"NOT", NOT, 0},
  {"NULL", TOK_NULL, 0},
  {"NUMBER", NUMBERTOK, 0},
  {"NUMERIC", NUMERIC, 0},
  {"OBJECT-COMPUTER", TOK_OBJECT_COMPUTER, 0},
  {"OCCURS", OCCURS, 0},
  {"OF", OF, 0},
  {"OMITTED", OMITTED, 0},
  {"ON", ON, 0},
  {"OPEN", OPEN, 0},
  {"OPTIONAL", OPTIONAL, 0},
  {"OR", OR, 0},
  {"ORGANIZATION", ORGANIZATION, 0},
  {"OTHER", OTHER, 0},
  {"OUTPUT", OUTPUT, 0},
  {"OVERFLOW", OVERFLOWTK, 0},
  {"PACKED-DECIMAL", PACKED_DECIMAL, 0},
  {"PAGE", PAGE, 0},
  {"PERFORM", PERFORM, 0},
  {"PIC", PICTURE, 0},
  {"PICTURE", PICTURE, 0},
  {"PLUS", PLUS, 0},
  {"POINTER", POINTER, 0},
  {"POSITION", POSITION, 0},
  {"POSITIVE", POSITIVE, 0},
  {"PREV", PREVIOUS, 0},
  {"PREVIOUS", PREVIOUS, 0},
  {"PRINTER", PORTNUM, 4},
  {"PROCEDURE", PROCEDURE_TOK, 0},
  {"PROGRAM", PROGRAM, 0},
  {"PROGRAM-ID", PROGRAM_ID, 0},
  {"QUOTE", QUOTES, 0},
  {"QUOTES", QUOTES, 0},
  {"RANDOM", RANDOM, 0},
  {"RD", RD, 0},
  {"READ", READ, 0},
  {"READY", READY, 0},
  {"RECORD", RECORD, 0},
  {"RECORDS", RECORDS, 0},
  {"REDEFINES", REDEFINES, 0},
  {"REFERENCE", REFERENCE, 0},
  {"RELATIVE", RELATIVE, 0},
  {"RELEASE", RELEASE, 0},
  {"REMAINDER", REMAINDER, 0},
  {"REPLACING", REPLACING, 0},
  {"REPORT", REPORT, 0},
  {"REQUIRED", REQUIRED, 0},
  {"RESET", RESET, 0},
  {"RETURN", RETURN_TOK, 0},
  {"RETURNING", RETURNING, 0},
  {"REVERSE-VIDEO", REVERSEVIDEO, 0},
  {"REWRITE", REWRITE, 0},
  {"RIGHT", RIGHT, 0},
  {"ROUNDED", ROUNDED, 0},
  {"RUN", RUN, 0},
  {"SAME", SAME, 0},
  {"SCREEN", SCREEN, 0},
  {"SD", SD, 0},
  {"SEARCH", SEARCH, 0},
  {"SECTION", SECTION, 0},
  {"SECURE", SECURE, 0},
  {"SECURITY", SECURITY, 0},
  {"SELECT", SELECT, 0},
  {"SENTENCE", SENTENCE, 0},
  {"SEPARATE", SEPARATE, 0},
  {"SEQUENTIAL", SEQUENTIAL, 0},
  {"SET", SET, 0},
  {"SIGN", SIGN, 0},
  {"SIZE", SIZE, 0},
  {"SORT", SORT, 0},
  {"SORT-MERGE", SORT_MERGE, 0},
  {"SOURCE", TOKSOURCE, 0},
  {"SOURCE-COMPUTER", TOK_SOURCE_COMPUTER, 0},
  {"SPACE", SPACES, 0},
  {"SPACES", SPACES, 0},
  {"SPECIAL-NAMES", SPECIAL_NAMES, 0},
  {"STANDARD", STANDARD, 0},
  {"START", START, 0},
  {"STATUS", STATUS, 0},
  {"STD-ERROR", STD_ERROR, 0},
  {"STD-OUTPUT", STD_OUTPUT, 0},
  {"STOP", STOP, 0},
  {"STRING", STRING, 0},
  {"SUBTRACT", SUBTRACT, 0},
  {"SUM", SUM, 0},
  {"SYNC", SYNCHRONIZED, 0},
  {"SYNCHRONIZED", SYNCHRONIZED, 0},
  {"TALLYING", TALLYING, 0},
  {"TERMINATE", TERMINATE, 0},
  {"TEST", TEST, 0},
  {"THAN", THAN, 0},
  {"THEN", THEN, 0},
  {"THROUGH", THRU, 0},
  {"THRU", THRU, 0},
  {"TIME", TIME, 0},
  {"TIMES", TIMES, 0},
  {"TO", TO, 0},
  {"TRACE", TRACE, 0},
  {"TRAILING", TRAILING, 0},
  {"TRUE", TOK_TRUE, 0},
  {"TYPE", TOK_TYPE, 0},
  {"UNDERLINE", UNDERLINE, 0},
  {"UNSTRING", UNSTRING, 0},
  {"UNTIL", UNTIL, 0},
  {"UP", UP, 0},
  {"UPDATE", UPDATE, 0},
  {"UPON", UPON, 0},
  {"USAGE", USAGE, 0},
  {"USING", USING, 0},
  {"VALUE", VALUE, 0},
  {"VALUES", VALUE, 0},
  {"VARYING", VARYING, 0},
  {"WHEN", WHEN, 0},
  {"WITH", WITH, 0},
  {"WORKING-STORAGE", WORKING_STORAGE, 0},
  {"WRITE", WRITE, 0},
  {"ZERO", ZEROS, 0},
  {"ZEROES", ZEROS, 0},
  {"ZEROS", ZEROS, 0},
  {0, 0, 0}
};

static int
hash (const char *s)
{
  int val = 0;
  for (; *s; s++)
    val += toupper (*s);
  return val % HASH_SIZE;
}

struct reserved_word *
lookup_reserved_word (char *name)
{
  struct reserved_item *p;
  for (p = reserved_table[hash (name)]; p; p = p->next)
    if (strcasecmp (name, p->name) == 0)
      return p->word;
  return NULL;
}

void
init_reserved_words (void)
{
  int i;
  struct reserved_item *p;

  for (i = 0; i < HASH_SIZE; i++)
    reserved_table[i] = NULL;

  for (i = 0; reserved_words[i].name != 0; i++)
    {
      const char *name = reserved_words[i].name;
      int val = hash (name);
      p = malloc (sizeof (struct reserved_item));
      p->name = name;
      p->word = &reserved_words[i];
      p->next = reserved_table[val];
      reserved_table[val] = p;
    }
}
