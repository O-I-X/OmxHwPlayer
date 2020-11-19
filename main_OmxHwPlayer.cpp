#include <sys/timeb.h>

timeb stTime;

char  CT[100];
#define UPD_CT strftime(CT, sizeof(CT), "%T", localtime(&stTime.time))

unsigned long GetMsTimeTicks()
{
  ftime(&stTime);
  return stTime.time*1000+stTime.millitm;
}

using namespace std;

#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <string>


int GetVideoList(vector<string> & _vecVideoList, string _strDirPath)
{
  DIR* pDir;
  dirent* pDirEntry;

  if((pDir=opendir(_strDirPath.c_str()))) {
    _vecVideoList.clear();
    while((pDirEntry=readdir(pDir))) {
      if(!pDirEntry->d_name || pDirEntry->d_name[0]=='.') continue; // ignore ., .. & hidden files
      _vecVideoList.push_back(pDirEntry->d_name);
    }
    closedir(pDir);
  } else return 1;

  return 0;
}


#include <wiringPi.h>
#include <iostream>
#include <thread>

int OmxLoopPlay(string _strBufferVid)
{
  static const string LOOP="omxplayer -o hdmi -w -b --no-osd --loop ./Buffer/";

  return system((LOOP+_strBufferVid).c_str());
}

#include <fstream>

const uint  PIN_SWITCH = 0;
const uint  PIN_PIR = 1;

ofstream logg;

void on_switch_pressed()
{
  delay(3000);                          // let settle down everything
  while(!digitalRead(PIN_SWITCH)) delay(50);

  if(logg.is_open()) {
    ulong uCT=GetMsTimeTicks(); UPD_CT;
    logg<<CT<<" ("<<uCT<<"): shutdown switch pressed! Goodbye :)\n";
    logg.close();
  }
  system("sudo shutdown -h now");
}

const ulong RESTART_TIMEOUT = 30000;    // 30s
const ulong LURE_TIMEOUT = 300000;      // 5min

int main(void)
{
  ulong uCurTime=GetMsTimeTicks();

  logg.open("OmxPlay.log", ofstream::app);
  strftime(CT, sizeof(CT), "%d.%m.%Y", localtime(&stTime.time));
  logg<<"\n---[ "<<CT<<" ]------------------------------------\n";
  UPD_CT;
  logg<<CT<<" ("<<uCurTime<<") OmxHwPlayer started\n";

  vector<string> vecVideoList;
  vector<string> vecLureVideos;
  string strBufferVid;

  GetVideoList(vecLureVideos, "./Buffer/");
  if(vecLureVideos.size()) strBufferVid=vecLureVideos[0];
  GetVideoList(vecLureVideos, "./Timed/");
  GetVideoList(vecVideoList, "./Videos/");

  const bool bHaveBufVid=!strBufferVid.empty();
  const uint uNumLure=vecLureVideos.size();
  const bool bHaveLureVid=uNumLure;
  const uint uNumVid=vecVideoList.size();
  if(!uNumVid) {
    logg<<CT<<" ("<<uCurTime<<") Error: No videos\n";
    logg.close();
    return 1;
  }

  if(bHaveBufVid) logg<<"use "<<strBufferVid<<" as buffer image\n";
  logg<<"found "<<uNumLure<<" lure files:\n";
  for(uint u=0; u<uNumLure; ++u)
    logg<<"  L-#"<<u<<" --> "<<vecLureVideos[u]<<endl;
  logg<<"found "<<uNumVid<<" video files:\n";
  for(uint u=0; u<uNumVid; ++u)
    logg<<"  V-#"<<u<<" --> "<<vecVideoList[u]<<endl;

  wiringPiSetup();
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_SWITCH, INPUT);
  srand(time(NULL));  // seed the random number generator

  uint  uLastVid=rand()%(uNumVid);
  uint  uLastLure=uNumLure?rand()%(uNumLure):0;
  ulong uVidTimeOut=uCurTime+RESTART_TIMEOUT/2;
  ulong uLureTimeOut=uCurTime+LURE_TIMEOUT;

  logg<<CT<<" ("<<uCurTime<<") VT="<<uVidTimeOut<<" LT="<<uLureTimeOut<<endl;
  thread shutdown(on_switch_pressed);                 // shutdown raspberry when switch is pressed
  while(true) {
    if(!digitalRead(PIN_PIR) || GetMsTimeTicks()<uVidTimeOut) {   // PIR is not triggered or still in restart suppression time
      if(bHaveBufVid) thread(OmxLoopPlay, strBufferVid).detach();
      while((!digitalRead(PIN_PIR) || uCurTime<uVidTimeOut) &&    // wait until PIR gets triggered after suppression ran out
            (!bHaveLureVid || uCurTime<uLureTimeOut)) {           // also go on, when there are lure videos and it is time to show one
        delay(100);
        uCurTime=GetMsTimeTicks();
      }
    }
    UPD_CT;

    if(digitalRead(PIN_PIR) || !bHaveLureVid) {       // PIR was triggerd -> show normal video
      uLastVid+=1+rand()%(uNumVid-1);                     // select next video by random
      if(uLastVid>=uNumVid) uLastVid-=uNumVid;
      if(logg.is_open()) logg<<CT<<" ("<<uCurTime<<") play(V) #"<<uLastVid<<endl;
      static const string PLAY="killall -9 omxplayer.bin; omxplayer -o hdmi -w -b --no-osd ./Videos/";
      system((PLAY+vecVideoList[uLastVid]).c_str());      // kill a running player and start new video
      uCurTime=GetMsTimeTicks();
      uVidTimeOut=uCurTime+RESTART_TIMEOUT;               // set normal restart suppression timout to prevent continuous playback
    } else {                                          // PIR not triggered -> show lure video
      uLastLure+=1+rand()%(uNumLure-1);                   // select next lure video by random
      if(uLastLure>=uNumLure) uLastLure-=uNumLure;
      if(logg.is_open()) logg<<CT<<" ("<<uCurTime<<") play(L) #"<<uLastLure<<endl;
      static const string LURE="killall -9 omxplayer.bin; omxplayer -o local -w -b --no-osd ./Timed/";
      system((LURE+vecLureVideos[uLastLure]).c_str());    // kill a running player and start lure video (w/o audio)
      uCurTime=GetMsTimeTicks();
      uVidTimeOut=uCurTime+RESTART_TIMEOUT/3;             // set shortened restart suppression timout after an lure video
    }
    UPD_CT;

    uLureTimeOut=uCurTime+LURE_TIMEOUT;               // set time when next lure video will start when PIR doesn't get a trigger
    if(logg.is_open()) logg<<CT<<" ("<<uCurTime<<") VT="<<uVidTimeOut<<" LT="<<uLureTimeOut<<endl;
  }

  if(logg.is_open()) {                                // maybe the shutdown thread already closed the log file
    logg<<CT<<" ("<<uCurTime<<") close OmxHwPlayer. Goodbye :)\n";
    logg.close();
  }

  return 0;
}
