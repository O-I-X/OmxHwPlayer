<center>
## Omx-Halloween-Player
#### (for Raspberry Pi)

version 0.1.0</center>

**Installation:**  

- place the setprompt.sh script in /usr/local/bin/  
  (it hides & restores the console command prompt and cursor. otherwise
   one could see the terminal for a brief moment inbetween videos) 
- video files have to be in subfolders named Buffer, Timed and Videos 
  in the players work directory (~/OmxHwPlayer)
- make sure .bashrc sources the autostart.inc file as its last instruction
- make sure omxplayer is installed


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
- press & hold the button during (usually auto-) login, will prevent the 
  player to be autostarted and to go to normal command prompt instead  
- press while halloween player is running, will shutdown the Raspberry

**General info:**  

the player will show a randomly selected video from the 'Videos' 
directory, each time the PIR sensor is triggered. there is a timeout 
(20 seconds) before the next video can be triggered.

in case there are videos present in the 'Timed' directory, they will be 
played by random selection, when there was no normal video triggered for 
3 minutes.

if there is a video file in the 'Buffer' directory, it is played as loop, 
while no other video playback is active.

except for the timed videos, audio is send via HDMI to the projector. thus, 
timed videos are silent when no speakers are connected to the local, analog 
audio out of the Pi or when a RPi Zero is used.  
(as the player is meant for digital Halloween decoration, I didn't want to 
annoy the neighborhood with frequent audio when no trick-and-treaters are 
around actually. thus, pay attention what video clip you put into the 
'Buffer' directory, as this video plays audio, to allow for instance the 
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

player was tested und used with a **RPi B rev.1** and a **RPi Zero**.
