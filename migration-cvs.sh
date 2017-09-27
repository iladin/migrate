#!/bin/bash

. ./common || exit 1

fix-commit-author() {
  git filter-branch --commit-filter '
        if [ "$GIT_AUTHOR_EMAIL" = "'"$1"'" ]; then
          GIT_AUTHOR_NAME="'"$2"'";
          GIT_AUTHOR_EMAIL="'"$3"'";
          git commit-tree "$@";
        else
          git commit-tree "$@";
        fi' HEAD
}

cd "$migration_dir"
git checkout cvs/import-review

# Fix email addresses.
fix-commit-author bgiroud "Bernard Giroud" "bgiroud@open-cobol.org"
fix-commit-author bitwalk "Keiichi Takahashi" "bitwalk@jcom.home.ne.jp"
fix-commit-author knishida "Keisuke Nishida " "knishida@netlab.jp"
fix-commit-author simrw "Roger While" "simrw@sim-basis.de"
