/*
 * Copyright (C) 2002 Keisuke Nishida
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

/*
 * ACCESS MODE |         | INPUT  OUTPUT  I-O  EXTEND
 * ------------+---------+---------------------------
 *             | READ    |   o      -      o     -
 *             | WRITE   |   -      o      -     o
 * SEQUENTIAL  | REWRITE |   -      -      o     -
 *             | START   |   o      -      o     -
 *             | DELETE  |   -      -      o     -
 * ------------+---------+---------------------------
 *             | READ    |   o      -      o     -
 *             | WRITE   |   -      o      o     -
 * RANDOM      | REWRITE |   -      -      o     -
 *             | START   |   -      -      -     -
 *             | DELETE  |   -      -      o     -
 * ------------+---------+---------------------------
 *             | READ    |   o      -      o     -
 *             | WRITE   |   -      o      o     -
 * DYNAMIC     | REWRITE |   -      -      o     -
 *             | START   |   o      -      o     -
 *             | DELETE  |   -      -      o     -
 */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "move.h"
#include "numeric.h"
#include "fileio.h"
#include "lib/gettext.h"

#define FILE_PERMISSION 0644

struct cob_file *cob_last_file;
char cob_dummy_status[2];

static struct cob_fileio_funcs *fileio_funcs[COB_ORG_MAX];


/*
 * SEQUENTIAL
 */

static int
sequential_open (struct cob_file *f, char *filename, int mode)
{
  int flags;

  switch (mode)
    {
    case COB_OPEN_INPUT:
      flags = O_RDONLY;
      break;
    case COB_OPEN_OUTPUT:
      flags = O_CREAT | O_RDWR | O_TRUNC;
      break;
    case COB_OPEN_I_O:
      flags = O_RDWR;
      break;
    case COB_OPEN_EXTEND:
      flags = O_RDWR | O_APPEND;
      if (f->f.optional)
	flags |= O_CREAT;
      break;
    }

  f->file.fd = open (filename, flags, FILE_PERMISSION);

  return (f->file.fd > 0) ? 0 : errno;
}

static int
sequential_close (struct cob_file *f, int opt)
{
  switch (opt)
    {
    case COB_CLOSE_NORMAL:
    case COB_CLOSE_LOCK:
      close (f->file.fd);
      f->file.fd = 0;
      return COB_FILE_SUCCEED;
    default:
      return COB_FILE_SUCCEED_NO_REEL;
    }
}

static int
sequential_read (struct cob_file *f)
{
  if (f->record_min != f->record_max)
    {
      if (read (f->file.fd, &f->record_size, sizeof (f->record_size)) <= 0)
	return COB_FILE_END_OF_FILE;
    }

  if (read (f->file.fd, f->record_data, f->record_size) <= 0)
    return COB_FILE_END_OF_FILE;

  return COB_FILE_SUCCEED;
}

static int
sequential_write (struct cob_file *f)
{
  if (f->record_min != f->record_max)
    write (f->file.fd, &f->record_size, sizeof (f->record_size));

  write (f->file.fd, f->record_data, f->record_size);
  return COB_FILE_SUCCEED;
}

static int
sequential_rewrite (struct cob_file *f, struct cob_field rec)
{
  if (rec.size != f->record_size)
    return COB_FILE_RECORD_OVERFLOW;

  if (COB_FIELD_IS_VALID (f->record_depending))
    if (f->record_size != cob_to_int (f->record_depending))
      return COB_FILE_RECORD_OVERFLOW;

  lseek (f->file.fd, - f->record_size, SEEK_CUR);
  write (f->file.fd, f->record_data, f->record_size);
  return COB_FILE_SUCCEED;
}

static struct cob_fileio_funcs sequential_funcs = {
  sequential_open,
  sequential_close,
  0,
  0,
  sequential_read,
  sequential_write,
  sequential_rewrite,
  0
};


/*
 * LINE SEQUENTIAL
 */

static int
lineseq_open (struct cob_file *f, char *filename, int mode)
{
  switch (mode)
    {
    case COB_OPEN_INPUT:
      f->file.fp = fopen (filename, "r");
      break;
    case COB_OPEN_OUTPUT:
      f->file.fp = fopen (filename, "w");
      break;
    case COB_OPEN_I_O:
      f->file.fp = fopen (filename, "rw");
      break;
    case COB_OPEN_EXTEND:
      f->file.fp = fopen (filename, "w+");
      break;
    }

  return (f->file.fp != NULL) ? 0 : errno;
}

static int
lineseq_close (struct cob_file *f, int opt)
{
  fclose (f->file.fp);
  f->file.fp = NULL;
  return COB_FILE_SUCCEED;
}

static int
lineseq_read (struct cob_file *f)
{
  int i;
  char buff[f->record_size + 1];

  /* read the file */
  if (fgets (buff, f->record_size + 1, f->file.fp) == NULL)
    return COB_FILE_END_OF_FILE;

  /* remove the newline */
  for (i = 0; i < f->record_size; i++)
    if (buff[i] == '\r' || buff[i] == '\n' || buff[i] == '\0')
      break;
  if (i < f->record_size)
    {
      /* replace the inline newline by spaces */
      for (; i < f->record_size; i++)
	buff[i] = ' ';
    }
  else
    {
      /* discard input until the next newline */
      int c = getc (f->file.fp);
      while (c != '\r' && c != '\n' && c != '\0' && c != EOF)
	c = getc (f->file.fp);
      if (c == '\r')
	c = getc (f->file.fp);
      if (c != '\n' && c != '\0')
	ungetc (c, f->file.fp);
    }

  memcpy (f->record_data, buff, f->record_size);

  return COB_FILE_SUCCEED;
}

static int
lineseq_write (struct cob_file *f)
{
  int i, size;

  /* determine the size to be written */
  for (i = f->record_size - 1; i >= 0; i--)
    if (f->record_data[i] != ' ')
      break;
  size = i + 1;

  /* write to the file */
  for (i = 0; i < size; i++)
    putc (f->record_data[i], f->file.fp);
  putc ('\n', f->file.fp);

  return COB_FILE_SUCCEED;
}

static struct cob_fileio_funcs lineseq_funcs = {
  lineseq_open,
  lineseq_close,
  0,
  0,
  lineseq_read,
  lineseq_write,
  0,
  0
};


/*
 * RELATIVE
 */

#define RELATIVE_SIZE(f) (f->record_max + sizeof (f->record_size))

#define RELATIVE_SEEK(f,i)						   \
  if (lseek (f->file.fd, RELATIVE_SIZE (f) * (i), SEEK_SET) == -1	   \
      || read (f->file.fd, &f->record_size, sizeof (f->record_size)) <= 0) \
    return COB_FILE_KEY_NOT_EXISTS;					   \
  lseek (f->file.fd, - sizeof (f->record_size), SEEK_CUR);

static int
relative_open (struct cob_file *f, char *filename, int mode)
{
  return sequential_open (f, filename, mode);
}

static int
relative_close (struct cob_file *f, int opt)
{
  return sequential_close (f, opt);
}

static int
relative_start (struct cob_file *f, int cond, struct cob_field k)
{
  /* get the index */
  int index = cob_to_int (k) - 1;
  if (cond == COB_LT)
    index--;
  else if (cond == COB_GT)
    index++;

  /* seek the index */
  while (1)
    {
      RELATIVE_SEEK (f, index);

      /* check if a valid record */
      if (f->record_size > 0)
	{
	  cob_set_int (k, index + 1);
	  return COB_FILE_SUCCEED;
	}

      /* continue */
      switch (cond)
	{
	case COB_EQ:
	  return COB_FILE_KEY_NOT_EXISTS;
	case COB_LT:
	case COB_LE:
	  index--;
	  break;
	case COB_GT:
	case COB_GE:
	  index++;
	  break;
	}
    }
}

static int
relative_read (struct cob_file *f, struct cob_field k)
{
  RELATIVE_SEEK (f, cob_to_int (k) - 1);

  if (f->record_size == 0)
    return COB_FILE_KEY_NOT_EXISTS;

  lseek (f->file.fd, sizeof (f->record_size), SEEK_CUR);
  read (f->file.fd, f->record_data, f->record_max);
  return COB_FILE_SUCCEED;
}

static int
relative_read_next (struct cob_file *f)
{
  while (1)
    {
      if (read (f->file.fd, &f->record_size, sizeof (f->record_size)) <= 0)
	return COB_FILE_END_OF_FILE;

      if (COB_FIELD_IS_VALID (f->relative_key))
	{
	  if (f->f.first_read)
	    {
	      cob_set_int (f->relative_key, 1);
	      f->f.first_read = 0;
	    }
	  else
	    cob_add_int (f->relative_key, 1, 0, 0);
	  if (cob_status != 0)
	    {
	      lseek (f->file.fd, - sizeof (f->record_size), SEEK_CUR);
	      return 14;
	    }
	}

      if (f->record_size > 0)
	{
	  read (f->file.fd, f->record_data, f->record_max);
	  return COB_FILE_SUCCEED;
	}

      lseek (f->file.fd, f->record_max, SEEK_CUR);
    }
}

static int
relative_write (struct cob_file *f)
{
  size_t size;

  if (f->access_mode != COB_ACCESS_SEQUENTIAL)
    {
      int index = cob_to_int (f->relative_key) - 1;
      lseek (f->file.fd, RELATIVE_SIZE (f) * index, SEEK_SET);
    }

  if (read (f->file.fd, &size, sizeof (size)) > 0)
    {
      lseek (f->file.fd, - sizeof (size), SEEK_CUR);
      if (size > 0)
	return COB_FILE_KEY_EXISTS;
    }

  write (f->file.fd, &f->record_size, sizeof (f->record_size));
  write (f->file.fd, f->record_data, f->record_max);

  /* update RELATIVE KEY */
  if (f->access_mode == COB_ACCESS_SEQUENTIAL)
    if (COB_FIELD_IS_VALID (f->relative_key))
      cob_set_int (f->relative_key,
		   lseek (f->file.fd, 0, SEEK_CUR) / RELATIVE_SIZE (f));

  return COB_FILE_SUCCEED;
}

static int
relative_rewrite (struct cob_file *f, struct cob_field rec)
{
  lseek (f->file.fd, - f->record_max, SEEK_CUR);
  write (f->file.fd, f->record_data, f->record_max);
  return COB_FILE_SUCCEED;
}

static int
relative_delete (struct cob_file *f)
{
  RELATIVE_SEEK (f, cob_to_int (f->relative_key) - 1);

  f->record_size = 0;
  write (f->file.fd, &f->record_size, sizeof (f->record_size));
  lseek (f->file.fd, f->record_max, SEEK_CUR);
  return COB_FILE_SUCCEED;
}

static struct cob_fileio_funcs relative_funcs = {
  relative_open,
  relative_close,
  relative_start,
  relative_read,
  relative_read_next,
  relative_write,
  relative_rewrite,
  relative_delete
};


/*
 * INDEXED
 */

#define DB_PUT(db,key,data,flags)				\
  db->put (db, NULL, key, data, flags)
#define DB_GET(db,key,data,flags)				\
  db->get (db, NULL, key, data, flags)
#define DB_DEL(db,key,flags)					\
  db->del (db, NULL, key, flags)
#if DB_VERSION_MAJOR == 2
#define DB_CURSOR(db,cur)					\
  db->cursor (db, NULL, cur)
#else
#define DB_CURSOR(db,cur)					\
  db->cursor (db, NULL, cur, 0)
#endif
#define DBC_GET(cur,key,data,flags)			\
  ({							\
    int ret = cur->c_get (cur, key, data, flags);	\
    if (ret == 0 && f->f.secondary)			\
      {							\
	*key = *data;					\
	ret = DB_GET (f->file.db, key, data, 0);	\
      }							\
    ret;						\
  })

#define DBT_SET(k,fld)				\
  k.data = fld.data;				\
  k.size = fld.size;

static DB_ENV *dbenv = NULL;

static int
indexed_open (struct cob_file *f, char *filename, int mode)
{
  int i, j, ret;
  int flags = 0;

  switch (mode)
    {
    case COB_OPEN_INPUT:
      flags = DB_RDONLY;
      break;
    case COB_OPEN_OUTPUT:
      flags = DB_CREATE | DB_TRUNCATE;
      break;
    case COB_OPEN_I_O:
      flags = DB_CREATE;
      break;
    case COB_OPEN_EXTEND:
      flags = 0;
      if (f->f.optional)
	flags = DB_CREATE;
      break;
    }

  for (i = 0; i < f->nkeys; i++)
    {
      char dbname[BUFSIZ];
      if (i == 0)
	strcpy (dbname, filename);
      else
	sprintf (dbname, "%s.%d", filename, i);
#if DB_VERSION_MAJOR == 2
      {
	DB_INFO info;
	memset (&info, 0, sizeof (info));
	if (f->keys[i].duplicates)
	  info.flags = DB_DUP;
	ret = db_open (dbname, DB_BTREE, flags, FILE_PERMISSION,
		       dbenv, &info, &f->keys[i].db);
      }
#else
      {
	DB *dbp;
	db_create (&f->keys[i].db, dbenv, 0);
	dbp = f->keys[i].db;
	if (f->keys[i].duplicates)
	  dbp->set_flags (dbp, DB_DUP);
	ret = dbp->open (dbp, dbname, NULL, DB_BTREE, flags, FILE_PERMISSION);
      }
#endif
      if (ret != 0)
	{
	  for (j = 0; j < i; j++)
	    f->keys[j].db->close (f->keys[j].db, 0);
	  return ret;
	}
    }

  f->file.db = f->keys[0].db;
  f->last_key = NULL;
  return 0;
}

static int
indexed_close (struct cob_file *f, int opt)
{
  int i;

  /* close the cursor */
  if (f->cursor)
    f->cursor->c_close (f->cursor);
  f->cursor = NULL;

  /* close DB's */
  for (i = 0; i < f->nkeys; i++)
    f->keys[i].db->close (f->keys[i].db, 0);
  f->file.db = NULL;

  if (f->last_key)
    free (f->last_key);

  return COB_FILE_SUCCEED;
}

static int
indexed_start (struct cob_file *f, int cond, struct cob_field k)
{
  int i, ret;
  DBT key, data;

  memset (&key, 0, sizeof (DBT));
  memset (&data, 0, sizeof (DBT));

  /* look up for the key */
  for (i = 0; i < f->nkeys; i++)
    if (f->keys[i].field.data == k.data)
      break;
#if COB_DEBUG
  if (i == f->nkeys)
    {
      cob_runtime_error ("cob_start_indexed: key not found "
			 "(should have been detected by cobc)");
      return 99;
    }
#endif

  /* close the current cursor */
  if (f->cursor)
    f->cursor->c_close (f->cursor);

  /* create a new cursor */
  DB_CURSOR (f->keys[i].db, &f->cursor);
  f->f.secondary = (i > 0);

  /* search */
  DBT_SET (key, k);
  switch (cond)
    {
    case COB_EQ:
      if (k.size == f->keys[i].field.size)
	ret = f->cursor->c_get (f->cursor, &key, &data, DB_SET);
      else
	{
	  ret = f->cursor->c_get (f->cursor, &key, &data, DB_SET_RANGE);
	  if (ret == 0)
	    ret = memcmp (key.data, k.data, k.size);
	}
      break;
    case COB_LT:
    case COB_LE:
      ret = f->cursor->c_get (f->cursor, &key, &data, DB_SET_RANGE);
      if (ret != 0)
	ret = f->cursor->c_get (f->cursor, &key, &data, DB_LAST);
      else if (cond == COB_LT
	       || memcmp (key.data, k.data, k.size) != 0)
	ret = f->cursor->c_get (f->cursor, &key, &data, DB_PREV);
      break;
    case COB_GT:
    case COB_GE:
      ret = f->cursor->c_get (f->cursor, &key, &data, DB_SET_RANGE);
      if (cond == COB_GT)
	while (ret == 0 && memcmp (key.data, k.data, k.size) == 0)
	  ret = f->cursor->c_get (f->cursor, &key, &data, DB_NEXT);
      break;
  }

  return (ret == 0) ? COB_FILE_SUCCEED : COB_FILE_KEY_NOT_EXISTS;
}

static int
indexed_read (struct cob_file *f, struct cob_field k)
{
  int ret;
  DBT key, data;

  memset (&key, 0, sizeof (DBT));
  memset (&data, 0, sizeof (DBT));

  if ((ret = indexed_start (f, COB_EQ, k)) != COB_FILE_SUCCEED)
    return ret;

  if (DBC_GET (f->cursor, &key, &data, DB_CURRENT) != 0)
    return COB_FILE_KEY_NOT_EXISTS;

  f->record_size = data.size;
  memcpy (f->record_data, data.data, data.size);

  return COB_FILE_SUCCEED;
}

static int
indexed_read_next (struct cob_file *f)
{
  int ret;
  DBT key, data;

  memset (&key, 0, sizeof (DBT));
  memset (&data, 0, sizeof (DBT));

  if (!f->f.first_read)
    {
      ret = DBC_GET (f->cursor, &key, &data, DB_NEXT);
    }
  else if (f->cursor)
    {
      ret = DBC_GET (f->cursor, &key, &data, DB_CURRENT);
    }
  else
    {
      DB_CURSOR (f->keys[0].db, &f->cursor);
      f->f.secondary = 0;
      ret = DBC_GET (f->cursor, &key, &data, DB_FIRST);
    }

  switch (ret)
    {
    case 0:
      f->record_size = data.size;
      memcpy (f->record_data, data.data, data.size);
      return COB_FILE_SUCCEED;
    case DB_NOTFOUND:
      return COB_FILE_END_OF_FILE;
    default:
      return 99;
    }
}

static int
indexed_write_internal (struct cob_file *f, DBT key)
{
  int i;
  DBT data;

  memset (&data, 0, sizeof (DBT));

  /* write data */
  data.data = f->record_data;
  data.size = f->record_size;
  if (DB_PUT (f->keys[0].db, &key, &data, DB_NOOVERWRITE) != 0)
    return COB_FILE_KEY_EXISTS;

  /* write secondary keys */
  data = key;
  for (i = 1; i < f->nkeys; i++)
    {
      DBT_SET (key, f->keys[i].field);
      if (DB_PUT (f->keys[i].db, &key, &data,
		  f->keys[i].duplicates ? 0 : DB_NOOVERWRITE) != 0)
	return COB_FILE_KEY_EXISTS;
    }

  return COB_FILE_SUCCEED;
}

static int
indexed_write (struct cob_file *f)
{
  DBT key;

  memset (&key, 0, sizeof (DBT));

  /* check record key */
  DBT_SET (key, f->keys[0].field);
  if (!f->last_key)
    f->last_key = malloc (key.size);
  else if (f->access_mode == COB_ACCESS_SEQUENTIAL
	   && memcmp (f->last_key, key.data, key.size) > 0)
    return COB_FILE_KEY_INVALID;
  memcpy (f->last_key, key.data, key.size);

  return indexed_write_internal (f, key);
}

static int
indexed_delete (struct cob_file *f)
{
  int i, offset;
  DBT key, data;

  memset (&key, 0, sizeof (DBT));
  memset (&data, 0, sizeof (DBT));

  /* find the primary key */
  switch (f->access_mode)
    {
    case COB_ACCESS_SEQUENTIAL:
      if (DBC_GET (f->cursor, &key, &data, DB_CURRENT) != 0)
	return COB_FILE_READ_NOT_DONE;
      break;
    case COB_ACCESS_RANDOM:
    case COB_ACCESS_DYNAMIC:
      DBT_SET (key, f->keys[0].field);
      if (DB_GET (f->file.db, &key, &data, 0) != 0)
	return COB_FILE_KEY_NOT_EXISTS;
      break;
    }

  /* delete the secondary keys */
  offset = data.data - (void *) f->record_data;
  for (i = 1; i < f->nkeys; i++)
    {
      DBT skey, dkey, pkey;
      memset (&skey, 0, sizeof (DBT));
      memset (&dkey, 0, sizeof (DBT));
      memset (&pkey, 0, sizeof (DBT));
      DBT_SET (skey, f->keys[i].field);
      skey.data += offset;
      if (f->keys[i].duplicates)
	{
	  DBC *cursor;
	  DB_CURSOR (f->keys[i].db, &cursor);
	  if (cursor->c_get (cursor, &skey, &pkey, DB_SET) == 0)
	    {
	      do {
		if (memcmp (pkey.data, key.data, key.size) == 0)
		  cursor->c_del (cursor, 0);
	      }
#if DB_VERSION_MAJOR == 2
	      while (cursor->c_get (cursor, &dkey, &pkey, DB_NEXT) == 0
		     && skey.size == dkey.size
		     && memcmp (dkey.data, skey.data, skey.size) == 0);
#else
	      while (cursor->c_get (cursor, &dkey, &pkey, DB_NEXT_DUP) == 0);
#endif
	    }
	  cursor->c_close (cursor);
	}
      else
	{
	  DB_DEL (f->keys[i].db, &skey, 0);
	}
    }

  /* delete the record */
  DB_DEL (f->file.db, &key, 0);

  return COB_FILE_SUCCEED;
}

static int
indexed_rewrite (struct cob_file *f, struct cob_field rec)
{
  int ret;
  DBT key;

  memset (&key, 0, sizeof (DBT));

  /* delete the current record */
  if ((ret = indexed_delete (f)) != COB_FILE_SUCCEED)
    return ret;

  /* write data */
  DBT_SET (key, f->keys[0].field);
  return indexed_write_internal (f, key);
}

static struct cob_fileio_funcs indexed_funcs = {
  indexed_open,
  indexed_close,
  indexed_start,
  indexed_read,
  indexed_read_next,
  indexed_write,
  indexed_rewrite,
  indexed_delete
};


/*
 * SORT
 */

static struct cob_file *sort_file;

static int
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR == 2)
sort_compare (DB *db, const DBT *k1, const DBT *k2)
#else
sort_compare (const DBT *k1, const DBT *k2)
#endif
{
  int i, cmp;
  struct cob_file *f = sort_file;
  for (i = 0; i < f->sort_nkeys; i++)
    {
      struct cob_field f1 = f->sort_keys[i].field;
      struct cob_field f2 = f->sort_keys[i].field;
      f1.data += ((unsigned char *) k1->data) - f->record_data;
      f2.data += ((unsigned char *) k2->data) - f->record_data;
      if (f1.desc &&
	  (f1.desc->type == COB_DISPLAY || f1.desc->type == COB_BINARY))
	{
	  cob_decimal_set_field (cob_d1, f1);
	  cob_decimal_set_field (cob_d2, f2);
	  cmp = cob_decimal_cmp (cob_d1, cob_d2);
	}
      else
	cmp = cob_cmp_field (f1, f2);
      if (cmp != 0)
	return (f->sort_keys[i].dir == COB_ASCENDING) ? cmp : - cmp;
    }
  return 0;
}

static int
sort_open (struct cob_file *f, char *filename, int mode)
{
  int flags = 0;
  DB *dbp;

  switch (mode)
    {
    case COB_OPEN_INPUT:
      flags = DB_RDONLY;
      break;
    case COB_OPEN_OUTPUT:
      flags = DB_CREATE | DB_TRUNCATE;
      break;
    case COB_OPEN_I_O:
    case COB_OPEN_EXTEND:
      abort ();
    }

  db_create (&f->file.db, dbenv, 0);
  dbp = f->file.db;
  dbp->set_flags (dbp, DB_DUP);
  dbp->set_bt_compare (dbp, sort_compare);
  return dbp->open (dbp, filename, NULL, DB_BTREE, flags, FILE_PERMISSION);
}

static int
sort_close (struct cob_file *f, int opt)
{
  f->file.db->close (f->file.db, 0);
  f->file.db = NULL;

  return COB_FILE_SUCCEED;
}

static int
sort_read (struct cob_file *f)
{
  int ret;
  DBT key, data;

  memset (&key, 0, sizeof (DBT));
  memset (&data, 0, sizeof (DBT));

  if (f->f.first_read)
    {
      DB_CURSOR (f->file.db, &f->cursor);
      ret = DBC_GET (f->cursor, &key, &data, DB_FIRST);
    }
  else
    {
      ret = DBC_GET (f->cursor, &key, &data, DB_NEXT);
    }

  switch (ret)
    {
    case 0:
      memcpy (f->record_data, key.data, key.size);
      return COB_FILE_SUCCEED;
    case DB_NOTFOUND:
      return COB_FILE_END_OF_FILE;
    default:
      return 99;
    }
}

static int
sort_write (struct cob_file *f)
{
  DBT key, data;

  memset (&key, 0, sizeof (DBT));
  memset (&data, 0, sizeof (DBT));

  sort_file = f;
  key.data = f->record_data;
  key.size = f->record_size;
  DB_PUT (f->file.db, &key, &data, 0);

  return COB_FILE_SUCCEED;
}

static struct cob_fileio_funcs sort_funcs = {
  sort_open,
  sort_close,
  0,
  0,
  sort_read,
  sort_write,
  0,
  0
};


/*
 * Public interface
 */

#define FILE_OPENED(f)		(f->file.fd != 0)

#define FILE_READABLE(f)			\
  (f->open_mode == COB_OPEN_INPUT		\
   || f->open_mode == COB_OPEN_I_O)

#define FILE_WRITABLE(f)			\
  (f->open_mode == COB_OPEN_OUTPUT		\
   || f->open_mode == COB_OPEN_I_O)

#define FILE_EXTENSIBLE(f)			\
  (f->open_mode == COB_OPEN_OUTPUT		\
   || f->open_mode == COB_OPEN_EXTEND)

#define RETURN_STATUS(x)			\
  do {						\
    f->file_status[0] = x / 10 + '0';		\
    f->file_status[1] = x % 10 + '0';		\
    cob_last_file = f;				\
    return;					\
  } while (0)

void
cob_open (struct cob_file *f, int mode)
{
  int was_not_exist = 0;
  char filename[FILENAME_MAX];
  struct stat st;

  f->f.read_done = 0;

  /* check if the file is already open */
  if (FILE_OPENED (f))
    RETURN_STATUS (COB_FILE_ALREADY_OPEN);

  cob_field_to_string (f->assign, filename);
  if ((mode == COB_OPEN_I_O || mode == COB_OPEN_EXTEND)
      && f->f.optional != 0
      && stat (filename, &st) == -1)
    was_not_exist = 1;

  f->open_mode = mode;
  f->f.nonexistent = 0;
  f->f.end_of_file = 0;
  f->f.first_read = 1;

  switch (fileio_funcs[f->organization]->open (f, filename, mode))
    {
    case 0:
      if (was_not_exist)
	RETURN_STATUS (COB_FILE_SUCCEED_OPTIONAL);
      else
	RETURN_STATUS (COB_FILE_SUCCEED);
    case ENOENT:
      if (f->f.optional)
	{
	  f->f.nonexistent = 1;
	  f->f.end_of_file = 1;
	  RETURN_STATUS (COB_FILE_SUCCEED_OPTIONAL);
	}
      else
	{
	  RETURN_STATUS (COB_FILE_NOT_EXISTS);
	}
    case EACCES:
    case EISDIR:
    case EROFS:
      RETURN_STATUS (COB_FILE_PERMISSION_DENIED);
    default:
      RETURN_STATUS (COB_FILE_PERMANENT_ERROR);
    }
}

void
cob_close (struct cob_file *f, int opt)
{
  int ret;

  f->f.read_done = 0;

  if (f->f.nonexistent)
    RETURN_STATUS (COB_FILE_SUCCEED);

  if (!FILE_OPENED (f))
    RETURN_STATUS (COB_FILE_NOT_OPEN);

  ret = fileio_funcs[f->organization]->close (f, opt);

  f->f.nonexistent = 0;
  f->f.end_of_file = 0;

  RETURN_STATUS (ret);
}

void
cob_start (struct cob_file *f, int cond, struct cob_field key)
{
  int ret;

  f->f.read_done = 0;
  f->f.first_read = 0;

  if (f->f.nonexistent)
    RETURN_STATUS (COB_FILE_KEY_NOT_EXISTS);

  if (f->access_mode == COB_ACCESS_RANDOM)
    RETURN_STATUS (COB_FILE_INPUT_DENIED);

  if (!FILE_OPENED (f) || !FILE_READABLE (f))
    RETURN_STATUS (COB_FILE_INPUT_DENIED);

  ret = fileio_funcs[f->organization]->start (f, cond, key);
  if (ret == COB_FILE_SUCCEED)
    f->f.first_read = 1;

  RETURN_STATUS (ret);
}

static void
read_common (struct cob_file *f, struct cob_field key)
{
  int ret;

  f->f.read_done = 0;

  if (f->f.nonexistent)
    {
      if (!f->f.first_read)
	RETURN_STATUS (COB_FILE_KEY_NOT_EXISTS);
      f->f.first_read = 0;
      RETURN_STATUS (COB_FILE_END_OF_FILE);
    }

  if (f->f.end_of_file)
    RETURN_STATUS (COB_FILE_READ_ERROR);

  if (!FILE_OPENED (f) || !FILE_READABLE (f))
    RETURN_STATUS (COB_FILE_INPUT_DENIED);

  if (COB_FIELD_IS_VALID (key))
    ret = fileio_funcs[f->organization]->read (f, key);
  else
    ret = fileio_funcs[f->organization]->read_next (f);

  switch (ret)
    {
    case COB_FILE_SUCCEED:
      f->f.first_read = 0;
      f->f.read_done = 1;
      if (COB_FIELD_IS_VALID (f->record_depending))
	cob_set_int (f->record_depending, f->record_size);
      break;
    case COB_FILE_END_OF_FILE:
      f->f.end_of_file = 1;
      break;
    }

  RETURN_STATUS (ret);
}

void
cob_read (struct cob_file *f, struct cob_field key)
{
  read_common (f, key);
}

void
cob_read_next (struct cob_file *f)
{
  read_common (f, (struct cob_field) {0, 0});
}

void
cob_write (struct cob_file *f, struct cob_field rec)
{
  int ret;

  f->f.read_done = 0;

  if (f->access_mode == COB_ACCESS_SEQUENTIAL)
    {
      if (!FILE_OPENED (f) || !FILE_EXTENSIBLE (f))
	RETURN_STATUS (COB_FILE_OUTPUT_DENIED);
    }
  else
    {
      if (!FILE_OPENED (f) || !FILE_WRITABLE (f))
	RETURN_STATUS (COB_FILE_OUTPUT_DENIED);
    }

  if (COB_FIELD_IS_VALID (f->record_depending))
    f->record_size = cob_to_int (f->record_depending);
  else
    f->record_size = rec.size;

  if (f->record_size < f->record_min || f->record_max < f->record_size)
    RETURN_STATUS (COB_FILE_RECORD_OVERFLOW);

  ret = fileio_funcs[f->organization]->write (f);

  RETURN_STATUS (ret);
}

void
cob_write_page (struct cob_file *f)
{
  if (!FILE_OPENED (f) || !FILE_EXTENSIBLE (f))
    return;

  write (f->file.fd, "\f", 1);
}

void
cob_write_lines (struct cob_file *f, int lines)
{
  int i;

  if (!FILE_OPENED (f) || !FILE_EXTENSIBLE (f))
    return;

  for (i = 0; i < lines; i++)
    write (f->file.fd, "\n", 1);
}

void
cob_rewrite (struct cob_file *f, struct cob_field rec)
{
  int ret;
  int read_done = f->f.read_done;

  f->f.read_done = 0;

  if (!FILE_OPENED (f) || f->open_mode != COB_OPEN_I_O)
    RETURN_STATUS (COB_FILE_I_O_DENIED);

  if (!read_done)
    RETURN_STATUS (COB_FILE_READ_NOT_DONE);

  ret = fileio_funcs[f->organization]->rewrite (f, rec);

  RETURN_STATUS (ret);
}

void
cob_delete (struct cob_file *f)
{
  int ret;

  f->f.read_done = 0;

  if (!FILE_OPENED (f) || f->open_mode != COB_OPEN_I_O)
    RETURN_STATUS (COB_FILE_I_O_DENIED);

  ret = fileio_funcs[f->organization]->delete (f);

  RETURN_STATUS (ret);
}

void
cob_sort_keys (struct cob_file *f, ...)
{
  int tag;
  size_t size = 0;
  va_list ap;

  f->sort_nkeys = 0;
  va_start (ap, f);
  while ((tag = va_arg (ap, int)) != 0)
    {
      if (size < f->sort_nkeys + 1)
	{
	  size = (size == 0) ? 4 : size * 2;
	  f->sort_keys = realloc (f->sort_keys,
				  size * sizeof (struct cob_sort_key));
	}
      f->sort_keys[f->sort_nkeys].dir = tag;
      f->sort_keys[f->sort_nkeys].field = va_arg (ap, struct cob_field);
      f->sort_nkeys++;
    }
  va_end (ap);
}

void
cob_sort_using (struct cob_file *sort_file, struct cob_file *data_file)
{
  cob_open (data_file, COB_OPEN_INPUT);
  do {
    cob_read_next (data_file);
    if (data_file->file_status[0] != '0')
      break;
    memcpy (sort_file->record_data, data_file->record_data,
	    sort_file->record_size);
    cob_write (sort_file, (struct cob_field) {sort_file->record_size, sort_file->record_data, 0});
  } while (1);
  cob_close (data_file, COB_CLOSE_NORMAL);
}

void
cob_sort_giving (struct cob_file *sort_file, struct cob_file *data_file)
{
  cob_open (data_file, COB_OPEN_OUTPUT);
  do {
    cob_read_next (sort_file);
    if (sort_file->file_status[0] != '0')
      break;
    memcpy (data_file->record_data,
	    sort_file->record_data,
	    data_file->record_size);
    cob_write (data_file, (struct cob_field) {data_file->record_size, data_file->record_data, 0});
  } while (1);
  cob_close (data_file, COB_CLOSE_NORMAL);
}

void
cob_default_error_handle (struct cob_file *f)
{
  const char *msg = NULL;
  int status = (f->file_status[0] - '0') * 10 + (f->file_status[1] - '0');
  switch (status)
    {
    case COB_FILE_END_OF_FILE:
      msg = N_("end of file");
      break;
    case COB_FILE_KEY_INVALID:
      msg = N_("key order not ascending");
      break;
    case COB_FILE_KEY_EXISTS:
      msg = N_("record key already exists");
      break;
    case COB_FILE_KEY_NOT_EXISTS:
      msg = N_("record key not exists");
      break;
    case COB_FILE_PERMANENT_ERROR:
      msg = N_("permanent file error");
      break;
    case COB_FILE_NOT_EXISTS:
      /* no message */
      break;
    case COB_FILE_PERMISSION_DENIED:
      msg = N_("permission denied");
      break;
    case COB_FILE_ALREADY_OPEN:
      msg = N_("file already open");
      break;
    case COB_FILE_NOT_OPEN:
      msg = N_("file not open");
      break;
    case COB_FILE_READ_NOT_DONE:
      msg = N_("READ must be executed first");
      break;
    case COB_FILE_RECORD_OVERFLOW:
      msg = N_("record overflow");
      break;
    case COB_FILE_READ_ERROR:
      msg = N_("failed to read");
      break;
    case COB_FILE_INPUT_DENIED:
      msg = N_("READ and START not allowed");
      break;
    case COB_FILE_OUTPUT_DENIED:
      msg = N_("WRITE not allowed");
      break;
    case COB_FILE_I_O_DENIED:
      msg = N_("DELETE and REWRITE not allowed");
      break;
    default:
      msg = N_("unknown file error");
      break;
    }

  if (msg)
    cob_runtime_error ("%s (STATUS=%02d)", gettext (msg), status);
}


void
cob_init_fileio (void)
{
  fileio_funcs[COB_ORG_SEQUENTIAL] = &sequential_funcs;
  fileio_funcs[COB_ORG_LINE_SEQUENTIAL] = &lineseq_funcs;
  fileio_funcs[COB_ORG_RELATIVE] = &relative_funcs;
  fileio_funcs[COB_ORG_INDEXED] = &indexed_funcs;
  fileio_funcs[COB_ORG_SORT] = &sort_funcs;

#if 0
  db_env_create (&dbenv, 0);
  dbenv->set_errpfx (dbenv, "DB");
  dbenv->set_errfile (dbenv, stderr);
  dbenv->open (dbenv, NULL, DB_CREATE | DB_INIT_MPOOL, 0);
#endif
}
