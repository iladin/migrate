# COBOL compiler specification					-*- sh -*-

name: "COBOL 85"

# Value: `yes', `no'
binary-bigendian: no
redefines-occurs: no

# Value:         digits  bytes
#                ------  -----
# `1-2-4-8'      1 -  2      1
#                3 -  4      2
#                5 -  9      4
#               10 - 18      8
# 
# `2-4-8'        1 -  4      2
#                5 -  9      4
#               10 - 18      8
binary-size: 1-2-4-8

# Value: `warning', `error'

invalid-value:		warning

# Value: `ok', `archaic', `obsolete', `unconformable'

author-paragraph:		obsolete
memory-size-clause:		obsolete
multiple-file-tape-clause:	obsolete
label-records-clause:		obsolete
value-of-clause:		obsolete
data-records-clause:		obsolete
alter-statement:		obsolete
goto-statement-without-name:	obsolete
stop-literal-statement:		obsolete
debugging-line:			ok
padding-character-clause:	ok
next-sentence-phrase:		archaic
