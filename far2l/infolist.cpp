/*
infolist.cpp

Информационная панель
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include "infolist.hpp"
#include "macroopcode.hpp"
#include "colors.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "help.hpp"
#include "fileview.hpp"
#include "fileedit.hpp"
#include "manager.hpp"
#include "cddrv.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "drivemix.hpp"
#include "dirmix.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "mix.hpp"
#include "execute.hpp"
#ifdef __APPLE__
//# include <sys/sysctl.h>
# include <mach/mach_host.h>
# include <mach/vm_statistics.h>
#elif !defined(__FreeBSD__)
# include <sys/sysinfo.h>
#endif
 


static int LastDizWrapMode = -1;
static int LastDizWrapType = -1;
static int LastDizShowScrollbar = -1;

InfoList::InfoList():
	DizView(nullptr),
	PrevMacroMode(-1)
{
	Type=INFO_PANEL;
	if (LastDizWrapMode < 0)
	{
		LastDizWrapMode = Opt.ViOpt.ViewerIsWrap;
		LastDizWrapType = Opt.ViOpt.ViewerWrap;
		LastDizShowScrollbar = Opt.ViOpt.ShowScrollbar;
	}
}

InfoList::~InfoList()
{
	CloseFile();
	SetMacroMode(TRUE);
}

// перерисовка, только если мы текущий фрейм
void InfoList::Update(int Mode)
{
	if (!EnableUpdate)
		return;

	if (CtrlObject->Cp() == FrameManager->GetCurrentFrame())
		Redraw();
}

FARString &InfoList::GetTitle(FARString &strTitle,int SubLen,int TruncSize)
{
	strTitle.Format(L" %ls ", MSG(MInfoTitle));
	TruncStr(strTitle,X2-X1-3);
	return strTitle;
}

void InfoList::DisplayObject()
{
	FARString strTitle;
	FARString strOutStr;
	Panel *AnotherPanel;
//	FARString strDriveRoot;
	FARString strVolumeName, strFileSystemName;
	DWORD MaxNameLength,FileSystemFlags,VolumeNumber;
	FARString strDiskNumber;
	CloseFile();

	Box(X1,Y1,X2,Y2,COL_PANELBOX,DOUBLE_BOX);
	SetScreen(X1+1,Y1+1,X2-1,Y2-1,L' ',COL_PANELTEXT);
	SetColor(Focus ? COL_PANELSELECTEDTITLE:COL_PANELTITLE);
	GetTitle(strTitle);

	if (!strTitle.IsEmpty())
	{
		GotoXY(X1+(X2-X1+1-(int)strTitle.GetLength())/2,Y1);
		Text(strTitle);
	}

	SetColor(COL_PANELTEXT);

	int CurY=Y1+1;

	/* #1 - computer name/user name */

	{
		FARString strComputerName, strUserName;
		DWORD dwSize = 256; //MAX_COMPUTERNAME_LENGTH+1;
		wchar_t *ComputerName = strComputerName.GetBuffer(dwSize);
//		if (Opt.InfoPanel.ComputerNameFormat == ComputerNamePhysicalNetBIOS || !GetComputerNameEx(Opt.InfoPanel.ComputerNameFormat, ComputerName, &dwSize))
		{
			dwSize = MAX_COMPUTERNAME_LENGTH+1;
			WINPORT(GetComputerName)(ComputerName, &dwSize);  // retrieves only the NetBIOS name of the local computer
		}
		strComputerName.ReleaseBuffer();

		GotoXY(X1+2,CurY++);
		PrintText(MInfoCompName);
		PrintInfo(strComputerName);

		dwSize = 256;
		wchar_t *UserName = strUserName.GetBuffer(dwSize);
		WINPORT(GetUserName)(UserName, &dwSize);
		strUserName.ReleaseBuffer();

		GotoXY(X1+2,CurY++);
		PrintText(MInfoUserName);
		PrintInfo(strUserName);
	}

	/* #2 - disk info */

	SetColor(COL_PANELBOX);
	DrawSeparator(CurY);
	SetColor(COL_PANELTEXT);

	AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
	AnotherPanel->GetCurDir(strCurDir);

	if (strCurDir.IsEmpty())
		apiGetCurrentDirectory(strCurDir);

	fprintf(stderr, "apiGetVolumeInformation: %ls\n", strCurDir.CPtr());
	if (apiGetVolumeInformation(strCurDir,&strVolumeName,
	                            &VolumeNumber,&MaxNameLength,&FileSystemFlags,
	                            &strFileSystemName))
	{
//		strTitle=FARString(L" ")+DiskType+L" "+MSG(MInfoDisk)+L" "+(strDriveRoot)+L" ("+strFileSystemName+L") ";
		strTitle=FARString(L" ")+L" ("+strFileSystemName+L") ";

		strDiskNumber.Format(L"%04X-%04X",VolumeNumber>>16,VolumeNumber & 0xffff);
	}
	else // Error!
		strTitle = strCurDir;//strDriveRoot;

	TruncStr(strTitle,X2-X1-3);
	GotoXY(X1+(X2-X1+1-(int)strTitle.GetLength())/2,CurY++);
	PrintText(strTitle);

	/* #3 - disk info: size */

	uint64_t TotalSize,TotalFree,UserFree;

	if (apiGetDiskSize(strCurDir,&TotalSize,&TotalFree,&UserFree))
	{
		GotoXY(X1+2,CurY++);
		PrintText(MInfoDiskTotal);
		InsertCommas(TotalSize,strOutStr);
		PrintInfo(strOutStr);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoDiskFree);
		InsertCommas(UserFree,strOutStr);
		PrintInfo(strOutStr);
	}

	/* #4 - disk info: label & SN */

	if (!strVolumeName.IsEmpty()) {
		GotoXY(X1+2,CurY++);
		PrintText(MInfoDiskLabel);
		PrintInfo(strVolumeName);
	}

	GotoXY(X1+2,CurY++);
	PrintText(MInfoDiskNumber);
	PrintInfo(strDiskNumber);

	/* #4 - memory info */

	SetColor(COL_PANELBOX);
	DrawSeparator(CurY);
	SetColor(COL_PANELTEXT);
	strTitle = MSG(MInfoMemory);
	TruncStr(strTitle,X2-X1-3);
	GotoXY(X1+(X2-X1+1-(int)strTitle.GetLength())/2,CurY++);
	PrintText(strTitle);

#ifdef __APPLE__
        unsigned long long totalram;
        vm_size_t page_size;
        unsigned long long freeram;
        int ret_sc;

        //ret_sc =  (sysctlbyname("hw.memsize", &totalram, &ulllen, NULL, 0) ? 1 : 0);
        ret_sc = ( KERN_SUCCESS !=_host_page_size(mach_host_self(), &page_size)  ? 1 : 0);

        mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
        vm_statistics_data_t vmstat;

        ret_sc += (KERN_SUCCESS != host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count) ? 1 : 0);
        totalram = (vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count) * page_size;
        freeram  = vmstat.free_count * page_size;

        //double total = vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count;
        //double wired = vmstat.wire_count;
        //double active = vmstat.active_count;
        //double inactive = vmstat.inactive_count;
        //double free = vmstat.free_count;

	if (!ret_sc)
	{
		DWORD dwMemoryLoad = 100 - 
			ToPercent64(freeram, totalram);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoMemoryLoad);
		strOutStr.Format(L"%d%%", dwMemoryLoad);
		PrintInfo(strOutStr);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoMemoryTotal);
		InsertCommas(totalram,strOutStr);
		PrintInfo(strOutStr);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoMemoryFree);
		InsertCommas(freeram,strOutStr);
		PrintInfo(strOutStr);
	}


#elif !defined(__FreeBSD__)
	struct sysinfo si = {};
	if (sysinfo(&si) == 0)
	{
		DWORD dwMemoryLoad = 100 - 
			ToPercent64(si.freeram + si.freeswap, si.totalram + si.totalswap);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoMemoryLoad);
		strOutStr.Format(L"%d%%", dwMemoryLoad);
		PrintInfo(strOutStr);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoMemoryTotal);
		InsertCommas(si.totalram,strOutStr);
		PrintInfo(strOutStr);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoMemoryFree);
		InsertCommas(si.freeram,strOutStr);
		PrintInfo(strOutStr);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoSharedMemory);
		InsertCommas(si.sharedram,strOutStr);
		PrintInfo(strOutStr);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoBufferMemory);
		InsertCommas(si.bufferram,strOutStr);
		PrintInfo(strOutStr);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoPageFileTotal);
		InsertCommas(si.totalswap,strOutStr);
		PrintInfo(strOutStr);

		GotoXY(X1+2,CurY++);
		PrintText(MInfoPageFileFree);
		InsertCommas(si.freeswap,strOutStr);
		PrintInfo(strOutStr);
	}
#endif
	/* #5 - description */

	ShowDirDescription(CurY);
	ShowPluginDescription();
}


int64_t InfoList::VMProcess(int OpCode,void *vParam,int64_t iParam)
{
	if (DizView)
		return DizView->VMProcess(OpCode,vParam,iParam);

	switch (OpCode)
	{
		case MCODE_C_EMPTY:
			return 1;
	}

	return 0;
}

int InfoList::ProcessKey(int Key)
{
	if (!IsVisible())
		return FALSE;

	if (Key>=KEY_RCTRL0 && Key<=KEY_RCTRL9)
	{
		ExecShortcutFolder(Key-KEY_RCTRL0);
		return TRUE;
	}

	switch (Key)
	{
		case KEY_F1:
		{
			Help Hlp(L"InfoPanel");
		}
		return TRUE;
		case KEY_F3:
		case KEY_NUMPAD5:  case KEY_SHIFTNUMPAD5:

			if (!strDizFileName.IsEmpty())
			{
				CtrlObject->Cp()->GetAnotherPanel(this)->GetCurDir(strCurDir);
				FarChDir(strCurDir);
				new FileViewer(strDizFileName,TRUE);//OT
			}

			CtrlObject->Cp()->Redraw();
			return TRUE;
		case KEY_F4:
			/* $ 30.04.2001 DJ
			не показываем редактор, если ничего не задано в именах файлов;
			не редактируем имена описаний со звездочками;
			убираем лишнюю перерисовку панелей
			*/
		{
			Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
			AnotherPanel->GetCurDir(strCurDir);
			FarChDir(strCurDir);

			if (!strDizFileName.IsEmpty())
			{
				new FileEditor(strDizFileName,CP_AUTODETECT,FFILEEDIT_ENABLEF6);
			}
			else if (!Opt.InfoPanel.strFolderInfoFiles.IsEmpty())
			{
				FARString strArgName;
				const wchar_t *p = Opt.InfoPanel.strFolderInfoFiles;

				while ((p = GetCommaWord(p,strArgName)) )
				{
					if (!strArgName.ContainsAnyOf("*?"))
					{
						new FileEditor(strArgName,CP_AUTODETECT,FFILEEDIT_CANNEWFILE|FFILEEDIT_ENABLEF6);
						break;
					}
				}
			}

			AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
			//AnotherPanel->Redraw();
			Update(0);
		}
		CtrlObject->Cp()->Redraw();
		return TRUE;
		case KEY_CTRLR:
			Redraw();
			return TRUE;
	}

	/* $ 30.04.2001 DJ
		обновляем кейбар после нажатия F8, F2 или Shift-F2
	*/
	if (DizView && Key>=256)
	{
		int ret = DizView->ProcessKey(Key);

		if (Key == KEY_F8 || Key == KEY_F2 || Key == KEY_SHIFTF2)
		{
			DynamicUpdateKeyBar();
			CtrlObject->MainKeyBar->Redraw();
		}

		if (Key == KEY_F7 || Key == KEY_SHIFTF7)
		{
			int64_t Pos, Length;
			DWORD Flags;
			DizView->GetSelectedParam(Pos,Length,Flags);
			//ShellUpdatePanels(nullptr,FALSE);
			DizView->InRecursion++;
			Redraw();
			CtrlObject->Cp()->GetAnotherPanel(this)->Redraw();
			DizView->SelectText(Pos,Length,Flags|1);
			DizView->InRecursion--;
		}

		return(ret);
	}

	return FALSE;
}


int InfoList::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	int RetCode;

	if (Panel::PanelProcessMouse(MouseEvent,RetCode))
		return(RetCode);

	if (MouseEvent->dwMousePosition.Y>=14 && DizView)
	{
		_tran(SysLog(L"InfoList::ProcessMouse() DizView = %p",DizView));
		int DVX1,DVX2,DVY1,DVY2;
		DizView->GetPosition(DVX1,DVY1,DVX2,DVY2);

		if ((MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) &&
		        MouseEvent->dwMousePosition.X > DVX1+1 &&
		        MouseEvent->dwMousePosition.X < DVX2 - DizView->GetShowScrollbar() - 1 &&
		        MouseEvent->dwMousePosition.Y > DVY1+1 &&
		        MouseEvent->dwMousePosition.Y < DVY2-1
		   )
		{
			ProcessKey(KEY_F3);
			return TRUE;
		}

		if (MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED)
		{
			ProcessKey(KEY_F4);
			return TRUE;
		}
	}

	SetFocus();

	if (DizView)
		return(DizView->ProcessMouse(MouseEvent));

	return TRUE;
}


void InfoList::PrintText(const wchar_t *Str)
{
	if (WhereY()<=Y2-1)
	{
		FS<<fmt::Precision(X2-WhereX())<<Str;
	}
}


void InfoList::PrintText(int MsgID)
{
	PrintText(MSG(MsgID));
}


void InfoList::PrintInfo(const wchar_t *str)
{
	if (WhereY()>Y2-1)
		return;

	int SaveColor=GetColor(),MaxLength=X2-WhereX()-2;

	if (MaxLength<0)
		MaxLength=0;

	FARString strStr = str;
	TruncStr(strStr,MaxLength);
	int Length=(int)strStr.GetLength();
	int NewX=X2-Length-1;

	if (NewX>X1 && NewX>WhereX())
	{
		GotoXY(NewX,WhereY());
		SetColor(COL_PANELINFOTEXT);
		FS<<strStr<<L" ";
		SetColor(SaveColor);
	}
}


void InfoList::PrintInfo(int MsgID)
{
	PrintInfo(MSG(MsgID));
}


void InfoList::ShowDirDescription(int YPos)
{
	Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
	DrawSeparator(YPos);

	if (AnotherPanel->GetMode()==FILE_PANEL)
	{
		FARString strDir;
		AnotherPanel->GetCurDir(strDir);

		do {
			FARString strGit = strDir + L"/.git";
			struct stat s;
			if (stat(strGit.GetMB().c_str(), &s) == 0)
			{
				fprintf(stderr, "GIT: %ls\n", strGit.CPtr());
				std::vector<std::wstring> lines;
				std::string cmd = "git -C \"";
				cmd+= EscapeCmdStr(Wide2MB(strDir.CPtr()));
				cmd+= "\" status";

				if (POpen(lines, cmd.c_str()))
				{
					for (const auto &l : lines)
					{
						GotoXY(X1 + 2, ++YPos);
						PrintText(l.c_str());
					}
					DrawSeparator(++YPos);
				}
				break;
			}
		} while (CutToSlash(strDir, true));


		AnotherPanel->GetCurDir(strDir);

		if (!strDir.IsEmpty())
			AddEndSlash(strDir);

		FARString strArgName;
		const wchar_t *NamePtr = Opt.InfoPanel.strFolderInfoFiles;

		while ((NamePtr=GetCommaWord(NamePtr,strArgName)))
		{
			FARString strFullDizName;
			strFullDizName = strDir;
			strFullDizName += strArgName;
			FAR_FIND_DATA_EX FindData;

			if (!apiGetFindDataEx(strFullDizName, FindData, FIND_FILE_FLAG_CASE_INSENSITIVE)) {
				continue;
			}

			CutToSlash(strFullDizName, false);
			strFullDizName += FindData.strFileName;

			if (OpenDizFile(strFullDizName,YPos))
				return;
		}
	}

	CloseFile();
	SetColor(COL_PANELTEXT);
	GotoXY(X1+2,YPos+1);
	PrintText(MInfoDizAbsent);
}


void InfoList::ShowPluginDescription()
{
	Panel *AnotherPanel;
	static wchar_t VertcalLine[2]={BoxSymbols[BS_V2],0};
	AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->GetMode()!=PLUGIN_PANEL)
		return;

	CloseFile();
	OpenPluginInfo Info;
	AnotherPanel->GetOpenPluginInfo(&Info);

	for (int I=0; I<Info.InfoLinesNumber; I++)
	{
		int Y=Y2-Info.InfoLinesNumber+I;

		if (Y<=Y1)
			continue;

		const InfoPanelLine *InfoLine=&Info.InfoLines[I];
		GotoXY(X1,Y);
		SetColor(COL_PANELBOX);
		Text(VertcalLine);
		SetColor(COL_PANELTEXT);
		FS<<fmt::Width(X2-X1-1)<<L"";
		SetColor(COL_PANELBOX);
		Text(VertcalLine);
		GotoXY(X1+2,Y);

		if (InfoLine->Separator)
		{
			FARString strTitle;

			if (InfoLine->Text && *InfoLine->Text)
				strTitle.Append(L" ").Append(InfoLine->Text).Append(L" ");

			DrawSeparator(Y);
			TruncStr(strTitle,X2-X1-3);
			GotoXY(X1+(X2-X1-(int)strTitle.GetLength())/2,Y);
			PrintText(strTitle);
		}
		else
		{
			PrintText(NullToEmpty(InfoLine->Text));
			PrintInfo(NullToEmpty(InfoLine->Data));
		}
	}
}

void InfoList::CloseFile()
{
	if (DizView)
	{
		if (DizView->InRecursion)
			return;

		LastDizWrapMode=DizView->GetWrapMode();
		LastDizWrapType=DizView->GetWrapType();
		LastDizShowScrollbar=DizView->GetShowScrollbar();
		DizView->SetWrapMode(OldWrapMode);
		DizView->SetWrapType(OldWrapType);
		delete DizView;
		DizView=nullptr;
	}

	strDizFileName.Clear();
}

int InfoList::OpenDizFile(const wchar_t *DizFile,int YPos)
{
	bool bOK=true;
	_tran(SysLog(L"InfoList::OpenDizFile([%ls]",DizFile));

	if (!DizView)
	{
		DizView=new(std::nothrow) DizViewer;

		if (!DizView)
			return FALSE;

		_tran(SysLog(L"InfoList::OpenDizFile() create new Viewer = %p",DizView));
		DizView->SetRestoreScreenMode(FALSE);
		DizView->SetPosition(X1+1,YPos+1,X2-1,Y2-1);
		DizView->SetStatusMode(0);
		DizView->EnableHideCursor(0);
		OldWrapMode = DizView->GetWrapMode();
		OldWrapType = DizView->GetWrapType();
		DizView->SetWrapMode(LastDizWrapMode);
		DizView->SetWrapType(LastDizWrapType);
		DizView->SetShowScrollbar(LastDizShowScrollbar);
	}
	else
	{
		//не будем менять внутренности если мы посреди операции со вьювером.
		bOK = !DizView->InRecursion;
	}

	if (bOK)
	{
		if (!DizView->OpenFile(DizFile,FALSE))
		{
			delete DizView;
			DizView = nullptr;
			return FALSE;
		}

		strDizFileName = DizFile;
	}

	DizView->Show();
	FARString strTitle;
	strTitle.Append(L" ").Append(PointToName(strDizFileName)).Append(L" ");
	TruncStr(strTitle,X2-X1-3);
	GotoXY(X1+(X2-X1-(int)strTitle.GetLength())/2,YPos);
	SetColor(COL_PANELTEXT);
	PrintText(strTitle);
	return TRUE;
}

void InfoList::SetFocus()
{
	Panel::SetFocus();
	SetMacroMode(FALSE);
}

void InfoList::KillFocus()
{
	Panel::KillFocus();
	SetMacroMode(TRUE);
}

void InfoList::SetMacroMode(int Restore)
{
	if (!CtrlObject)
		return;

	if (PrevMacroMode == -1)
		PrevMacroMode = CtrlObject->Macro.GetMode();

	CtrlObject->Macro.SetMode(Restore ? PrevMacroMode:MACRO_INFOPANEL);
}


int InfoList::GetCurName(FARString &strName)
{
	strName = strDizFileName;
	return (TRUE);
}

BOOL InfoList::UpdateKeyBar()
{
	KeyBar *KB = CtrlObject->MainKeyBar;
	KB->SetAllGroup(KBL_MAIN, MInfoF1, 12);
	KB->SetAllGroup(KBL_SHIFT, MInfoShiftF1, 12);
	KB->SetAllGroup(KBL_ALT, MInfoAltF1, 12);
	KB->SetAllGroup(KBL_CTRL, MInfoCtrlF1, 12);
	KB->SetAllGroup(KBL_CTRLSHIFT, MInfoCtrlShiftF1, 12);
	KB->SetAllGroup(KBL_CTRLALT, MInfoCtrlAltF1, 12);
	KB->SetAllGroup(KBL_ALTSHIFT, MInfoAltShiftF1, 12);
	KB->SetAllGroup(KBL_CTRLALTSHIFT, MInfoCtrlAltShiftF1, 12);
	DynamicUpdateKeyBar();
	return TRUE;
}

void InfoList::DynamicUpdateKeyBar()
{
	KeyBar *KB = CtrlObject->MainKeyBar;

	if (DizView)
	{
		KB->Change(MSG(MInfoF3), 3-1);

		if (DizView->GetCodePage() != WINPORT(GetOEMCP)())
			KB->Change(MSG(MViewF8DOS), 7);
		else
			KB->Change(MSG(MInfoF8), 7);

		if (!DizView->GetWrapMode())
		{
			if (DizView->GetWrapType())
				KB->Change(MSG(MViewShiftF2), 2-1);
			else
				KB->Change(MSG(MViewF2), 2-1);
		}
		else
			KB->Change(MSG(MViewF2Unwrap), 2-1);

		if (DizView->GetWrapType())
			KB->Change(KBL_SHIFT, MSG(MViewF2), 2-1);
		else
			KB->Change(KBL_SHIFT, MSG(MViewShiftF2), 2-1);
	}
	else
	{
		KB->Change(MSG(MF2), 2-1);
		KB->Change(KBL_SHIFT, L"", 2-1);
		KB->Change(L"", 3-1);
		KB->Change(L"", 8-1);
		KB->Change(KBL_SHIFT, L"", 8-1);
		KB->Change(KBL_ALT, MSG(MAltF8), 8-1);  // стандартный для панели - "хистори"
	}

	KB->ReadRegGroup(L"Info",Opt.strLanguage);
	KB->SetAllRegGroup();
}

