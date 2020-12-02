<center>
##Omx-Halloween-Player
###(for Raspberry Pi)

version 1.0.0
</center>

**Genaral Description**  

this little tool can control a projector for your digital Halloween 
decoration using a Raspberry Pi.  

it will play a video file when triggered by an external signal, like 
a motion sensor for instance. inbetween such triggered action videos, 
the player can display nothing or an idle image or video loop. and if 
no video is triggered for a certain time, it can play a short clip to 
lure some trick-r-treaters.

![gh_social_preview](https://repository-images.githubusercontent.com/317739699/cf9e0580-34ef-11eb-9f2f-1478599c9057 "Halloween 2020")


**Setup:**  

- place the setprompt.sh script in /usr/local/bin/  
  (it hides & restores the console command prompt and cursor. otherwise 
   one could see the terminal for a brief moment inbetween videos)
- video files and images have to be in subfolders called "**Idle**", 
  "**Lure**" and "**Action**" in the players work directory (~/OmxHwPlayer) 
  or in the root of an USB jump drive.
- when using an USB jump drive, the system has to auto-mount such drives
  (ie. using udev.rules) or you have to add the drives UUID to the fstab 
  file to allow mounting during boot-up directly under /media.
  (ie. UUID=A3C0-96B1   /media   auto  nosuid,nodev,nofail,noatime  0 0)
- when there are also videos in the players work directory, you have to 
  add 'usb' as parameter to start the player using the videos from the 
  jump drive (i.e. by adding it to the '**gohw**'-alias in autostart.inc), 
  otherwise the player will always use the local video files.
  only if there are no local videos, you can omit the 'usb' parameter, 
  as the player tries to find videos under /media when there are no local 
  files.
- the player will look for USB video directories in /media, /media/USER, 
  /media/DRIVE_NAME and /media/USER/DRIVE_NAME
- make sure omxplayer and fbi (frame buffer image viewer) are installed
- make sure .bashrc sources the autostart.inc file as its last instruction  
  ```bash
  # autostart the OmxHwPlayer  
  . OmxHwPlayer/autostart.inc
  ```


**Wiring:**  

make sure the GPIOs are enabled in the RPi configuration.
the PIR motion sensor that triggers the video playback has to be 
connected to pin 1 (wiringPi number system).
you may also connect a LOW-active button (switch against GND) to pin 0.

- press & hold the button during (usually auto-)login, will prevent the 
  player to be autostarted and to go to normal command prompt instead.
- press for less than 5 seconds while Halloween Player is running, will 
  quit the player. (note: I have not figured out yet, how to reliably 
  return to the console when fbi currently was using the framebuffer 
  to display an image. :/ thus, player may quit into an all black 
  console - even if you switch virtual terminals. sometimes pressing
  Ctrl-D twice followed by Ctrl-C logs you out and restores login shell)
- press button for more than 5 seconds while Halloween Player is 
  running, will shutdown Raspberry.


**Alias commands:**  

: **phide** : hide the command prompt and the console  
  **pshow** : show the command prompt and console again  
  **gohw**  : manually start the Halloween player  
  **gotb**  : show a 1080p test pattern with pilot tone  
              (handy to setup the projector)  
  **killhw**: kill all halloween player processes


**Detailed info:**  

the player will show a randomly selected video from the 'Action' directory, 
each time the PIR sensor is triggered. after video finishes, there is a 
timeout (20 seconds) before the next video can be triggered. there must be
at least one video file present in 'Action' directory, otherwise the player
wont start.

in case there are videos present in the 'Lure' directory, they will be 
played by random selection, when there was no action video triggered for 
3 minutes.

if there are video or image files in the 'Idle' directory, one is randomly 
selected and displayed (images) or played as loop (video), when no other 
video playback is active. if there are no idle files, player will show a 
black image.

values for the action video restart timeout and the lure timeout can be 
specified in the OmxHwPlayer.cfg. w/o an cfg-file or when parameters are
not set in cfg, the default values of 20 sec and 3 min are used.

except for the lure videos, audio is send via HDMI to the projector. thus, 
lure videos are silent when no speakers are connected to the local, analog 
audio out of the Pi or when a RPi Zero is used. (as the player is meant for 
digital Halloween decoration, I don't want to annoy the neighborhood with 
frequent audio when no trick-r-treaters are around actually. thus, pay 
attention what video clips you put into the 'Idle' directory, as those 
videos play audio, to allow for instance the faint sparkling of a fireplace 
or from a jack-o-lantern illumination)

while the files are always selected by random, the player will not show the 
same file twice in a row (based on each directory - thus, if you place the 
same video in multiple directories or multiple times in one directory, you 
may see it consecutively now and then).

if everything is installed properly, RPi will autostart the Halloween Player 
after user login. thus, it is recommended to setup the Pi to auto-login into 
CLI. the autostart only happens for the first login shell after system start 
or reboot and when the pin 0 button is not pressed during login. autostart 
is also not performed, when logging in from a remote system (ie. via ssh).

the player does not print out anything to the terminal but writes logging 
information to OmxHwPlayer.log in the players work directory. remember that 
timestamps may be wrong or suddenly jump forward, as RPi does not have an 
RTC and only updates time to real local time when connected to internet.

hiding console prompt and cursor belongs to earlier versions (0.x.x) of
the Halloween Player. meanwhile it is not really required anymore, but I
kept it in the scripts (just in case and it doesn't hurt). 


feel free to add your own functionality and improvements to the player.  
to compile the program:  
: ***g++ -Wall -o OmxHwPlayer main_OmxHwPlayer.cpp -lwiringPi -lpthread***  
(to cross compile, replace 'g++' with 'arm-linux-gnueabi-g++ -std=c++11'
 and make sure the host system has access to the matching ld-linux.so, 
 libpthread.so and libwiringPi.so)

player was tested and used with a **RPi B rev.1** and a **RPi Zero W**.
