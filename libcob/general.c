/* Genaral functions
 *
 * Copyright (C) 2000  Rildo Pragana, Alan Cox, Andrew Cameron,
 *		      David Essex, Glen Colbert, Jim Noeth.
 * Copyright (C) 1999  Rildo Pragana, Alan Cox, Andrew Cameron, David Essex.
 * Copyright (C) 1991, 1993  Rildo Pragana.
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

#include "_libcob.h"
#include "decimal.h"

char
sign_to_char (int digit)
{
  if (!digit)
    return '{';
  if (digit == 0x80)
    return '}';
  if (digit > 0)
    return 'A' + (char) (digit - 1);
  digit = -digit;
  return 'J' + (char) (digit - 1);
}

int
char_to_sign (char ch)
{
  if (ch == '{')
    return 0;
  if (ch == '}')
    return 0x80;
  if (ch < 'J')
    return (int) (ch - 'A' + 1);
  return (-(int) (ch - 'J' + 1));
}

int
extract_sign (struct fld_desc *f, char *s)
{
  char *tmp;
  int digit;

  if (f->type == 'C')
    {
      digit = f->len / 2;
      return (f->len & 1) ?	/* odd number of digits? */
	(((s[digit] & 0x0f) == 0x0d) ? 1 : 0) :
	(((s[digit] & 0xf0) == 0xd0) ? 1 : 0);
    }
  if (*f->pic != 'S')
    return 0;
  tmp = (f->leading_sign) ? s : s + f->len - 1;
  digit = char_to_sign (*tmp);
  if (digit == 0x80)
    *tmp = '0';
  else if (digit < 0)
    *tmp = '0' - digit;
  else
    {
      *tmp = '0' + digit;
      return 0;
    }
  return 1;
}

void
put_sign (struct fld_desc *f, char *s, int sign)
{
  char *tmp;
  int digit;

  if (f->type == 'C')
    {
      digit = f->len / 2;
      s[digit] = (f->len & 1) ?	/* odd number of digits */
	((s[digit] & 0xf0) | (sign ? 0x0d : 0x0c)) : (sign ? 0xd0 : 0xc0);
      return;
    }
  if (*f->pic != 'S')
    return;
  tmp = (f->leading_sign) ? s : s + f->len - 1;
  digit = *tmp - '0';
  if (sign)
    digit = -digit;
  *tmp = sign_to_char ((sign && digit == 0) ? 0x80 : digit);
}

int
get_index (struct fld_desc *f, char *s)
{
  int index;
  struct fld_desc fld = { 4, 'B', 0, 0, 0, 0, 0, 0, "S9\x9" };
  cob_move (f, s, &fld, (char *) &index);
  return index;
}

struct fld_desc *
cob_adjust_length (struct fld_desc *dep_desc, char *dep_val,
		   int min, int max, struct fld_desc *var_desc,
		   struct fld_desc *item, struct fld_desc *copy)
{
  int itocc = get_index (dep_desc, dep_val);
  if (itocc < min || itocc > max)
    {
      /* should generate exception, for now just a warning */
      fprintf (stderr, "*** Warning: table size out of bounds ");
      fprintf (stderr, "(requested = %d, min = %d, max = %d)\n",
	       itocc, min, max);
      itocc = max;
    }

  memmove (copy, var_desc, sizeof (struct fld_desc));
  copy->len -= (max - itocc) * item->len;
  return copy;
}

int
check_condition (struct fld_desc *f1, char *s1, ...)
{
  int i, len2, len3;
  struct fld_desc *f2, *f3;
  char *s2, *s3;
  int ret = 1;			/* assume wrong */
  va_list args;

  va_start (args, s1);
  f2 = va_arg (args, struct fld_desc *);
  while (f2)
    {
      s2 = va_arg (args, char *);
      f3 = va_arg (args, struct fld_desc *);
      s3 = va_arg (args, char *);

      if (f1->type == '9' || f1->type == 'B')
	{
	  union numeric_type n1, n2, n3;
	  cob_fld_to_decimal (f1, s1, &n1);
	  cob_fld_to_decimal (f2, s2, &n2);
	  cob_fld_to_decimal (f3, s3, &n3);
	  if ((compare_decimal (n2, n1) <= 0)
	      && (compare_decimal (n1, n3) <= 0))
	    {
	      ret = 0;
	      break;
	    }
	}
      else
	{
	  len2 = f2->len;
	  len3 = f3->len;
	  for (i = 0; i < f1->len; i++)
	    {
	      if ((i < len2) && (s1[i] >= s2[i]))
		{
		  if ((i < len3) && (s1[i] <= s3[i]))
		    {
		      va_end (args);
		      return 0;
		    }
		}
	    }
	}
      f2 = va_arg (args, struct fld_desc *);
    }
  va_end (args);
  return ret;
}

/*------------------------------------------------------------------------*\
 |                                                                        |
 |                          compare_all                                   |
 |  return  1 if s1>s2; 0 if s1==s2; -1 if s1<s2                          |
 |                                                                        |
\*------------------------------------------------------------------------*/

int
compare_all (struct fld_desc *f1, unsigned char *s1,
	     struct fld_desc *f2, unsigned char *s2)
{
  int i, j, k, maxi;

  maxi = (f1->len < f2->len) ? f1->len : f2->len;	// min (f1->len, f2->len)
  j = 0;
  k = 0;
  for (i = 0; i < maxi; i++)
    {
      if (s1[j] == s2[k])
	continue;
      if (s1[j] > s2[k])
	return 1;
      if (s1[j] < s2[k])
	return -1;
      j++;
      k++;
      if (f1->all && j >= f1->len)
	j = 0;
      if (f2->all && k >= f2->len)
	k = 0;
    }

  if (f1->len > f2->len)
    while (j < f1->len)
      {
	if (s1[j++] != s2[k++])
	  return 1;
	if (k >= f2->len)
	  k = 0;
      }
  else
    while (k < f2->len)
      {
	if (s2[k++] != s1[j++])
	  return -1;
	if (j >= f1->len)
	  j = 0;
      }
  return 0;
}

/*------------------------------------------------------------------------*\
 |                                                                        |
 |                          compare                                       |
 |  return  1 if s1>s2; 0 if s1==s2; -1 if s1<s2                          |
 |  return  1 if s1>s2; 0 if s1==s2; -1 if s1<s2                          |
 |                                                                        |
\*------------------------------------------------------------------------*/

int
compare (struct fld_desc *f1, char *s1, struct fld_desc *f2, char *s2)
{
  int i, maxi;

  if ((f1->type != '9' && f1->type != 'C' && f1->type != 'B') ||
      (f2->type != '9' && f2->type != 'C' && f2->type != 'B'))
    {				// compare strings
      if (f1->all || f2->all)
	{
	  return (compare_all (f1, s1, f2, s2));
	}
      maxi = (f1->len < f2->len) ? f1->len : f2->len;	// min (f1->len, f2->len)
      for (i = 0; i < maxi; i++)
	{
	  if (s1[i] == s2[i])
	    continue;
	  if (s1[i] > s2[i])
	    return 1;
	  if (s1[i] < s2[i])
	    return -1;
	}
      if (f1->len > f2->len)
	while (i < f1->len)
	  {
	    if (s1[i++] != ' ')
	      return 1;
	  }
      else
	while (i < f2->len)
	  {
	    if (s2[i++] != ' ')
	      return -1;
	  }
    }
  else
    {
      union numeric_type n1, n2;
      cob_fld_to_decimal (f1, s1, &n1);
      cob_fld_to_decimal (f2, s2, &n2);
      return compare_decimal (n1, n2);
    }
  return 0;
}

/*
 * Class check
 */

int
cob_check_numeric (struct fld_desc *f, char *s)
{
  int i, dig = 0;
  char c;

  if ((f->type == 'B') || (f->type == 'C') || (f->type == 'U'))
    return 0;			/* the B and C formats have valid numbers always (?) */
  for (i = 0; i < f->len; i++)
    {
      c = s[i];
      /* must have at least one digit */
      if (!dig && (c >= '0') && (c <= '9'))
	dig++;
      if (i == 0 && ((c == ' ') || (c == '+') || (c == '-')))
	continue;
      /* look for a number followed by several spaces (is this valid?) */
      if (c == ' ')
	{
	  while (i < f->len)
	    {
	      if (s[i] != ' ')
		return 1;
	      i++;
	    }
	  break;
	}
	/******** take care of NULL picture (bug in refmod's) ********/
      if (f->pic != NULL)
	{
	  /* take care of signed numbers (non separate sign) */
	  if ((i == f->len - 1) && (*(f->pic) == 'S'))
	    {
	      if (strchr ("}ABCDEFGHI{JKLMNOPQR", c) != NULL)
		{
		  dig++;
		  break;
		}
	    }
	}
      if ((c > '9') || (c < '0'))
	return 1;
    }
  if (!dig)
    return 1;
  return 0;
}

int
cob_check_alphabetic (struct fld_desc *f, char *s)
{
  int i;
  char c;

  for (i = 0; i < f->len; i++)
    {
      c = s[i];
      if (!
	  ((c == ' ') || ((c >= 'a') && (c <= 'z'))
	   || ((c >= 'A') && (c <= 'Z'))))
	return 1;
    }
  return 0;
}

int
cob_check_upper (struct fld_desc *f, char *s)
{
  int i;
  char c;

  for (i = 0; i < f->len; i++)
    {
      c = s[i];
      if (!((c == ' ') || ((c >= 'A') && (c <= 'Z'))))
	return 1;
    }
  return 0;
}

int
cob_check_lower (struct fld_desc *f, char *s)
{
  int i;
  char c;

  for (i = 0; i < f->len; i++)
    {
      c = s[i];
      if (!((c == ' ') || ((c >= 'a') && (c <= 'z'))))
	return 1;
    }
  return 0;
}


/*
 * 
 */

void
stop_run ()
{
//     do_scrio_finish();
}

int
fldLength (struct fld_desc *f)
{
  switch (f->type)
    {
    case 'B':
      switch (f->len)
	{
	case 1: return 3;
	case 2: return 5;
	case 4: return 10;
	default: return 18;
	}
    case 'U':
      if (f->len == 4)
	return 14;
      else
	return 30;
    default:
      return f->len;
    }
}

int
picCompLength (struct fld_desc *f)
{
  int len = 0, i;
  unsigned char *pic = f->pic;
  for (i = 0; pic[i]; i++)
    {
      if (pic[i] == '9' || pic[i] == 'P')
	len += pic[++i];
    }
  return len;
}
