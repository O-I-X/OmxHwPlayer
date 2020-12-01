#include <sys/timeb.h>

timeb g_stTime;

char  CT[100];
#define UPD_CT strftime(CT, sizeof(CT), "%T", localtime(&g_stTime.time))

unsigned long GetMsTimeTicks()
{
  ftime(&g_stTime);
  return g_stTime.time*1000+g_stTime.millitm;
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

int OmxLoopPlay(string _strIdleVid)
{
  static const string LOOP="omxplayer -o hdmi -w -b --no-osd --loop ";

  return system((LOOP+_strIdleVid).c_str());
}

int FbiImageShow(string _strIdleImg)
{
  static const string SHOW="fbi --noverbose -a ";

  return system((SHOW+_strIdleImg).c_str());
}

#include <fstream>

const uint  PIN_SWITCH = 0;
const uint  PIN_PIR = 1;

ofstream g_log;

void on_switch_pressed()
{
  delay(1000);                                // let settle down everything
  while(digitalRead(PIN_SWITCH)) delay(50);   // wait until button gets pressed
                                              // (low-active by internal pull-up)
  const ulong uPressed=GetMsTimeTicks(); UPD_CT;
  while(!digitalRead(PIN_SWITCH)) delay(50);  // wait until button is released

  if(GetMsTimeTicks()-uPressed<5000) {
    if(g_log.is_open()) {
      g_log<<CT<<" ("<<uPressed<<") QUIT switch pressed! Goodbye :)\n"<<flush;
      g_log.close();
    }
    system("killall -9 OmxHwPlayer omxplayer.bin fbi");
  } else {
    if(g_log.is_open()) {
      g_log<<CT<<" ("<<uPressed<<") shutdown switch pressed! Goodbye :)\n"<<flush;
      g_log.close();
    }
    system("sudo shutdown -h now");
  }
}

bool g_bPIR=false;

void on_PIR_triggered()
{
  g_log<<CT<<" INFO: waiting for PIR-sensor ...\n"<<flush;
  // only accept the PIR-signal when there is an PIR sensor connected actually
  // as we set the pin to internal pull-up, a LOW reading confirmes 'something'
  // is attached to this signal pin
  while(digitalRead(PIN_PIR)) delay(250);     // wait for initial PIR sensor free signal
  g_log<<CT<<" INFO: PIR-sensor was detected!\n"<<flush;
  // track PIR signal status and take care of short mis-trigers (the HC-SR510 is prone to)
  while(true) {                               // filter out mis-triggers of PIR sensor
    while(!digitalRead(PIN_PIR)) delay(50);   // wait until PIR gets triggered
    delay(100);                               // after some ms check PIR again
    if(!digitalRead(PIN_PIR)) {               // if it felt back, it was noise only
      const ulong uCT=GetMsTimeTicks(); UPD_CT;
      g_log<<CT<<" ("<<uCT<<") WARNING: false PIR trigger detected!\n"<<flush;
      continue;
    }
    g_bPIR=true;                              // otherwise it is a stable detection
    while(digitalRead(PIN_PIR)) delay(50);    // wait until PIR is not triggered anymore
    g_bPIR=false;
  }
}

const ulong RESTART_TIMEOUT = 30000;    // 30s
const ulong LURE_TIMEOUT = 300000;      // 5min

int main(int argc, char** argv)
{
  ulong uCurTime=GetMsTimeTicks();

  g_log.open("OmxPlay.log", ofstream::app);
  strftime(CT, sizeof(CT), "%d.%m.%Y", localtime(&g_stTime.time));
  g_log<<"\n---[ "<<CT<<" ]------------------------------------\n";
  UPD_CT;
  g_log<<CT<<" ("<<uCurTime<<") OmxHwPlayer started\n";

  string strVidDir="./";
  vector<string> vecIdleList;
  vector<string> vecLureList;
  vector<string> vecVideoList;

  if(argc>1) {                 // any command line argument means 'wait for an USB stick'
    uCurTime=GetMsTimeTicks(); UPD_CT;
    g_log<<CT<<" ("<<uCurTime<<") waiting for USB-stick ...\n"<<flush;
    string strUsbDir;
    while(true) {
      strUsbDir="/media/";
      GetVideoList(vecIdleList, strUsbDir);
      if(!vecIdleList.size()) { delay(5000); continue; }
//      strUsbDir+=vecIdleList[0]+'/';
//      GetVideoList(vecIdleList, strUsbDir);
//      if(!vecIdleList.size()) { delay(5000); continue; }
//      strVidDir=strUsbDir+vecIdleList[0]+'/';
        strVidDir=strUsbDir;
      uCurTime=GetMsTimeTicks(); UPD_CT;
      g_log<<CT<<" ("<<uCurTime<<") found USB-stick @ "<<strVidDir<<"\n"<<flush;
      break;
    }
  }

  GetVideoList(vecIdleList, strVidDir+"Idle/");
  GetVideoList(vecLureList, strVidDir+"Timed/");
  GetVideoList(vecVideoList, strVidDir+"Videos/");

  const uint uNumIdle=vecIdleList.size();
  const uint uNumLure=vecLureList.size();
  const uint uNumVid=vecVideoList.size();
  if(!uNumVid) {
    g_log<<CT<<" ("<<uCurTime<<") ERROR: No videos!\n"<<flush;
    g_log.close();
    return 1;
  }

  g_log<<"found "<<uNumIdle<<" idle files:\n";
  for(uint u=0; u<uNumIdle; ++u)
    g_log<<"  I-#"<<u<<" --> "<<vecIdleList[u]<<endl;
  g_log<<"found "<<uNumLure<<" lure files:\n";
  for(uint u=0; u<uNumLure; ++u)
    g_log<<"  L-#"<<u<<" --> "<<vecLureList[u]<<endl;
  g_log<<"found "<<uNumVid<<" video files:\n";
  for(uint u=0; u<uNumVid; ++u)
    g_log<<"  V-#"<<u<<" --> "<<vecVideoList[u]<<endl;

  wiringPiSetup();
  pinMode(PIN_PIR,    INPUT); pullUpDnControl(PIN_PIR,    PUD_UP);
  pinMode(PIN_SWITCH, INPUT); pullUpDnControl(PIN_SWITCH, PUD_UP);

  srand(time(NULL));  // seed the random number generator

  uint  uLastIdle=uNumIdle?rand()%uNumIdle:0;
  uint  uLastLure=uNumLure?rand()%uNumLure:0;
  uint  uLastVid=rand()%uNumVid;

  ulong uVidTimeOut=uCurTime+RESTART_TIMEOUT/2;
  ulong uLureTimeOut=uCurTime+LURE_TIMEOUT;

  g_log<<CT<<" ("<<uCurTime<<") VT="<<uVidTimeOut<<" LT="<<uLureTimeOut<<endl;
  thread shutdown(on_switch_pressed);                 // shutdown raspberry when switch is pressed
  thread PIRstate(on_PIR_triggered);
  while(true) {
    if(!g_bPIR || GetMsTimeTicks()<uVidTimeOut) {     // PIR is not triggered or still in restart suppression time

      switch(uNumIdle) {
        case 0  : break;                              // no idle files
        default :                                     // multiple idle files -> pick next to show
          uLastIdle+=1+rand()%(uNumIdle-1);           // select next idle file by random
          if(uLastIdle>=uNumIdle) uLastIdle-=uNumIdle;
        case 1  :                                     // have only one idle file or next was selected from list
          if(g_log.is_open()) g_log<<CT<<" ("<<uCurTime<<") show(I) #"<<uLastIdle<<endl;

          const string  strIdleFile=strVidDir+"Idle/"+vecIdleList[uLastIdle];
          size_t idx=strIdleFile.find_last_of('.');
          string strIdleType=strIdleFile.substr(idx+1);

          for(auto& c : strIdleType) c=tolower(c);
          const bool bIsImage=strIdleType=="png" ||
                              strIdleType=="jpg" ||
                              strIdleType=="jpeg" ||
                              strIdleType=="gif";
          if(bIsImage) thread(FbiImageShow, strIdleFile).detach();
          else         thread(OmxLoopPlay, strIdleFile).detach();
          break;
      }

      while((!g_bPIR || uCurTime<uVidTimeOut) &&          // wait until PIR gets triggered after suppression ran out
            (!uNumLure || uCurTime<uLureTimeOut)) {       // also go on, when there are lure videos and it is time to show one
        delay(100);
        uCurTime=GetMsTimeTicks();
      }
    }
    UPD_CT;

    if(uNumIdle>1) {                                         // remove a possible idle-image from console
      system("killall -9 fbi");                              // as the next will be a different one for sure
      thread(FbiImageShow, strVidDir+"blank.png").detach();  // kill fbi first, then overwrite its last image
    }                                                        // (witch remains on display) with a black picture

    if(g_bPIR || !uNumLure) {                         // PIR was triggerd -> show normal video
      if(uNumVid>1) {
        uLastVid+=1+rand()%(uNumVid-1);                   // select next video by random
        if(uLastVid>=uNumVid) uLastVid-=uNumVid;
      }
      if(g_log.is_open()) g_log<<CT<<" ("<<uCurTime<<") play(V) #"<<uLastVid<<endl;
      static const string PLAY="killall -9 omxplayer.bin; omxplayer -o hdmi -w -b --no-osd "+strVidDir+"Videos/";
      system((PLAY+vecVideoList[uLastVid]).c_str());      // kill a running player and start new video
      uCurTime=GetMsTimeTicks();
      uVidTimeOut=uCurTime+RESTART_TIMEOUT;               // set normal restart suppression timout to prevent continuous playback
    } else {                                          // PIR not triggered -> show lure video
      if(uNumLure>1) {
        uLastLure+=1+rand()%(uNumLure-1);                 // select next lure video by random
        if(uLastLure>=uNumLure) uLastLure-=uNumLure;
      }
      if(g_log.is_open()) g_log<<CT<<" ("<<uCurTime<<") play(L) #"<<uLastLure<<endl;
      static const string LURE="killall -9 omxplayer.bin; omxplayer -o local -w -b --no-osd "+strVidDir+"Timed/";
      system((LURE+vecLureList[uLastLure]).c_str());      // kill a running player and start lure video (w/o audio)
      uCurTime=GetMsTimeTicks();
      uVidTimeOut=uCurTime+RESTART_TIMEOUT/3;             // set shortened restart suppression timout after an lure video
    }
    UPD_CT;

    uLureTimeOut=uCurTime+LURE_TIMEOUT;               // set time when next lure video will start when PIR doesn't get a trigger
    if(g_log.is_open()) g_log<<CT<<" ("<<uCurTime<<") VT="<<uVidTimeOut<<" LT="<<uLureTimeOut<<endl<<flush;
  }

  if(g_log.is_open()) {                               // maybe the shutdown thread already closed the log file
    g_log<<CT<<" ("<<uCurTime<<") close OmxHwPlayer. Goodbye :)\n";
    g_log.close();
  }

  return 0;
}
