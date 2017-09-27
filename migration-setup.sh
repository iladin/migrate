#!/bin/bash

. ./common || exit 1

if [[ -d "$migration_dir" ]]; then
  echo "Press C-c to abort generating a fresh trial migration tree"
  echo "or press return to continue."
  read dummy
fi
if [[ -d "$migration_dir" ]]; then
  rm -rf "$migration_dir"
fi

git init --bare "$migration_dir"

# Push migration branch.
git push "$migration_dir" meta/2017-migration

# Push CVS branch.
cd "$cvs_git_dir"
git push "$migration_dir" master:cvs/import-review

# Push SVN branch.
cd "$svn_git_dir"
git push "$migration_dir" master:svn/import-review

# Run migration scripts.
cd "$migration_dir"
