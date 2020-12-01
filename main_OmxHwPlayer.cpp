// ***************************************************
// **                                               **
// **    Omx-Halloween-Player (for Raspberry Pi)    **
// **                                               **
// **  created  :   Oct.2019     by ObeliX-@gmx.de  **
// **  last edit: 12.11.2020     by ObeliX-@gmx.de  **
// **                                               **
// ***************************************************
//
// tested with RPi Zero & Mod. B rev 1
// compile: g++ -Wall -o OmxHwPlayer main_OmxHwPlayer.cpp -lwiringPi -lpthread


#define SILENT " >/dev/null 2>&1"

#include <iostream>
#include <thread>
#include <wiringPi.h>
#include <sys/timeb.h>

timeb g_stTime;

char  CT[100];                // Current Time
#define UPD_CT strftime(CT, sizeof(CT), "%T", localtime(&g_stTime.time))

#include <time.h>

unsigned long GetMsTimeTicks(bool _bUpdate=true)
{
  ftime(&g_stTime);
  if(_bUpdate) UPD_CT;
  return g_stTime.time*1000+g_stTime.millitm;
}


using namespace std;

#include <dirent.h>
#include <vector>
#include <string>
#include <sstream>

int GetVideoList(vector<string> & _vecVideoList, const string& _strDirPath)
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


string trim(const string& _str)
{
  auto beg=_str.find_first_not_of(" \t");           // find SPACE or TAB
  auto end=_str.find_last_not_of(" \t")+1;

  return _str.substr(beg, end-beg);
}


vector<string> split(const string& _str, char _cDelim=' ')
{
  stringstream ss(_str);
  string strEntry;
  vector<string> vecResultList;

  while(getline(ss, strEntry, _cDelim))
    vecResultList.push_back(trim(strEntry));

  return vecResultList;
}


int FbiClearScreen()
{
//  thread( []{ system("fbi -a --noverbose ./blank.png" SILENT); } ).detach();
  return system("fbi -a --noverbose ./blank.png" SILENT);
}


int OmxLoopPlay(const string& _strIdleVid)            // show idle video
{
  static const string LOOP="omxplayer -o hdmi -w -b --no-osd --no-keys --loop \"";

  return system((LOOP+_strIdleVid+'\"'+SILENT).c_str());
}


int FbiImageShow(const string& _strIdleImg)           // show idle image
{
  static const string SHOW="fbi -a --noverbose \"";   // autozoom & no status line

  return system((SHOW+_strIdleImg+'\"'+SILENT).c_str());
}


int StopIdle(const bool _bIsImage, const unsigned int _uDelay=0)
{
  // when current idle is an animation, the blank canvas is already in place and
  // we have to live with a brief, black display gap before omx starts new video
  if(!_bIsImage) return system("killall -q -SIGINT omxplayer.bin" SILENT);

  if(_uDelay) delay(_uDelay);     // when current idle is an image, allow player
                                  // to start up and overwrite idle image. than
  system("killall -q -SIGINT fbi" SILENT);    // remove idle display and replace
                                  // it with a blank screen, while omx plays the
  return FbiClearScreen();        // new action or lure video. thus blank canvas
}                                 // is in place when the video ends.


#include <fstream>

const uint  PIN_SWITCH = 0;
const uint  PIN_PIR = 1;

ofstream  g_log;
bool      g_bQuit=false;

void on_switch_pressed()
{
  delay(1000);                                // let settle down everything
  while(digitalRead(PIN_SWITCH)) delay(50);   // wait until button gets pressed
                                              // (low-active by internal pull-up)
  const ulong uPressed=GetMsTimeTicks();
  while(!digitalRead(PIN_SWITCH)) delay(50);  // wait until button is released

  g_bQuit=true;                               // set global quit flag for player

  if(GetMsTimeTicks(false)-uPressed<5000) {   // quit player
    g_log<<CT<<" ("<<uPressed<<") QUIT button pressed! Goodbye :)\n"<<flush;
    g_log.close();                            // make it the last log-message
    system("killall -q -SIGINT fbi omxplayer.bin" SILENT);
  } else {                                    // shutdown system
    g_log<<CT<<" ("<<uPressed<<") SHUTDOWN button pressed! Goodbye :)\n"<<flush;
    g_log.close();                            // close log before shutdown
    system("sudo shutdown -h now" SILENT);
  }
}


bool g_bPIR=false;

void on_PIR_triggered()
{
  g_log<<CT<<" INFO: waiting for PIR-sensor ...\n"<<flush;
  // only accept the PIR-signal when there is an PIR sensor connected actually
  // as we set the pin to internal pull-up, a LOW reading confirmes 'something'
  // is attached to this signal pin
  while(digitalRead(PIN_PIR)) {       // wait for initial PIR sensor free signal
    delay(250);
    if(g_bQuit) return;
  }
  GetMsTimeTicks();
  g_log<<CT<<" INFO: PIR-sensor was detected!\n"<<flush;

  // track PIR state and take care of short mis-triggers (HC-SR510 is prone to)
  while(!g_bQuit) {                   // filter out mis-triggers of PIR sensor
    while(!digitalRead(PIN_PIR)) {    // wait until PIR gets triggered
      delay(50);
      if(g_bQuit) return;
    }
    delay(100);                       // after some ms check PIR again
    if(!digitalRead(PIN_PIR)) {       // if it felt back, it was noise only
      const ulong uCT=GetMsTimeTicks();
      g_log<<CT<<" ("<<uCT<<") WARNING: false PIR trigger detected!\n"<<flush;
      continue;
    }
    g_bPIR=true;                      // otherwise it is a stable detection
    while(digitalRead(PIN_PIR)) {     // wait until PIR is not triggered anymore
      delay(50);
      if(g_bQuit) return;
    }
    g_bPIR=false;
  }
}


ulong RESTART_TIMEOUT = 20000;        // 20s
ulong LURE_TIMEOUT    = 180000;       // 3min
ulong IDLE_STOP_DELAY = 4000;         // 4s    (omxplayer start-up time)

int ReadConfig(const string _strCfgFile)
{
  if(_strCfgFile.empty()) return 0;             // no file name -> we are done

  ifstream CfgFile(_strCfgFile);
  if(!CfgFile.is_open()) return -1;             // failed to open cfg file

  string strLine;
  vector<string> vecSplitted;
  while(getline(CfgFile, strLine)) {
    strLine=strLine.substr(0, strLine.find_first_of('#'));  // remove comments
    vecSplitted=split(strLine, ':');
    if(vecSplitted.size()!=2) continue;
    if(vecSplitted[0]=="Restart_Timeout") {
      RESTART_TIMEOUT=stol(vecSplitted[1]);
    } else
    if(vecSplitted[0]=="Lure_Timeout") {
      LURE_TIMEOUT=stol(vecSplitted[1]);
    } else
    if(vecSplitted[0]=="IdleStop_Delay") {
      IDLE_STOP_DELAY=stol(vecSplitted[1]);
    }
  }
  CfgFile.close();

  return 1;                                     // config file was pocessed
}


int Wait4UsbDrive(string & _strDir)
{
  string strUsbDir="/media/";                   // start in base mount directory
  const string strUser(getenv("USER"));
  bool bInUserDir=false;
  vector<string> vecDirEntries;
  uint u, uListSize;

  while(true) {                                 // first, wait that a jump drive gets mounted
    if(g_bQuit) return 0;
    GetVideoList(vecDirEntries, strUsbDir);     // load usb directory entries
    uListSize=vecDirEntries.size();
    if(!uListSize) { delay(5000); continue; }   // wait until something shows up in usb-dir
    if(!bInUserDir) {                           // when still in base mount dir,
      for(u=0; u<uListSize; ++u)                // check if usb drive is mounted under users name
        if(vecDirEntries[u]==strUser) {         // if so, point usb-directory to that location
          strUsbDir+=strUser+'/';
          bInUserDir=true;                      // usb drive is mounted under user sub dir
          break;
        }
      if(bInUserDir) continue;                  // usb-dir was set to user -> check again for entries
    }
    break;                                      // usb-dir with entries was detected
  }                                             // (either in base- or user specific mount dir)

  string strSubDir;                             // mounted usb drive was detected
  vector<string> vecActionFiles;                // try to find the video file directory
  for(u=0; u<uListSize; ++u) {
    strSubDir=vecDirEntries[u];                 // check if usb-dir contains the 'Action'-folder
    if(strSubDir=="Action") break;              // found video directory (direct in user's mount dir)
    GetVideoList(vecActionFiles, strUsbDir+strSubDir+'/');  // if entry isn't, check all its sub-dirs
    for(uint s=0; s<vecActionFiles.size(); ++s) // maybe usb-dir contains an entries for each mounted
      if(vecActionFiles[s]=="Action") {         // jump drive -> check all sub dirs
        strUsbDir+=strSubDir+'/';               // when action folder was found, set usb-dir to it
        u=uListSize;                            // and terminate both for-loops
        break;                                  // video directory is in jump drives sub-dir or
      }                                         // drive itself was mounted as own sub-dir
  }

  if(u==uListSize) return 0;                    // no 'Action'-folder was found

  _strDir=strUsbDir;

  return 1;
}


#include <sys/wait.h>                 // just for SIGnal names

int main(int argc, char** argv)
{
  // begin logging
  ulong uCurTime=GetMsTimeTicks(false);

  g_log.open("OmxHwPlayer.log", ofstream::app);
  strftime(CT, sizeof(CT), "%d.%m.%Y", localtime(&g_stTime.time));
  g_log<<"\n---[ "<<CT<<" ]------------------------------------\n";
  UPD_CT;
  g_log<<CT<<" ("<<uCurTime<<") OmxHwPlayer started\n";

  // check for command line parameter
  bool bUseUsb=false;
  for(int i=1; i<argc; ++i)
    if(string(argv[i])=="usb") {
      bUseUsb=true;
      g_log<<CT<<" ("<<uCurTime<<") INFO: use of USB jump drive forced by parameter\n";
    }

  // read parameters from an optional config file
  switch(ReadConfig("OmxHwPlayer.cfg")) {
    default : g_log<<CT<<" ("<<uCurTime<<") ERROR: failed to read config file\n"; break;
    case  0 : break;                  // no cfg file & no error
    case  1 : g_log<<CT<<" ("<<uCurTime<<") INFO: found config file\n"; break;
  }

  // create a list of available video and image files
  vector<string> vecIdleList;
  vector<string> vecLureList;
  vector<string> vecVideoList;

  // load mandatory action videos
  string strVidDir="./";              // by default videos directories are
                                      // local in the player directory
  if(!bUseUsb) {                      // with no USB forced, try local videos first
    GetVideoList(vecVideoList, strVidDir+"Action/");
    if(!vecVideoList.size()) {        // if there are no local video files
      bUseUsb=true;                   // try to get them from the USB drive
      g_log<<CT<<" ("<<uCurTime<<") INFO: no local videos - lets try USB jump drive\n";
    }
  }

  if(bUseUsb) {                       // USB was forced or no local videos were found
    uCurTime=GetMsTimeTicks();
    g_log<<CT<<" ("<<uCurTime<<") INFO: waiting for USB-stick ...\n";
    switch(Wait4UsbDrive(strVidDir)) {
      default : break;                // some error occured
      case 0  : break;                // no USB drive data was found
      case 1  :                       // found video files
        uCurTime=GetMsTimeTicks();
        g_log<<CT<<" ("<<uCurTime<<") INFO: found USB-stick @ "<<strVidDir<<"\n";
        GetVideoList(vecVideoList, strVidDir+"Action/");
        break;
    }
  }

  const uint uNumVid=vecVideoList.size();
  if(!uNumVid) {
    g_log<<CT<<" ("<<uCurTime<<") ERROR: Sorry, no videos! :/ Goodbye\n";
    g_log.close();
    return 1;
  }

  // load optional idle- and lure videos
  GetVideoList(vecIdleList, strVidDir+"Idle/");
  GetVideoList(vecLureList, strVidDir+"Lure/");

        uint uNumIdle=vecIdleList.size();
  const uint uNumLure=vecLureList.size();

  g_log<<"found "<<uNumIdle<<" idle files:\n";
  for(uint u=0; u<uNumIdle; ++u)
    g_log<<"  I-#"<<u<<" --> "<<vecIdleList[u]<<endl;
  g_log<<"found "<<uNumLure<<" lure files:\n";
  for(uint u=0; u<uNumLure; ++u)
    g_log<<"  L-#"<<u<<" --> "<<vecLureList[u]<<endl;
  g_log<<"found "<<uNumVid<<" action files:\n";
  for(uint u=0; u<uNumVid; ++u)
    g_log<<"  V-#"<<u<<" --> "<<vecVideoList[u]<<endl;

  // setup external hardware triggers
  wiringPiSetup();
  pinMode(PIN_PIR,    INPUT); pullUpDnControl(PIN_PIR,    PUD_UP);
  pinMode(PIN_SWITCH, INPUT); pullUpDnControl(PIN_SWITCH, PUD_UP);

  thread(on_switch_pressed).detach();           // quit player or shutdown raspberry
  thread(on_PIR_triggered).detach();

  // prepare player
  srand(time(NULL));                            // seed the random number generator

  int   iSysRet=0;                              // return value of system() calls
  uint  uLastIdle=uNumIdle?rand()%uNumIdle:0;
  uint  uLastLure=uNumLure?rand()%uNumLure:0;
  uint  uLastVid=rand()%uNumVid;
  bool  bIsImage=true;                          // initial state is always FbiClearScreen's 'blank'-image

  ulong uVidTimeOut=uCurTime+RESTART_TIMEOUT/2;
  ulong uLureTimeOut=uCurTime+LURE_TIMEOUT;

  g_log<<CT<<" ("<<uCurTime<<") VT="<<uVidTimeOut<<" LT="<<uLureTimeOut<<endl;

  // main player loop
  thread(FbiClearScreen).detach();              // hide console
  while(!g_bQuit) {
    if(!g_bPIR || uCurTime<uVidTimeOut) {       // PIR is not triggered or still in restart suppression time
      // show idle animation or image
      switch(uNumIdle) {
        default :                                     // multiple idle files -> pick next to show
          uLastIdle+=1+rand()%(uNumIdle-1);           // select next idle file by random
          if(uLastIdle>=uNumIdle) uLastIdle-=uNumIdle;
        case 1  : {                                   // have only one idle file or next was selected from list
          g_log<<CT<<" ("<<uCurTime<<") show I-#"<<uLastIdle<<endl;

          const string strIdleFile=strVidDir+"Idle/"+vecIdleList[uLastIdle];
          size_t idx=strIdleFile.find_last_of('.');
          string strIdleType=strIdleFile.substr(idx+1);
          for(auto &c : strIdleType) c=tolower(c);

          bIsImage=strIdleType=="png" || strIdleType=="jpg" || strIdleType=="jpeg" || strIdleType=="gif";
          if(bIsImage) {
            if(uNumIdle==1) uNumIdle=0;               //display this single idle image once for all times
            thread(FbiImageShow, strIdleFile).detach();
          } else
            thread(OmxLoopPlay, strIdleFile).detach();
          } break;
        case 0  : break;                              // no idle files OR there is only one image file that was
      }                                               // already displayed and doesn't need any further handling

      while((!g_bPIR || uCurTime<uVidTimeOut) &&      // wait until PIR gets triggered after suppression ran out
            (!uNumLure || uCurTime<uLureTimeOut) &&   // or when there are lure videos and it is time to show one
            !g_bQuit) {                               // (of course don't wait anymore if player is about to quit)
        delay(100);
        uCurTime=GetMsTimeTicks(false);
      }
      UPD_CT;                                         // update time stamp when idle mode was left

      // stop idle display                            // if it was an animation or if there are different idle files
      if(!bIsImage || uNumIdle>1)                               // use some delay to avoid a blank screen inbetween,
        thread(StopIdle, bIsImage, IDLE_STOP_DELAY).detach();   // as omxplayer needs a moment to start the new
                                                                // action or lure video
      if(g_bQuit) break;
    }

    // show action or lure video
    if(g_bPIR) {                                // PIR was triggerd -> show action video
      if(uNumVid>1) {
        uLastVid+=1+rand()%(uNumVid-1);               // select next video by random
        if(uLastVid>=uNumVid) uLastVid-=uNumVid;
      }
      if(g_log.is_open()) g_log<<CT<<" ("<<uCurTime<<") play V-#"<<uLastVid<<endl;
      static const string PLAY="omxplayer -o hdmi -w -b --no-osd --no-keys \""+strVidDir+"Action/";
      iSysRet=system((PLAY+vecVideoList[uLastVid]+'\"'+SILENT).c_str());   // show action video
      uCurTime=GetMsTimeTicks();
      uVidTimeOut=uCurTime+RESTART_TIMEOUT;     // set normal restart suppression timeout to prevent continuous playback
    } else {                                    // PIR not triggered -> show lure video
      switch(uNumLure) {
        default: uLastLure+=1+rand()%(uNumLure-1);    // multiple lure files -> pick next by random
                 if(uLastLure>=uNumLure) uLastLure-=uNumLure;
        case 1 : break;                               // have only one lure file or next was selected from list
        case 0 : continue;                            // no lure videos -> return to idle
      }
      if(g_log.is_open()) g_log<<CT<<" ("<<uCurTime<<") play L-#"<<uLastLure<<endl;
      static const string LURE="omxplayer -o local -w -b --no-osd --no-keys \""+strVidDir+"Lure/";
      iSysRet=system((LURE+vecLureList[uLastLure]+'\"'+SILENT).c_str());   // show lure video (w/o HDMI-audio)
      uCurTime=GetMsTimeTicks();
      uVidTimeOut=uCurTime+RESTART_TIMEOUT/3;   // set shortened restart suppression timeout after a lure video
    }

    // check if a termination signal was received while system()-call was executed to play the video
    if(WIFSIGNALED(iSysRet) && (WTERMSIG(iSysRet)==SIGINT || WTERMSIG(iSysRet)==SIGQUIT)) {
      if(g_log.is_open()) g_log<<CT<<" ("<<uCurTime<<") DEBUG: termination fetched by system()-handler\n"<<flush;
      break;
    }

    uLureTimeOut=uCurTime+LURE_TIMEOUT;         // set time when next lure video will start if PIR doesn't get a trigger
    if(g_log.is_open()) g_log<<CT<<" ("<<uCurTime<<") VT="<<uVidTimeOut<<" LT="<<uLureTimeOut<<endl<<flush;
  }

  if(g_log.is_open()) {                         // maybe button thread already closed the log file with its own message
    g_log<<CT<<" ("<<uCurTime<<") close OmxHwPlayer. Goodbye :)\n";
    g_log.close();
  }

  if(!g_bQuit) system("killall -q -SIGINT fbi omxplayer.bin" SILENT);   // if termination was not initiated by button
                                                                        // stop possible running display threads
  return 0;
}
