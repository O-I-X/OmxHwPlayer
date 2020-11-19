#!/bin/bash --

# this script is intended to be called in the context of
# the calling bash. thus all variables are not script-local
# and would exist after script is done. thats way variables
# are explicitly unset at the end.

GLOBAL_DIR=/usr/local/share
USER_DIR=~/.local

if [ -f $GLOBAL_DIR/Prompt_ori.txt ]; then
  PDIR=$GLOBAL_DIR
elif [ -f $USER_DIR/Prompt_ori.txt ]; then
  PDIR=$USER_DIR
else
  PDIR=
fi

if [ "$1-" == "hide-" ]; then
  echo -e "HIDE prompt ...\n'pshow' to restore original"
  tput civis
  sleep 2
  if [ -n "$PDIR" ]; then
    if [ -f $PDIR/Prompt_non.txt ]; then
      PS1=`less ${PDIR}/Prompt_non.txt`
    else
      PS1='\e[0;30m'
    fi
  else
    echo $PS1 > $USER_DIR/Prompt_ori.txt
    PS1='\e[0;30m'
  fi
  clear
else
  if [ -n "$PDIR" ]; then
    PS1="`less ${PDIR}/Prompt_ori.txt` "
  else
    PS1='\[\e[0m\e]0;\u@\h: \w\a\]${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '
  fi
  tput cnorm
fi

unset PDIR
unset GLOBAL_DIR
unset USER_DIR
