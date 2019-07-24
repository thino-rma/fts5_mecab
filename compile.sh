#!/bin/bash

CC=gcc
PREFIX=/usr/local
CFLAGS="-DDEBUG -DSTOP789"
DRY_RUN=0

NAME=fts5_mecab
SRC=$NAME.c
OUT=$NAME.so

usage_exit() {
    echo "Usage: $0 [-h|--help] [--prefix=PREFIX] [--clear-cflags] [-DSYMBOL] [--dry-run]"
    echo "  \$PREFIX=$PREFIX"
    echo "  \$CFLAGS=\"$CFLAGS\""
    echo "  \$DRY_RUN=$DRY_RUN"
    echo "Sample"
    echo "  $0 --prefix=\$HOME/usr --clear-cflags -DDEBUG -DSTOP789 --dry-run"
    exit 0
}

echo_and_do() {
    echo "$1"
    if [ $2 == 0 ]; then
        eval "$1"
    else
        echo "!! DRY RUN !!"
    fi 
}

_HELP=0
_DRY_RUN=0
for x in "$@"; do
    if [[ $x == -h ]] || [[ $x == --help ]]; then
        _HELP=1
    elif [[ $x == --dry-run ]]; then
        DRY_RUN=1
    elif [[ $x == --prefix=* ]]; then
        PREFIX=${x:9}
    elif [[ $x == --clear-cflags ]]; then
        CFLAGS=""
    elif [[ $x == -D* ]]; then
        CFLAGS="$CFLAGS -${x:1}"
    fi
done

if [ $_HELP == 1 ]; then
    usage_exit
fi


[ ! -d $PREFIX/lib ] && mkdir -p $PREFIX/lib

echo_and_do "$CC -g -fPIC -shared $SRC -o $OUT -I$PREFIX/include -L$PREFIX/lib -lmecab $CFLAGS" $DRY_RUN
if [ $? == 0 ]; then
  echo "Compilation succeeded. execute command below."
  echo "  cp -a $OUT $PREFIX/lib/"
else
  echo "compile error."
fi
echo ""