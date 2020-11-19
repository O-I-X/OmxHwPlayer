#!/bin/bash --

# ===========================================
# source this file at the end of your .bashrc
# to enable autostart of the OmxHwPlayer
# ===========================================

alias phide='. setprompt.sh hide'
alias pshow='. setprompt.sh'
alias gohw='phide && cd ~/OmxHwPlayer && ./OmxHwPlayer >/dev/null 2>&1'
alias gotb='omxplayer -o hdmi -w -b --loop ~/OmxHwPlayer/Testbild\ 1080p.mp4'

# check if this session is local or via network
if [ -n "$SSH_CLIENT" ] || [ -n "$SSH_TTY" ]; then
  SESSION_TYPE=remote
else
  case $(ps -o comm= -p $PPID) in
    sshd|*/sshd) SESSION_TYPE=remote;;
  esac
fi

# the first bash after a system start or reboot sets a flag-file and exports a
# variable, thus any following session can identify, that it is not the first one
if [ ! -f /tmp/.not_first_bash.flag ]; then
  touch /tmp/.not_first_bash.flag
  START_PLAYER=allow               # allow autostart of halloween player
fi
export FIRST_TTY=no                # any further bash is not the initial one anymore

# start the Halloween-Player when the shell is the first one after system
# start and is not from a remote terminal and the player-button is not pressed
/usr/bin/gpio mode 0 up  # activate pull-up for button pin, thus it is H when not pressed
sleep 1
if [ `/usr/bin/gpio read 0` == "1" ] &&   # button is not pressed
   [ -z "$SESSION_TYPE" ] &&              # it is not a remote/SSH session
   [ -n "$START_PLAYER" ]; then           # it is the first session after system start
  gohw                             # hide prompt and start halloween player
  pshow                            # when player is done, restore the prompt
fi
