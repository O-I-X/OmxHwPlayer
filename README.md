<center>
## Omx-Halloween-Player
#### (for Raspberry Pi)

version 0.9.1</center>

**Installation:**  

- place the setprompt.sh script in /usr/local/bin/  
  (it hides & restores the console command prompt and cursor. otherwise 
   one could see the terminal for a brief moment inbetween videos)
- video files and images have to be in subfolders Idle, Timed and Videos 
  in the players work directory (~/OmxHwPlayer) or in the root of an 
  USB jump drive
- when using an USB jump drive, the system has to auto-mount such drives
  (ie. using udev.rules) or you have to add the drives UUID to the fstab 
  file to allow mounting during boot-up directly under /media.
  (ie. UUID=A3C0-96B1   /media   auto  nosuid,nodev,nofail,noatime  0 0)
- you must add 'usb' as parameter to start the player using the videos 
  from the jump drive (i.e. by adding it to the 'gohw'-alias in 
  autostart.inc), when there are also videos in the players work 
  directory. otherwise the player will always use the local video files. 
  only if there are no local videos, you can omit the 'usb' parameter, 
  as the player tries to find videos under /media when there are no local 
  files.
- make sure .bashrc sources the autostart.inc file as its last instruction
- make sure omxplayer and fbi (frame buffer image viewer) are installed


**Alias commands:**  

: **phide** : hide the command prompt and the console  
  **pshow** : show the command prompt and console again  
  **gohw**  : manually start the Halloween player  
  **gotb**  : show a 1080p test pattern with pilot tone  
              (handy to setup the projector)

**Wiring:**  

make sure the GPIOs are enabled in the RPi configuration.
the PIR motion sensor that triggers the video playback has to be 
connected to pin 1 (wiringPi number system).
you may also connect a LOW-active button (switch against GND) to pin 0.

- press & hold the buton during (usually auto-) login, will prevent the 
  player to be autostarted and to go to normal command prompt instead
- press for less than 5 seconds while halloween player is running, will 
  quit the player. ( note: I have not figured out yet, how to reliably 
  return to the console when fbi was using the framebuffer to display 
  images. :/ )
- press for more than 5 seconds while halloween player is running, will 
  shutdown the Raspberry

**General info:**  

the player will show a randomly selected video from the 'Videos' 
directory, each time the PIR sensor is triggered. there is a timeout 
(20 seconds) before the next video can be triggered.

in case there are videos present in the 'Timed' directory, they will be 
played by random selection, when there was no normal video triggered for 
3 minutes.

if there are video or image files in the 'Idle' directory, one is randomly 
selected and displayed (images) or played as loop (video), while no other 
video playback is active.

except for the timed videos, audio is send via HDMI to the projector. thus, 
timed videos are silent when no speakers are connected to the local, analog 

audio out of the Pi or when a RPi Zero is used.
(as the player is meant for digital Halloween decoration, I didn't want to 
annoy the neighborhood with frequent audio when no trick-and-treaters are 
around actually. thus, pay attention what video clips you put into the 
'Idle' directory, as those videos play audio, to allow for instance the 
faint sparkling of a fireplace or from a jack-o-lantern illumination)

while the files are always selected by random, the player will not show the 
same file twice in a row (based on each directory - thus, if you place the 
same video in multiple directories or multiple times in one directory, you 
may see it consecutively now and then).

if everything is installed correctly, RPi will autostart the Halloween player 
after user login. thus, it is recommended to setup the Pi to auto-login into 
CLI. the autostart only happens for the first login shell after system start 
or reboot and when the pin 0 switch is not pressed during login. autostart 
is also not performed, when logging in from a remote system (ie. via ssh).


feel free to add your own functionality to the player.  
to compile the program:  
***  g++ -Wall -o OmxHwPlayer main_OmxHwPlayer.cpp -lwiringPi -lpthread  ***

player was tested and used with a **RPi B rev.1** and a **RPi Zero**.

