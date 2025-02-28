#ifndef __MULTIARC_HPP__
#define __MULTIARC_HPP__

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <windows.h>
#include <sudo.h>
#include <string>
#include <vector>
#include <utils.h>
#include <KeyFileHelper.h>
#include <pluginold.hpp>
using namespace oldfar;
#include "fmt.hpp"

#define INI_LOCATION (InMyConfig("plugins/multiarc/config.ini"))
#define INI_SECTION ("Settings")

//#define _NEW_ARC_SORT_
#define OLD_DIALOG_STYLE 1
#define _ARC_UNDER_CURSOR_
#define _GROUP_NAME_

#ifndef LIGHTGRAY
#define LIGHTGRAY 7
#endif

#define RETEXEC_ARCNOTFOUND 0x40000

#define F_ENCRYPTED 1

//#define MAX_COMMAND_LENGTH 32768
#define MA_MAX_SIZE_COMMAND_NAME 512

#define SUPER_PUPER_ZERO (0)

enum {
  CMD_EXTRACT=0,
  CMD_EXTRACTWITHOUTPATH,
  CMD_TEST,
  CMD_DELETE,
  CMD_COMMENT,
  CMD_COMMENTFILES,
  CMD_SFX,
  CMD_LOCK,
  CMD_PROTECT,
  CMD_RECOVER,
  CMD_ADD,
  CMD_MOVE,
  CMD_ADDRECURSE,
  CMD_MOVERECURSE,
  CMD_ALLFILESMASK,
  CMD_DEFEXT
};


// TODO: add to Archive API (?)
struct ArcItemUserData{
   DWORD SizeStruct;
   int Codepage;
   char *Prefix;
   char *LinkName;
};

typedef DWORD (WINAPI *PLUGINLOADFORMATMODULE)(const char *ModuleName);
typedef BOOL (WINAPI *PLUGINISARCHIVE)(const char *Name,const unsigned char *Data,int DataSize);
typedef BOOL (WINAPI *PLUGINOPENARCHIVE)(const char *Name,int *Type,bool Silent);
typedef int (WINAPI *PLUGINGETARCITEM)(struct PluginPanelItem *Item,struct ArcItemInfo *Info);
typedef BOOL (WINAPI *PLUGINCLOSEARCHIVE)(struct ArcInfo *Info);
typedef BOOL (WINAPI *PLUGINGETFORMATNAME)(int Type,char *FormatName,char *DefaultExt);
typedef BOOL (WINAPI *PLUGINGETDEFAULTCOMMANDS)(int Type,int Command,char *Dest);
typedef void (WINAPI *PLUGINSETFARINFO)(const struct PluginStartupInfo *plg);
typedef DWORD (WINAPI *PLUGINGETSFXPOS)(void);

struct PluginItem
{
  DWORD Flags;
  struct PluginStartupInfo Info;
  struct FarStandardFunctions FSF;

  PLUGINLOADFORMATMODULE pLoadFormatModule;
  PLUGINISARCHIVE pIsArchive;
  PLUGINOPENARCHIVE pOpenArchive;
  PLUGINGETARCITEM pGetArcItem;
  PLUGINCLOSEARCHIVE pCloseArchive;
  PLUGINGETFORMATNAME pGetFormatName;
  PLUGINGETDEFAULTCOMMANDS pGetDefaultCommands;
  PLUGINSETFARINFO pSetFarInfo;
  PLUGINGETSFXPOS pGetSFXPos;
};


class ArcPlugins
{
  private:
    std::vector<PluginItem> PluginsData;

	void AddPluginItem(
				PLUGINISARCHIVE pIsArchive,
				PLUGINOPENARCHIVE pOpenArchive,
				PLUGINGETARCITEM pGetArcItem,
				PLUGINLOADFORMATMODULE pLoadFormatModule = NULL,
				PLUGINCLOSEARCHIVE pCloseArchive = NULL,
				PLUGINGETFORMATNAME pGetFormatName = NULL,
				PLUGINGETDEFAULTCOMMANDS pGetDefaultCommands = NULL,
				PLUGINSETFARINFO pSetFarInfo = NULL,
				PLUGINGETSFXPOS pGetSFXPos = NULL);
  public:
    ArcPlugins(const char *ModuleName);
    ~ArcPlugins();

  public:
    int  IsArchive(const char *Name,const unsigned char *Data,int DataSize);
    BOOL IsArchive(int ArcPluginNumber, const char *Name,const unsigned char *Data,int DataSize, DWORD* SFXSize);
    BOOL OpenArchive(int PluginNumber,const char *Name,int *Type,bool Silent);
    int  GetArcItem(int PluginNumber,struct PluginPanelItem *Item,struct ArcItemInfo *Info);
    void CloseArchive(int PluginNumber,struct ArcInfo *Info);
    BOOL GetFormatName(int PluginNumber,int Type,char *FormatName,char *DefaultExt);
    BOOL GetDefaultCommands(int PluginNumber,int Type,int Command,char *Dest);
    int  FmtCount() {return (int)PluginsData.size();}
    static int WINAPI LoadFmtModules(const WIN32_FIND_DATA *FData,const char *FullName,ArcPlugins *plugins);
    static int __cdecl CompareFmtModules(const void *elem1, const void *elem2);

};


class PluginClass
{
  private:
    char ArcName[NM];
    char CurDir[NM];
    std::vector<PluginPanelItem> ArcData;
    struct stat ArcStat {};
    int ArcPluginNumber;
    int ArcPluginType;
    int LastTestState,LastWithoutPathsState;
    struct ArcItemInfo ItemsInfo;
    struct ArcInfo CurArcInfo;
    int64_t TotalSize;
    int64_t PackedSize;
    int DizPresent;

    char farlang[100];

    bool bGOPIFirstCall;
    char Title[NM];
    char FormatName[100];
    char DefExt[NM];
    struct InfoPanelLine InfoLines[15];
    struct KeyBarTitles KeyBar;
    char Format[100];
    char *DescrFiles[32];
    char DescrFilesString[256];

  private:
    void GetGroupName(PluginPanelItem *Items, int Count, char *ArcName);//$ AA 29.11.2001
    BOOL GetCursorName(char *ArcName, char *ArcFormat, char *ArcExt, PanelInfo *pi);//$ AA 29.11.2001
    BOOL GetFormatName(char *FormatName, char *DefExt=NULL); //$ AA 25.11.2001
    void GetCommandFormat(int Command,char *Format,int FormatSize);
    void FreeArcData();
    bool FarLangChanged();
    bool EnsureFindDataUpToDate(int OpMode);
    int ReadArchive(const char *Name,int OpMode);

  public:
    PluginClass(int ArcPluginNumber);
    ~PluginClass();

  public:
    int PreReadArchive(const char *Name);
    int GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
    void FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
    int SetDirectory(const char *Dir,int OpMode);
    void GetOpenPluginInfo(struct OpenPluginInfo *Info);
    int DeleteFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
    int ProcessHostFile(struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
    int GetFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,
                 int Move,char *DestPath,int OpMode);
    int PutFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,
                 int Move,int OpMode);
    int ProcessKey(int Key,unsigned int ControlState);
    static int SelectFormat(char *ArcFormat,int AddOnly=FALSE);
    static int FormatToPlugin(char *Format,int &PluginNumber,int &PluginType);
    static LONG_PTR WINAPI PutDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);
};


class ArcCommand
{
  private:
    bool NeedSudo;
    struct PluginPanelItem *PanelItem;
    int ItemsNumber;
    std::string ArcName;
    std::string ArcDir;
    std::string RealArcDir;
    std::string Password;
    std::string AllFilesMask;
    std::string TempPath;
    std::string NextFileName;
    int NameNumber;
    int PrevFileNameNumber;
    char PrefixFileName[32];
    char ListFileName[NM];
    unsigned int ExecCode;
    unsigned int MaxAllowedExitCode;
    int Silent; // $ 07.02.2002 AA
    int DefaultCodepage;

    char CommentFileName[MAX_PATH]; //$ AA 25.11.2001
    //HANDLE CommentFile; //$ AA 25.11.2001

  private:
    bool ProcessCommand(std::string FormatString, int CommandType, int IgnoreErrors, char *pcListFileName = nullptr);
    void DeleteBraces(std::string &Command);
    int ReplaceVar(std::string &Command);
    int MakeListFile(char *ListFileName, int QuoteName,
                     int UseSlash, int FolderName, int NameOnly, int PathOnly,
                     int FolderMask, const char *LocalAllFilesMask);

  public:
    ArcCommand(struct PluginPanelItem *PanelItem,int ItemsNumber,
               const char *FormatString,const char *ArcName,const char *ArcDir,const char *Password,
               const char *AllFilesMask,int IgnoreErrors,int CommandType,
               int Silent,const char *RealArcDir,int DefaultCodepage);
    ~ArcCommand(); //$ AA 25.11.2001

  public:
    int GetExecCode() {return(ExecCode);};
};


struct InitDialogItem
{
  unsigned char Type;
  unsigned char X1,Y1,X2,Y2;
  unsigned char Focus;
  DWORD_PTR Selected;
  unsigned int Flags;
  unsigned char DefaultButton;
  const char *Data;
};

/* $ 13.09.2000 tran
   разное для ожидания процесса, чтобы убить лист-файл */
struct KillStruct
{
    char ListFileName[260];
    HANDLE hProcess;
    HANDLE hThread;
};
/* tran 13.09.2000 $ */

struct MAAdvFlags
{
  unsigned ExactArcName         :1;
  unsigned AutoResetExactArcName:1;
  unsigned GroupName            :1;
  unsigned ArcUnderCursor       :1;
  unsigned MenuWrapMode         :2;
  unsigned PutDialogStyle       :1;
  unsigned                      :25;

  operator int32_t(){return *((int32_t *)this);}
  MAAdvFlags &operator=(int32_t Flags){
        *((int32_t *)this) = Flags;
        return *this;
  }
};

struct Options
{
  int HideOutput;
  int ProcessShiftF1;
  char DescriptionNames[NM];
  int ReadDescriptions;
  int UpdateDescriptions;
  /* $ 13.09.2000 tran
     запуск процесса в фоне */
  int UserBackground;
  int Background;
  int PriorityClass;
  /* tran 13.09.2000 $ */
  int OldUserBackground; // $ 02.07.2002 AY
  int UseLastHistory; // $ 18.05.2001 SVS
  int AllowChangeDir; // $ 05.08.2001 SVS
  //int DeleteExtFile; // $ 12.07.2001 SVS
  //int AddExtArchive; // $ 16.07.2001 SVS
  //BOOL AutoResetExactArcName; // $ 2?.11.2001 AA
  //BOOL ExactArcName;   // $ 30.11.2001 AA
  MAAdvFlags AdvFlags; //$ 06.03.2002 AA
  char CommandPrefix1[50]; //$ 23.01.2003 AY
};

/*
  Global Data
*/
extern struct FarStandardFunctions FSF;
extern struct Options Opt;
extern struct PluginStartupInfo Info;
extern class ArcPlugins *ArcPlugin;
extern const char *CmdNames[];

#ifdef _NEW_ARC_SORT_
extern char IniFile[];
extern const char *SortModes[];
#endif //_NEW_ARC_SORT_

extern DWORD PriorityProcessCode[];

/*
  Functions
*/

#ifdef _NEW_ARC_SORT_
void WritePrivateProfileInt(char *Section, char *Key, int Value, char *Ini);
#endif //_NEW_ARC_SORT_

std::string MakeFullName(const char *name);

int ConfigGeneral();
int ConfigCommands(char *ArcFormat,int IDFocus=2,BOOL FastAccess=FALSE,int PluginNumber=0,int PluginType=0);

const char *GetMsg(int MsgId);
int Execute(HANDLE hPlugin, const std::string &CmdStr, int HideOutput, int Silent, int NeedSudo, int ShowTitle, char *ListFileName=0);
char *SeekDefExtPoint(char *Name, char *DefExt=NULL, char **Ext=NULL); //$ AA 28.11.2001
BOOL AddExt(char *Name, char *Ext);                               //$ AA 28.11.2001
//void StartThreadForKillListFile(PROCESS_INFORMATION *pi,char *list);
char* QuoteText(char *Str);
void InitDialogItems(const struct InitDialogItem *Init,struct FarDialogItem *Item,
                     int ItemsNumber);
void InsertCommas(unsigned long Number,char *Dest);
void InsertCommas(int64_t Number,char *Dest);
int MA_ToPercent(long N1,long N2);
int MA_ToPercent(int64_t N1,int64_t N2);
int IsCaseMixed(const char *Str);
int CheckForEsc();
int LocalStrnicmp(const char *Str1,const char *Str2,int Length);
int __isspace(int Chr);
int FindExecuteFile(char *OriginalName,char *DestName,int SizeDest);
char *GetCommaWord(char *Src,char *Word,char Separator);
BOOL GoToFile(const char *Target, BOOL AllowChangeDir);
BOOL FileExists(const char* Name);
int GetScrX(void);
void NormalizePath(const char *SrcName,char *DestName);
std::string &ExpandEnv(std::string &str);
std::string &NormalizePath(std::string &path);

int WINAPI GetPassword(char *Password,const char *FileName);
void WINAPI UnixTimeToFileTime(DWORD UnixTime,FILETIME *FileTime);

#define MAX_WIDTH_MESSAGE (GetScrX()-14)

#ifdef __GNUC__
#define I64(x) x##ll
#else
#define I64(x) x##i64
#endif

#endif // __MULTIARC_HPP__

