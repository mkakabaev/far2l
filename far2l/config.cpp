/*
config.cpp

Конфигурация
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


#include "config.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "colors.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "filelist.hpp"
#include "panel.hpp"
#include "help.hpp"
#include "filefilter.hpp"
#include "poscache.hpp"
#include "findfile.hpp"
#include "hilight.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "palette.hpp"
#include "message.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "panelmix.hpp"
#include "strmix.hpp"
#include "udlist.hpp"
#include "datetime.hpp"
#include "FarDlgBuilder.hpp"
#include "vtshell.h"
#include "ConfigRW.hpp"

Options Opt={0};

// Стандартный набор разделителей
static const wchar_t *WordDiv0 = L"~!%^&*()+|{}:\"<>?`-=\\[];',./";

// Стандартный набор разделителей для функции Xlat
static const wchar_t *WordDivForXlat0=L" \t!#$%^&*()+|=\\/@?";

FARString strKeyNameConsoleDetachKey;
static const wchar_t szCtrlShiftX[]=L"CtrlShiftX";
static const wchar_t szCtrlDot[]=L"Ctrl.";
static const wchar_t szCtrlShiftDot[]=L"CtrlShift.";

// KeyName
static const char NKeyColors[]="Colors";
static const char NKeyScreen[]="Screen";
static const char NKeyCmdline[]="Cmdline";
static const char NKeyInterface[]="Interface";
static const char NKeyInterfaceCompletion[]="Interface/Completion";
static const char NKeyViewer[]="Viewer";
static const char NKeyDialog[]="Dialog";
static const char NKeyEditor[]="Editor";
static const char NKeyNotifications[]="Notifications";
static const char NKeyXLat[]="XLat";
static const char NKeySystem[]="System";
static const char NKeySystemExecutor[]="System/Executor";
static const char NKeySystemNowell[]="System/Nowell";
static const char NKeyHelp[]="Help";
static const char NKeyLanguage[]="Language";
static const char NKeyConfirmations[]="Confirmations";
static const char NKeyPluginConfirmations[]="PluginConfirmations";
static const char NKeyPanel[]="Panel";
static const char NKeyPanelLeft[]="Panel/Left";
static const char NKeyPanelRight[]="Panel/Right";
static const char NKeyPanelLayout[]="Panel/Layout";
static const char NKeyPanelTree[]="Panel/Tree";
static const char NKeyPanelInfo[]="Panel/Info";
static const char NKeyLayout[]="Layout";
static const char NKeyDescriptions[]="Descriptions";
static const char NKeyKeyMacros[]="KeyMacros";
static const char NKeyPolicies[]="Policies";
static const char NKeyFileFilter[]="OperationsFilter";
static const char NKeySavedHistory[]="SavedHistory";
static const char NKeySavedViewHistory[]="SavedViewHistory";
static const char NKeySavedFolderHistory[]="SavedFolderHistory";
static const char NKeySavedDialogHistory[]="SavedDialogHistory";
static const char NKeyCodePages[]="CodePages";
static const char NParamHistoryCount[]="HistoryCount";
static const char NKeyVMenu[]="VMenu";

static const wchar_t *constBatchExt=L".BAT;.CMD;";

static DWORD ApplyConsoleTweaks()
{
	DWORD tweaks = 0;
	if (Opt.ExclusiveCtrlLeft) tweaks|= EXCLUSIVE_CTRL_LEFT;
	if (Opt.ExclusiveCtrlRight) tweaks|= EXCLUSIVE_CTRL_RIGHT;
	if (Opt.ExclusiveAltLeft) tweaks|= EXCLUSIVE_ALT_LEFT;
	if (Opt.ExclusiveAltRight) tweaks|= EXCLUSIVE_ALT_RIGHT;
	if (Opt.ExclusiveWinLeft) tweaks|= EXCLUSIVE_WIN_LEFT;
	if (Opt.ExclusiveWinRight) tweaks|= EXCLUSIVE_WIN_RIGHT;
	if (Opt.ConsolePaintSharp) tweaks|= CONSOLE_PAINT_SHARP;
	return WINPORT(SetConsoleTweaks)(tweaks);
}

static void ApplySudoConfiguration()
{
 	const std::string &sudo_app = GetHelperPathName("far2l_sudoapp");
	const std::string &askpass_app = GetHelperPathName("far2l_askpass");

	SudoClientMode mode;
	if (Opt.SudoEnabled) {
		mode = Opt.SudoConfirmModify ? SCM_CONFIRM_MODIFY : SCM_CONFIRM_NONE;
	} else
		mode = SCM_DISABLE;
	sudo_client_configure(mode, Opt.SudoPasswordExpiration, sudo_app.c_str(), askpass_app.c_str(),
		Wide2MB(MSG(MSudoTitle)).c_str(), Wide2MB(MSG(MSudoPrompt)).c_str(), Wide2MB(MSG(MSudoConfirm)).c_str());
}

static void AddHistorySettings(DialogBuilder &Builder, int MTitle, int *OptEnabled, int *OptCount)
{
	DialogItemEx *EnabledCheckBox = Builder.AddCheckbox(MTitle, OptEnabled);
	DialogItemEx *CountEdit = Builder.AddIntEditField(OptCount, 6);
	DialogItemEx *CountText = Builder.AddTextBefore(CountEdit, MConfigMaxHistoryCount);
	CountEdit->Indent(4);
	CountText->Indent(4);
	Builder.LinkFlags(EnabledCheckBox, CountEdit, DIF_DISABLE);
	Builder.LinkFlags(EnabledCheckBox, CountText, DIF_DISABLE);
}

static void SanitizeHistoryCounts()
{
	Opt.HistoryCount = std::max(Opt.HistoryCount, 16);
	Opt.FoldersHistoryCount = std::max(Opt.FoldersHistoryCount, 16);
	Opt.ViewHistoryCount = std::max(Opt.ViewHistoryCount, 16);
	Opt.DialogsHistoryCount = std::max(Opt.DialogsHistoryCount, 16);
}

void SystemSettings()
{
	DialogBuilder Builder(MConfigSystemTitle, L"SystemSettings");

	DialogItemEx *SudoEnabledItem = Builder.AddCheckbox(MConfigSudoEnabled, &Opt.SudoEnabled);
	DialogItemEx *SudoPasswordExpirationEdit = Builder.AddIntEditField(&Opt.SudoPasswordExpiration, 4);
	DialogItemEx *SudoPasswordExpirationText = Builder.AddTextBefore(SudoPasswordExpirationEdit, MConfigSudoPasswordExpiration);
	
	SudoPasswordExpirationText->Indent(4);
	SudoPasswordExpirationEdit->Indent(4);

	DialogItemEx *SudoConfirmModifyItem = Builder.AddCheckbox(MConfigSudoConfirmModify, &Opt.SudoConfirmModify);
	SudoConfirmModifyItem->Indent(4);

	Builder.LinkFlags(SudoEnabledItem, SudoConfirmModifyItem, DIF_DISABLE);
	Builder.LinkFlags(SudoEnabledItem, SudoPasswordExpirationEdit, DIF_DISABLE);


	DialogItemEx *DeleteToRecycleBin = Builder.AddCheckbox(MConfigRecycleBin, &Opt.DeleteToRecycleBin);
	DialogItemEx *DeleteLinks = Builder.AddCheckbox(MConfigRecycleBinLink, &Opt.DeleteToRecycleBinKillLink);
	DeleteLinks->Indent(4);
	Builder.LinkFlags(DeleteToRecycleBin, DeleteLinks, DIF_DISABLE);


//	Builder.AddCheckbox(MSudoParanoic, &Opt.SudoParanoic);
//	Builder.AddCheckbox(CopyWriteThrough, &Opt.CMOpt.WriteThrough);
	Builder.AddCheckbox(MConfigScanJunction, &Opt.ScanJunction);
	Builder.AddCheckbox(MConfigOnlyFilesSize, &Opt.OnlyFilesSize);

	DialogItemEx *InactivityExit = Builder.AddCheckbox(MConfigInactivity, &Opt.InactivityExit);
	DialogItemEx *InactivityExitTime = Builder.AddIntEditField(&Opt.InactivityExitTime, 2);
	InactivityExitTime->Indent(4);
	Builder.AddTextAfter(InactivityExitTime, MConfigInactivityMinutes);
	Builder.LinkFlags(InactivityExit, InactivityExitTime, DIF_DISABLE);

	AddHistorySettings(Builder, MConfigSaveFoldersHistory, &Opt.SaveFoldersHistory, &Opt.FoldersHistoryCount);
	AddHistorySettings(Builder, MConfigSaveViewHistory, &Opt.SaveViewHistory, &Opt.ViewHistoryCount);

	Builder.AddCheckbox(MConfigAutoSave, &Opt.AutoSaveSetup);
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
		SanitizeHistoryCounts();
		ApplySudoConfiguration();
	}
}


void PanelSettings()
{
	DialogBuilder Builder(MConfigPanelTitle, L"PanelSettings");
	BOOL AutoUpdate = (Opt.AutoUpdateLimit );

	Builder.AddCheckbox(MConfigHidden, &Opt.ShowHidden);
	Builder.AddCheckbox(MConfigHighlight, &Opt.Highlight);
	Builder.AddCheckbox(MConfigAutoChange, &Opt.Tree.AutoChangeFolder);
	Builder.AddCheckbox(MConfigSelectFolders, &Opt.SelectFolders);
	Builder.AddCheckbox(MConfigSortFolderExt, &Opt.SortFolderExt);
	Builder.AddCheckbox(MConfigReverseSort, &Opt.ReverseSort);

	DialogItemEx *AutoUpdateEnabled = Builder.AddCheckbox(MConfigAutoUpdateLimit, &AutoUpdate);
	DialogItemEx *AutoUpdateLimit = Builder.AddIntEditField((int *) &Opt.AutoUpdateLimit, 6);
	Builder.LinkFlags(AutoUpdateEnabled, AutoUpdateLimit, DIF_DISABLE, false);
	DialogItemEx *AutoUpdateText = Builder.AddTextBefore(AutoUpdateLimit, MConfigAutoUpdateLimit2);
	AutoUpdateLimit->Indent(4);
	AutoUpdateText->Indent(4);
	Builder.AddCheckbox(MConfigAutoUpdateRemoteDrive, &Opt.AutoUpdateRemoteDrive);

	Builder.AddSeparator();
	Builder.AddCheckbox(MConfigShowColumns, &Opt.ShowColumnTitles);
	Builder.AddCheckbox(MConfigShowStatus, &Opt.ShowPanelStatus);
	Builder.AddCheckbox(MConfigShowTotal, &Opt.ShowPanelTotals);
	Builder.AddCheckbox(MConfigShowFree, &Opt.ShowPanelFree);
	Builder.AddCheckbox(MConfigShowScrollbar, &Opt.ShowPanelScrollbar);
	Builder.AddCheckbox(MConfigShowScreensNumber, &Opt.ShowScreensNumber);
	Builder.AddCheckbox(MConfigShowSortMode, &Opt.ShowSortMode);
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
		if (!AutoUpdate)
			Opt.AutoUpdateLimit = 0;

	//  FrameManager->RefreshFrame();
		CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
		CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
		CtrlObject->Cp()->Redraw();
	}
}


/* $ 17.12.2001 IS
   Настройка средней кнопки мыши для панелей. Воткнем пока сюда, потом надо
   переехать в специальный диалог по программированию мыши.
*/
void InterfaceSettings()
{
	for (;;) {
		DialogBuilder Builder(MConfigInterfaceTitle, L"InterfSettings");
		
		Builder.AddCheckbox(MConfigClock, &Opt.Clock);
		Builder.AddCheckbox(MConfigViewerEditorClock, &Opt.ViewerEditorClock);
		Builder.AddCheckbox(MConfigMouse, &Opt.Mouse);
		Builder.AddCheckbox(MConfigKeyBar, &Opt.ShowKeyBar);
		Builder.AddCheckbox(MConfigMenuBar, &Opt.ShowMenuBar);
		DialogItemEx *SaverCheckbox = Builder.AddCheckbox(MConfigSaver, &Opt.ScreenSaver);
		
		DialogItemEx *SaverEdit = Builder.AddIntEditField(&Opt.ScreenSaverTime, 2);
		SaverEdit->Indent(4);
		Builder.AddTextAfter(SaverEdit, MConfigSaverMinutes);
		Builder.LinkFlags(SaverCheckbox, SaverEdit, DIF_DISABLE);
		
		Builder.AddCheckbox(MConfigCopyTotal, &Opt.CMOpt.CopyShowTotal);
		Builder.AddCheckbox(MConfigCopyTimeRule, &Opt.CMOpt.CopyTimeRule);
		Builder.AddCheckbox(MConfigDeleteTotal, &Opt.DelOpt.DelShowTotal);
		Builder.AddCheckbox(MConfigPgUpChangeDisk, &Opt.PgUpChangeDisk);
		
		
		DWORD supported_tweaks = ApplyConsoleTweaks();
		int ChangeFontID = -1;
		DialogItemEx *Item = Builder.AddButton(MConfigConsoleChangeFont, ChangeFontID);
		
		if (supported_tweaks & TWEAK_STATUS_SUPPORT_PAINT_SHARP) {
			Builder.AddCheckboxAfter(Item, MConfigConsolePaintSharp, &Opt.ConsolePaintSharp);
		}
		if (supported_tweaks & TWEAK_STATUS_SUPPORT_EXCLUSIVE_KEYS) {
			Builder.AddText(MConfigExclusiveKeys);
			Item = Builder.AddCheckbox(MConfigExclusiveCtrlLeft, &Opt.ExclusiveCtrlLeft);
			Item->Indent(4);
			Builder.AddCheckboxAfter(Item, MConfigExclusiveCtrlRight, &Opt.ExclusiveCtrlRight);
		
			Item = Builder.AddCheckbox(MConfigExclusiveAltLeft, &Opt.ExclusiveAltLeft);
			Item->Indent(4);
			Builder.AddCheckboxAfter(Item, MConfigExclusiveAltRight, &Opt.ExclusiveAltRight);
		
			Item = Builder.AddCheckbox(MConfigExclusiveWinLeft, &Opt.ExclusiveWinLeft);
			Item->Indent(4);
			Builder.AddCheckboxAfter(Item, MConfigExclusiveWinRight, &Opt.ExclusiveWinRight);
		}
		
		Builder.AddText(MConfigTitleAddons);
		Builder.AddEditField(&Opt.strTitleAddons, 47);
		
		//OKButton->Flags = DIF_CENTERGROUP;
		//OKButton->DefaultButton = TRUE;
		//OKButton->Y1 = OKButton->Y2 = NextY++;
		//OKButtonID = DialogItemsCount-1;
		
		
		Builder.AddOKCancel();
		
		int clicked_id = -1;
		if (Builder.ShowDialog(&clicked_id)) {
			if (Opt.CMOpt.CopyTimeRule)
				Opt.CMOpt.CopyTimeRule = 3;
		
			SetFarConsoleMode();
			CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
			CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
			CtrlObject->Cp()->SetScreenPosition();
			// $ 10.07.2001 SKV ! надо это делать, иначе если кейбар спрятали, будет полный рамс.
			CtrlObject->Cp()->Redraw();
			ApplyConsoleTweaks();
			break;
		}
		
		if (clicked_id != ChangeFontID)
			break;
		
		WINPORT(ConsoleChangeFont)();
	}
}

void AutoCompleteSettings()
{
	DialogBuilder Builder(MConfigAutoCompleteTitle, L"AutoCompleteSettings");
	DialogItemEx *ListCheck=Builder.AddCheckbox(MConfigAutoCompleteShowList, &Opt.AutoComplete.ShowList);
	DialogItemEx *ModalModeCheck=Builder.AddCheckbox(MConfigAutoCompleteModalList, &Opt.AutoComplete.ModalList);
	ModalModeCheck->Indent(4);
	Builder.AddCheckbox(MConfigAutoCompleteAutoAppend, &Opt.AutoComplete.AppendCompletion);
	Builder.LinkFlags(ListCheck, ModalModeCheck, DIF_DISABLE);

	Builder.AddText(MConfigAutoCompleteExceptions);
	Builder.AddEditField(&Opt.AutoComplete.Exceptions, 47);
	
	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void InfoPanelSettings()
{

	DialogBuilder Builder(MConfigInfoPanelTitle, L"InfoPanelSettings");
	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void DialogSettings()
{
	DialogBuilder Builder(MConfigDlgSetsTitle, L"DialogSettings");

	AddHistorySettings(Builder, MConfigDialogsEditHistory, &Opt.Dialogs.EditHistory, &Opt.DialogsHistoryCount);
	Builder.AddCheckbox(MConfigDialogsEditBlock, &Opt.Dialogs.EditBlock);
	Builder.AddCheckbox(MConfigDialogsDelRemovesBlocks, &Opt.Dialogs.DelRemovesBlocks);
	Builder.AddCheckbox(MConfigDialogsAutoComplete, &Opt.Dialogs.AutoComplete);
	Builder.AddCheckbox(MConfigDialogsEULBsClear, &Opt.Dialogs.EULBsClear);
	Builder.AddCheckbox(MConfigDialogsMouseButton, &Opt.Dialogs.MouseButton);
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
		SanitizeHistoryCounts();
		if (Opt.Dialogs.MouseButton )
			Opt.Dialogs.MouseButton = 0xFFFF;
	}
}

void VMenuSettings()
{
	DialogBuilderListItem CAListItems[]=
	{
		{ MConfigVMenuClickCancel, VMENUCLICK_CANCEL },  // Cancel menu
		{ MConfigVMenuClickApply,  VMENUCLICK_APPLY  },  // Execute selected item
		{ MConfigVMenuClickIgnore, VMENUCLICK_IGNORE },  // Do nothing
	};

	DialogBuilder Builder(MConfigVMenuTitle, L"VMenuSettings");

	Builder.AddText(MConfigVMenuLBtnClick);
	Builder.AddComboBox((int *) &Opt.VMenu.LBtnClick, 40, CAListItems, ARRAYSIZE(CAListItems), DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE);
	Builder.AddText(MConfigVMenuRBtnClick);
	Builder.AddComboBox((int *) &Opt.VMenu.RBtnClick, 40, CAListItems, ARRAYSIZE(CAListItems), DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE);
	Builder.AddText(MConfigVMenuMBtnClick);
	Builder.AddComboBox((int *) &Opt.VMenu.MBtnClick, 40, CAListItems, ARRAYSIZE(CAListItems), DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE);
	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void CmdlineSettings()
{
	DialogBuilderListItem CMWListItems[]=
	{
		{ MConfigCmdlineWaitKeypress_Never, 0 },
		{ MConfigCmdlineWaitKeypress_OnError,  1 },
		{ MConfigCmdlineWaitKeypress_Always, 2 },
	};

	DialogBuilder Builder(MConfigCmdlineTitle, L"CmdlineSettings");
	AddHistorySettings(Builder, MConfigSaveHistory, &Opt.SaveHistory, &Opt.HistoryCount);
	Builder.AddCheckbox(MConfigCmdlineEditBlock, &Opt.CmdLine.EditBlock);
	Builder.AddCheckbox(MConfigCmdlineDelRemovesBlocks, &Opt.CmdLine.DelRemovesBlocks);
	Builder.AddCheckbox(MConfigCmdlineAutoComplete, &Opt.CmdLine.AutoComplete);

	Builder.AddText(MConfigCmdlineWaitKeypress);
	Builder.AddComboBox((int *) &Opt.CmdLine.WaitKeypress, 40,
		CMWListItems, ARRAYSIZE(CMWListItems), DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE);


	DialogItemEx *UsePromptFormat = Builder.AddCheckbox(MConfigCmdlineUsePromptFormat, &Opt.CmdLine.UsePromptFormat);
	DialogItemEx *PromptFormat = Builder.AddEditField(&Opt.CmdLine.strPromptFormat, 19);
	PromptFormat->Indent(4);
	Builder.LinkFlags(UsePromptFormat, PromptFormat, DIF_DISABLE);
	DialogItemEx *UseShell = Builder.AddCheckbox(MConfigCmdlineUseShell, &Opt.CmdLine.UseShell);
	DialogItemEx *Shell =Builder.AddEditField(&Opt.CmdLine.strShell, 19);
	Shell->Indent(4);
	Builder.LinkFlags(UseShell, Shell, DIF_DISABLE);
	Builder.AddOKCancel();

	int oldUseShell = Opt.CmdLine.UseShell;
	FARString oldShell = FARString(Opt.CmdLine.strShell);

	if (Builder.ShowDialog())
	{
		SanitizeHistoryCounts();

		CtrlObject->CmdLine->SetPersistentBlocks(Opt.CmdLine.EditBlock);
		CtrlObject->CmdLine->SetDelRemovesBlocks(Opt.CmdLine.DelRemovesBlocks);
		CtrlObject->CmdLine->SetAutoComplete(Opt.CmdLine.AutoComplete);

		if (Opt.CmdLine.UseShell != oldUseShell || Opt.CmdLine.strShell != oldShell)
		    VTShell_Shutdown();
	}
}

void SetConfirmations()
{
	DialogBuilder Builder(MSetConfirmTitle, L"ConfirmDlg");

	Builder.AddCheckbox(MSetConfirmCopy, &Opt.Confirm.Copy);
	Builder.AddCheckbox(MSetConfirmMove, &Opt.Confirm.Move);
	Builder.AddCheckbox(MSetConfirmRO, &Opt.Confirm.RO);
	Builder.AddCheckbox(MSetConfirmDelete, &Opt.Confirm.Delete);
	Builder.AddCheckbox(MSetConfirmDeleteFolders, &Opt.Confirm.DeleteFolder);
	Builder.AddCheckbox(MSetConfirmEsc, &Opt.Confirm.Esc);
	Builder.AddCheckbox(MSetConfirmRemoveConnection, &Opt.Confirm.RemoveConnection);
	Builder.AddCheckbox(MSetConfirmRemoveSUBST, &Opt.Confirm.RemoveSUBST);
	Builder.AddCheckbox(MSetConfirmDetachVHD, &Opt.Confirm.DetachVHD);
	Builder.AddCheckbox(MSetConfirmRemoveHotPlug, &Opt.Confirm.RemoveHotPlug);
	Builder.AddCheckbox(MSetConfirmAllowReedit, &Opt.Confirm.AllowReedit);
	Builder.AddCheckbox(MSetConfirmHistoryClear, &Opt.Confirm.HistoryClear);
	Builder.AddCheckbox(MSetConfirmExit, &Opt.Confirm.Exit);
	Builder.AddOKCancel();

	Builder.ShowDialog();
}

void PluginsManagerSettings()
{
	DialogBuilder Builder(MPluginsManagerSettingsTitle, L"PluginsManagerSettings");

	Builder.AddCheckbox(MPluginsManagerOEMPluginsSupport, &Opt.LoadPlug.OEMPluginsSupport);
	Builder.AddCheckbox(MPluginsManagerScanSymlinks, &Opt.LoadPlug.ScanSymlinks);
	Builder.AddText(MPluginsManagerPersonalPath);
	Builder.AddEditField(&Opt.LoadPlug.strPersonalPluginsPath, 45, L"PersPath", DIF_EDITPATH);

	Builder.AddSeparator(MPluginConfirmationTitle);
	DialogItemEx *ConfirmOFP = Builder.AddCheckbox(MPluginsManagerOFP, &Opt.PluginConfirm.OpenFilePlugin);
	ConfirmOFP->Flags|=DIF_3STATE;
	DialogItemEx *StandardAssoc = Builder.AddCheckbox(MPluginsManagerStdAssoc, &Opt.PluginConfirm.StandardAssociation);
	DialogItemEx *EvenIfOnlyOne = Builder.AddCheckbox(MPluginsManagerEvenOne, &Opt.PluginConfirm.EvenIfOnlyOnePlugin);
	StandardAssoc->Indent(2);
	EvenIfOnlyOne->Indent(4);

	Builder.AddCheckbox(MPluginsManagerSFL, &Opt.PluginConfirm.SetFindList);
	Builder.AddCheckbox(MPluginsManagerPF, &Opt.PluginConfirm.Prefix);
	Builder.AddOKCancel();

	Builder.ShowDialog();
}


void SetDizConfig()
{
	DialogBuilder Builder(MCfgDizTitle, L"FileDiz");

	Builder.AddText(MCfgDizListNames);
	Builder.AddEditField(&Opt.Diz.strListNames, 65);
	Builder.AddSeparator();

	Builder.AddCheckbox(MCfgDizSetHidden, &Opt.Diz.SetHidden);
	Builder.AddCheckbox(MCfgDizROUpdate, &Opt.Diz.ROUpdate);
	DialogItemEx *StartPos = Builder.AddIntEditField(&Opt.Diz.StartPos, 2);
	Builder.AddTextAfter(StartPos, MCfgDizStartPos);
	Builder.AddSeparator();

	static int DizOptions[] = { MCfgDizNotUpdate, MCfgDizUpdateIfDisplayed, MCfgDizAlwaysUpdate };
	Builder.AddRadioButtons(&Opt.Diz.UpdateMode, 3, DizOptions);
	Builder.AddSeparator();

	Builder.AddCheckbox(MCfgDizAnsiByDefault, &Opt.Diz.AnsiByDefault);
	Builder.AddCheckbox(MCfgDizSaveInUTF, &Opt.Diz.SaveInUTF);
	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void ViewerConfig(ViewerOptions &ViOpt,bool Local)
{
	DialogBuilder Builder(MViewConfigTitle, L"ViewerSettings");

	if (!Local)
	{
		Builder.AddCheckbox(MViewConfigExternalF3, &Opt.ViOpt.UseExternalViewer);
		Builder.AddText(MViewConfigExternalCommand);
		Builder.AddEditField(&Opt.strExternalViewer, 64, L"ExternalViewer", DIF_EDITPATH);
		Builder.AddSeparator(MViewConfigInternal);
	}

	Builder.StartColumns();
	Builder.AddCheckbox(MViewConfigPersistentSelection, &ViOpt.PersistentBlocks);
	DialogItemEx *SavePos = Builder.AddCheckbox(MViewConfigSavePos, &Opt.ViOpt.SavePos);
	DialogItemEx *TabSize = Builder.AddIntEditField(&ViOpt.TabSize, 3);
	Builder.AddTextAfter(TabSize, MViewConfigTabSize);
	Builder.AddCheckbox(MViewShowKeyBar, &ViOpt.ShowKeyBar);
	Builder.ColumnBreak();
	Builder.AddCheckbox(MViewConfigArrows, &ViOpt.ShowArrows);
	DialogItemEx *SaveShortPos = Builder.AddCheckbox(MViewConfigSaveShortPos, &Opt.ViOpt.SaveShortPos);
	Builder.LinkFlags(SavePos, SaveShortPos, DIF_DISABLE);
	Builder.AddCheckbox(MViewConfigScrollbar, &ViOpt.ShowScrollbar);
	Builder.AddCheckbox(MViewShowTitleBar, &ViOpt.ShowTitleBar);
	Builder.EndColumns();

	if (!Local)
	{
		Builder.AddEmptyLine();
		Builder.AddCheckbox(MViewAutoDetectCodePage, &ViOpt.AutoDetectCodePage);
		Builder.AddText(MViewConfigDefaultCodePage);
		Builder.AddCodePagesBox(&ViOpt.DefaultCodePage, 40, false, false);
	}
	Builder.AddOKCancel();
	if (Builder.ShowDialog())
	{
		if (ViOpt.TabSize<1 || ViOpt.TabSize>512)
			ViOpt.TabSize=8;
	}
}

void EditorConfig(EditorOptions &EdOpt,bool Local)
{
	DialogBuilder Builder(MEditConfigTitle, L"EditorSettings");
	if (!Local)
	{
		Builder.AddCheckbox(MEditConfigEditorF4, &Opt.EdOpt.UseExternalEditor);
		Builder.AddText(MEditConfigEditorCommand);
		Builder.AddEditField(&Opt.strExternalEditor, 64, L"ExternalEditor", DIF_EDITPATH);
		Builder.AddSeparator(MEditConfigInternal);
	}

	Builder.AddText(MEditConfigExpandTabsTitle);
	DialogBuilderListItem ExpandTabsItems[] = {
		{ MEditConfigDoNotExpandTabs, EXPAND_NOTABS },
		{ MEditConfigExpandTabs, EXPAND_NEWTABS },
		{ MEditConfigConvertAllTabsToSpaces, EXPAND_ALLTABS }
	};
	Builder.AddComboBox(&EdOpt.ExpandTabs, 64, ExpandTabsItems, 3, DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE);

	Builder.StartColumns();
	Builder.AddCheckbox(MEditConfigPersistentBlocks, &EdOpt.PersistentBlocks);
	DialogItemEx *SavePos = Builder.AddCheckbox(MEditConfigSavePos, &EdOpt.SavePos);
	Builder.AddCheckbox(MEditConfigAutoIndent, &EdOpt.AutoIndent);
	DialogItemEx *TabSize = Builder.AddIntEditField(&EdOpt.TabSize, 3);
	Builder.AddTextAfter(TabSize, MEditConfigTabSize);
	Builder.AddCheckbox(MEditShowWhiteSpace, &EdOpt.ShowWhiteSpace);
	Builder.AddCheckbox(MEditShowKeyBar, &EdOpt.ShowKeyBar);
	Builder.ColumnBreak();
	Builder.AddCheckbox(MEditConfigDelRemovesBlocks, &EdOpt.DelRemovesBlocks);
	DialogItemEx *SaveShortPos = Builder.AddCheckbox(MEditConfigSaveShortPos, &EdOpt.SaveShortPos);
	Builder.LinkFlags(SavePos, SaveShortPos, DIF_DISABLE);
	Builder.AddCheckbox(MEditCursorBeyondEnd, &EdOpt.CursorBeyondEOL);
	Builder.AddCheckbox(MEditConfigScrollbar, &EdOpt.ShowScrollBar);
	Builder.AddCheckbox(MEditConfigPickUpWord, &EdOpt.SearchPickUpWord);
	Builder.AddCheckbox(MEditShowTitleBar, &EdOpt.ShowTitleBar);
	Builder.EndColumns();

	if (!Local)
	{
		Builder.AddEmptyLine();
		Builder.AddCheckbox(MEditShareWrite, &EdOpt.EditOpenedForWrite);
		Builder.AddCheckbox(MEditLockROFileModification, &EdOpt.ReadOnlyLock, 1);
		Builder.AddCheckbox(MEditWarningBeforeOpenROFile, &EdOpt.ReadOnlyLock, 2);
		Builder.AddCheckbox(MEditAutoDetectCodePage, &EdOpt.AutoDetectCodePage);
		Builder.AddText(MEditConfigDefaultCodePage);
		Builder.AddCodePagesBox(&EdOpt.DefaultCodePage, 40, false, false);
	}

	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
		if (EdOpt.TabSize<1 || EdOpt.TabSize>512)
			EdOpt.TabSize=8;
	}
}


void NotificationsConfig(NotificationsOptions &NotifOpt)
{
	DialogBuilder Builder(MNotifConfigTitle, L"NotificationsSettings");

	Builder.AddCheckbox(MNotifConfigOnFileOperation, &NotifOpt.OnFileOperation);
	Builder.AddCheckbox(MNotifConfigOnConsole, &NotifOpt.OnConsole);
	Builder.AddEmptyLine();
	Builder.AddCheckbox(MNotifConfigOnlyIfBackground, &NotifOpt.OnlyIfBackground);
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
	}
}


void SetFolderInfoFiles()
{
	FARString strFolderInfoFiles;

	if (GetString(MSG(MSetFolderInfoTitle),MSG(MSetFolderInfoNames),L"FolderInfoFiles",
	              Opt.InfoPanel.strFolderInfoFiles,strFolderInfoFiles,L"OptMenu",FIB_ENABLEEMPTY|FIB_BUTTONS))
	{
		Opt.InfoPanel.strFolderInfoFiles = strFolderInfoFiles;

		if (CtrlObject->Cp()->LeftPanel->GetType() == INFO_PANEL)
			CtrlObject->Cp()->LeftPanel->Update(0);

		if (CtrlObject->Cp()->RightPanel->GetType() == INFO_PANEL)
			CtrlObject->Cp()->RightPanel->Update(0);
	}
}


// Структура, описывающая всю конфигурацию(!)
static struct FARConfig
{
	int   IsSave;   // =1 - будет записываться в SaveConfig()
	DWORD ValType;  // REG_DWORD, REG_SZ, REG_BINARY
	const char *KeyName;
	const char *ValName;
	void *ValPtr;   // адрес переменной, куда помещаем данные
	DWORD DefDWord; // он же размер данных для REG_SZ и REG_BINARY
	const wchar_t *DefStr;   // строка/данные по умолчанию
} CFG[]=
{
	{1, REG_BINARY, NKeyColors, "CurrentPalette",(char*)Palette,(DWORD)SizeArrayPalette,(wchar_t*)DefaultPalette},

	{1, REG_DWORD,  NKeyScreen, "Clock", &Opt.Clock, 1, 0},
	{1, REG_DWORD,  NKeyScreen, "ViewerEditorClock",&Opt.ViewerEditorClock,0, 0},
	{1, REG_DWORD,  NKeyScreen, "KeyBar",&Opt.ShowKeyBar,1, 0},
	{1, REG_DWORD,  NKeyScreen, "ScreenSaver",&Opt.ScreenSaver, 0, 0},
	{1, REG_DWORD,  NKeyScreen, "ScreenSaverTime",&Opt.ScreenSaverTime,5, 0},
	{0, REG_DWORD,  NKeyScreen, "DeltaXY", &Opt.ScrSize.DeltaXY, 0, 0},

	{1, REG_DWORD,  NKeyCmdline, "UsePromptFormat", &Opt.CmdLine.UsePromptFormat,0, 0},
	{1, REG_SZ,     NKeyCmdline, "PromptFormat",&Opt.CmdLine.strPromptFormat, 0, L"$p$# "},
	{1, REG_DWORD,  NKeyCmdline, "UseShell",&Opt.CmdLine.UseShell, 0, 0},
	{1, REG_SZ,     NKeyCmdline, "Shell",&Opt.CmdLine.strShell, 0, L"/bin/bash"},
	{1, REG_DWORD,  NKeyCmdline, "DelRemovesBlocks", &Opt.CmdLine.DelRemovesBlocks,1, 0},
	{1, REG_DWORD,  NKeyCmdline, "EditBlock", &Opt.CmdLine.EditBlock,0, 0},
	{1, REG_DWORD,  NKeyCmdline, "AutoComplete",&Opt.CmdLine.AutoComplete,1, 0},
	{1, REG_DWORD,  NKeyCmdline, "WaitKeypress",&Opt.CmdLine.WaitKeypress,1, 0},

	{1, REG_DWORD,  NKeyInterface, "Mouse",&Opt.Mouse,1, 0},
	{0, REG_DWORD,  NKeyInterface, "UseVk_oem_x",&Opt.UseVk_oem_x,1, 0},
	{1, REG_DWORD,  NKeyInterface, "ShowMenuBar",&Opt.ShowMenuBar,0, 0},
	{0, REG_DWORD,  NKeyInterface, "CursorSize1",&Opt.CursorSize[0],15, 0},
	{0, REG_DWORD,  NKeyInterface, "CursorSize2",&Opt.CursorSize[1],10, 0},
	{0, REG_DWORD,  NKeyInterface, "CursorSize3",&Opt.CursorSize[2],99, 0},
	{0, REG_DWORD,  NKeyInterface, "CursorSize4",&Opt.CursorSize[3],99, 0},
	{0, REG_DWORD,  NKeyInterface, "ShiftsKeyRules",&Opt.ShiftsKeyRules,1, 0},
	{1, REG_DWORD,  NKeyInterface, "CtrlPgUp",&Opt.PgUpChangeDisk, 1, 0},

	{1, REG_DWORD,  NKeyInterface, "ConsolePaintSharp",&Opt.ConsolePaintSharp, 0, 0},
	{1, REG_DWORD,  NKeyInterface, "ExclusiveCtrlLeft",&Opt.ExclusiveCtrlLeft, 0, 0},
	{1, REG_DWORD,  NKeyInterface, "ExclusiveCtrlRight",&Opt.ExclusiveCtrlRight, 0, 0},
	{1, REG_DWORD,  NKeyInterface, "ExclusiveAltLeft",&Opt.ExclusiveAltLeft, 0, 0},
	{1, REG_DWORD,  NKeyInterface, "ExclusiveAltRight",&Opt.ExclusiveAltRight, 0, 0},
	{1, REG_DWORD,  NKeyInterface, "ExclusiveWinLeft",&Opt.ExclusiveWinLeft, 0, 0},
	{1, REG_DWORD,  NKeyInterface, "ExclusiveWinRight",&Opt.ExclusiveWinRight, 0, 0},

	{0, REG_DWORD,  NKeyInterface, "ShowTimeoutDelFiles",&Opt.ShowTimeoutDelFiles, 50, 0},
	{0, REG_DWORD,  NKeyInterface, "ShowTimeoutDACLFiles",&Opt.ShowTimeoutDACLFiles, 50, 0},
	{0, REG_DWORD,  NKeyInterface, "FormatNumberSeparators",&Opt.FormatNumberSeparators, 0, 0},
	{1, REG_DWORD,  NKeyInterface, "CopyShowTotal",&Opt.CMOpt.CopyShowTotal,1, 0},
	{1, REG_DWORD,  NKeyInterface, "DelShowTotal",&Opt.DelOpt.DelShowTotal,0, 0},
	{1, REG_SZ,     NKeyInterface, "TitleAddons",&Opt.strTitleAddons, 0, L"%Ver %Build %Platform %User@%Host"},
	{1, REG_SZ,     NKeyInterfaceCompletion, "Exceptions",&Opt.AutoComplete.Exceptions, 0, L"git*reset*--hard;*://*:*@*"},
	{1, REG_DWORD,  NKeyInterfaceCompletion, "ShowList",&Opt.AutoComplete.ShowList, 1, 0},
	{1, REG_DWORD,  NKeyInterfaceCompletion, "ModalList",&Opt.AutoComplete.ModalList, 0, 0},
	{1, REG_DWORD,  NKeyInterfaceCompletion, "Append",&Opt.AutoComplete.AppendCompletion, 0, 0},

	{1, REG_SZ,     NKeyViewer, "ExternalViewerName",&Opt.strExternalViewer, 0, L""},
	{1, REG_DWORD,  NKeyViewer, "UseExternalViewer",&Opt.ViOpt.UseExternalViewer,0, 0},
	{1, REG_DWORD,  NKeyViewer, "SaveViewerPos",&Opt.ViOpt.SavePos,1, 0},
	{1, REG_DWORD,  NKeyViewer, "SaveViewerShortPos",&Opt.ViOpt.SaveShortPos,1, 0},
	{1, REG_DWORD,  NKeyViewer, "AutoDetectCodePage",&Opt.ViOpt.AutoDetectCodePage,0, 0},
	{1, REG_DWORD,  NKeyViewer, "SearchRegexp",&Opt.ViOpt.SearchRegexp,0, 0},

	{1, REG_DWORD,  NKeyViewer, "TabSize",&Opt.ViOpt.TabSize,8, 0},
	{1, REG_DWORD,  NKeyViewer, "ShowKeyBar",&Opt.ViOpt.ShowKeyBar,1, 0},
	{1, REG_DWORD,  NKeyViewer, "ShowTitleBar",&Opt.ViOpt.ShowTitleBar,1, 0},
	{1, REG_DWORD,  NKeyViewer, "ShowArrows",&Opt.ViOpt.ShowArrows,1, 0},
	{1, REG_DWORD,  NKeyViewer, "ShowScrollbar",&Opt.ViOpt.ShowScrollbar,0, 0},
	{1, REG_DWORD,  NKeyViewer, "IsWrap",&Opt.ViOpt.ViewerIsWrap,1, 0},
	{1, REG_DWORD,  NKeyViewer, "Wrap",&Opt.ViOpt.ViewerWrap,0, 0},
	{1, REG_DWORD,  NKeyViewer, "PersistentBlocks",&Opt.ViOpt.PersistentBlocks,0, 0},
	{1, REG_DWORD,  NKeyViewer, "DefaultCodePage",&Opt.ViOpt.DefaultCodePage,CP_UTF8, 0},

	{1, REG_DWORD,  NKeyDialog, "EditHistory",&Opt.Dialogs.EditHistory,1, 0},
	{1, REG_DWORD,  NKeyDialog, "EditBlock",&Opt.Dialogs.EditBlock,0, 0},
	{1, REG_DWORD,  NKeyDialog, "AutoComplete",&Opt.Dialogs.AutoComplete,1, 0},
	{1, REG_DWORD,  NKeyDialog, "EULBsClear",&Opt.Dialogs.EULBsClear,0, 0},
	{0, REG_DWORD,  NKeyDialog, "SelectFromHistory",&Opt.Dialogs.SelectFromHistory,0, 0},
	{0, REG_DWORD,  NKeyDialog, "EditLine",&Opt.Dialogs.EditLine,0, 0},
	{1, REG_DWORD,  NKeyDialog, "MouseButton",&Opt.Dialogs.MouseButton,0xFFFF, 0},
	{1, REG_DWORD,  NKeyDialog, "DelRemovesBlocks",&Opt.Dialogs.DelRemovesBlocks,1, 0},
	{0, REG_DWORD,  NKeyDialog, "CBoxMaxHeight",&Opt.Dialogs.CBoxMaxHeight,24, 0},

	{1, REG_SZ,     NKeyEditor, "ExternalEditorName",&Opt.strExternalEditor, 0, L""},
	{1, REG_DWORD,  NKeyEditor, "UseExternalEditor",&Opt.EdOpt.UseExternalEditor,0, 0},
	{1, REG_DWORD,  NKeyEditor, "ExpandTabs",&Opt.EdOpt.ExpandTabs,0, 0},
	{1, REG_DWORD,  NKeyEditor, "TabSize",&Opt.EdOpt.TabSize,8, 0},
	{1, REG_DWORD,  NKeyEditor, "PersistentBlocks",&Opt.EdOpt.PersistentBlocks,0, 0},
	{1, REG_DWORD,  NKeyEditor, "DelRemovesBlocks",&Opt.EdOpt.DelRemovesBlocks,1, 0},
	{1, REG_DWORD,  NKeyEditor, "AutoIndent",&Opt.EdOpt.AutoIndent,0, 0},
	{1, REG_DWORD,  NKeyEditor, "SaveEditorPos",&Opt.EdOpt.SavePos,1, 0},
	{1, REG_DWORD,  NKeyEditor, "SaveEditorShortPos",&Opt.EdOpt.SaveShortPos,1, 0},
	{1, REG_DWORD,  NKeyEditor, "AutoDetectCodePage",&Opt.EdOpt.AutoDetectCodePage,0, 0},
	{1, REG_DWORD,  NKeyEditor, "EditorCursorBeyondEOL",&Opt.EdOpt.CursorBeyondEOL,1, 0},
	{1, REG_DWORD,  NKeyEditor, "ReadOnlyLock",&Opt.EdOpt.ReadOnlyLock,0, 0}, // Вернём назад дефолт 1.65 - не предупреждать и не блокировать
	{0, REG_DWORD,  NKeyEditor, "EditorUndoSize",&Opt.EdOpt.UndoSize,0, 0}, // $ 03.12.2001 IS размер буфера undo в редакторе
	{0, REG_SZ,     NKeyEditor, "WordDiv",&Opt.strWordDiv, 0, WordDiv0},
	{0, REG_DWORD,  NKeyEditor, "BSLikeDel",&Opt.EdOpt.BSLikeDel,1, 0},
	{0, REG_DWORD,  NKeyEditor, "EditorF7Rules",&Opt.EdOpt.F7Rules,1, 0},
	{0, REG_DWORD,  NKeyEditor, "FileSizeLimit",&Opt.EdOpt.FileSizeLimitLo,(DWORD)0, 0},
	{0, REG_DWORD,  NKeyEditor, "FileSizeLimitHi",&Opt.EdOpt.FileSizeLimitHi,(DWORD)0, 0},
	{0, REG_DWORD,  NKeyEditor, "CharCodeBase",&Opt.EdOpt.CharCodeBase,1, 0},
	{0, REG_DWORD,  NKeyEditor, "AllowEmptySpaceAfterEof", &Opt.EdOpt.AllowEmptySpaceAfterEof,0,0},//skv
	{1, REG_DWORD,  NKeyEditor, "DefaultCodePage",&Opt.EdOpt.DefaultCodePage,CP_UTF8, 0},
	{1, REG_DWORD,  NKeyEditor, "ShowKeyBar",&Opt.EdOpt.ShowKeyBar,1, 0},
	{1, REG_DWORD,  NKeyEditor, "ShowTitleBar",&Opt.EdOpt.ShowTitleBar,1, 0},
	{1, REG_DWORD,  NKeyEditor, "ShowScrollBar",&Opt.EdOpt.ShowScrollBar,0, 0},
	{1, REG_DWORD,  NKeyEditor, "EditOpenedForWrite",&Opt.EdOpt.EditOpenedForWrite,1, 0},
	{1, REG_DWORD,  NKeyEditor, "SearchSelFound",&Opt.EdOpt.SearchSelFound,0, 0},
	{1, REG_DWORD,  NKeyEditor, "SearchRegexp",&Opt.EdOpt.SearchRegexp,0, 0},
	{1, REG_DWORD,  NKeyEditor, "SearchPickUpWord",&Opt.EdOpt.SearchPickUpWord,0, 0},
	{1, REG_DWORD,  NKeyEditor, "ShowWhiteSpace",&Opt.EdOpt.ShowWhiteSpace,0, 0},

	{1, REG_DWORD,  NKeyNotifications, "OnFileOperation",&Opt.NotifOpt.OnFileOperation,1, 0},
	{1, REG_DWORD,  NKeyNotifications, "OnConsole",&Opt.NotifOpt.OnConsole,1, 0},
	{1, REG_DWORD,  NKeyNotifications, "OnlyIfBackground",&Opt.NotifOpt.OnlyIfBackground,1, 0},

	{0, REG_DWORD,  NKeyXLat, "Flags",&Opt.XLat.Flags,(DWORD)XLAT_SWITCHKEYBLAYOUT|XLAT_CONVERTALLCMDLINE, 0},
	{0, REG_SZ,     NKeyXLat, "Table1",&Opt.XLat.Table[0],0,L""},
	{0, REG_SZ,     NKeyXLat, "Table2",&Opt.XLat.Table[1],0,L""},
	{0, REG_SZ,     NKeyXLat, "Rules1",&Opt.XLat.Rules[0],0,L""},
	{0, REG_SZ,     NKeyXLat, "Rules2",&Opt.XLat.Rules[1],0,L""},
	{0, REG_SZ,     NKeyXLat, "Rules3",&Opt.XLat.Rules[2],0,L""},
	{0, REG_SZ,     NKeyXLat, "WordDivForXlat",&Opt.XLat.strWordDivForXlat, 0,WordDivForXlat0},

	{1, REG_DWORD,  NKeySavedHistory, NParamHistoryCount,&Opt.HistoryCount,512, 0},
	{1, REG_DWORD,  NKeySavedFolderHistory, NParamHistoryCount,&Opt.FoldersHistoryCount,512, 0},
	{1, REG_DWORD,  NKeySavedViewHistory, NParamHistoryCount,&Opt.ViewHistoryCount,512, 0},
	{1, REG_DWORD,  NKeySavedDialogHistory, NParamHistoryCount,&Opt.DialogsHistoryCount,512, 0},

	{1, REG_DWORD,  NKeySystem, "SaveHistory",&Opt.SaveHistory,1, 0},
	{1, REG_DWORD,  NKeySystem, "SaveFoldersHistory",&Opt.SaveFoldersHistory,1, 0},
	{0, REG_DWORD,  NKeySystem, "SavePluginFoldersHistory",&Opt.SavePluginFoldersHistory,0, 0},
	{1, REG_DWORD,  NKeySystem, "SaveViewHistory",&Opt.SaveViewHistory,1, 0},
	{1, REG_DWORD,  NKeySystem, "AutoSaveSetup",&Opt.AutoSaveSetup,0, 0},
	{1, REG_DWORD,  NKeySystem, "DeleteToRecycleBin",&Opt.DeleteToRecycleBin,0, 0},
	{1, REG_DWORD,  NKeySystem, "DeleteToRecycleBinKillLink",&Opt.DeleteToRecycleBinKillLink,1, 0},
	{0, REG_DWORD,  NKeySystem, "WipeSymbol",&Opt.WipeSymbol,0, 0},
	{1, REG_DWORD,  NKeySystem, "SudoEnabled",&Opt.SudoEnabled,1, 0},
	{1, REG_DWORD,  NKeySystem, "SudoConfirmModify",&Opt.SudoConfirmModify,1, 0},
	{1, REG_DWORD,  NKeySystem, "SudoPasswordExpiration",&Opt.SudoPasswordExpiration,15*60, 0},

	{1, REG_DWORD,  NKeySystem, "UseCOW",&Opt.CMOpt.SparseFiles, 0, 0},
	{1, REG_DWORD,  NKeySystem, "SparseFiles",&Opt.CMOpt.SparseFiles, 0, 0},
	{1, REG_DWORD,  NKeySystem, "HowCopySymlink",&Opt.CMOpt.HowCopySymlink, 1, 0},
	{1, REG_DWORD,  NKeySystem, "WriteThrough",&Opt.CMOpt.WriteThrough, 0, 0},
	{1, REG_DWORD,  NKeySystem, "CopyXAttr",&Opt.CMOpt.CopyXAttr, 0, 0},
	{0, REG_DWORD,  NKeySystem, "CopyAccessMode",&Opt.CMOpt.CopyAccessMode,1, 0},
	{1, REG_DWORD,  NKeySystem, "MultiCopy",&Opt.CMOpt.MultiCopy,0, 0},
	{1, REG_DWORD,  NKeySystem, "CopyTimeRule",  &Opt.CMOpt.CopyTimeRule, 3, 0},

	{1, REG_DWORD,  NKeySystem, "InactivityExit",&Opt.InactivityExit,0, 0},
	{1, REG_DWORD,  NKeySystem, "InactivityExitTime",&Opt.InactivityExitTime,15, 0},
	{1, REG_DWORD,  NKeySystem, "DriveMenuMode",&Opt.ChangeDriveMode,DRIVE_SHOW_TYPE|DRIVE_SHOW_PLUGINS|DRIVE_SHOW_SIZE_FLOAT|DRIVE_SHOW_BOOKMARKS, 0},
	{1, REG_DWORD,  NKeySystem, "DriveDisconnetMode",&Opt.ChangeDriveDisconnetMode,1, 0},
	{1, REG_DWORD,  NKeySystem, "AutoUpdateRemoteDrive",&Opt.AutoUpdateRemoteDrive,1, 0},
	{1, REG_DWORD,  NKeySystem, "FileSearchMode",&Opt.FindOpt.FileSearchMode,FINDAREA_FROM_CURRENT, 0},
	{0, REG_DWORD,  NKeySystem, "CollectFiles",&Opt.FindOpt.CollectFiles, 1, 0},
	{1, REG_SZ,     NKeySystem, "SearchInFirstSize",&Opt.FindOpt.strSearchInFirstSize, 0, L""},
	{1, REG_DWORD,  NKeySystem, "FindAlternateStreams",&Opt.FindOpt.FindAlternateStreams,0,0},
	{1, REG_SZ,     NKeySystem, "SearchOutFormat",&Opt.FindOpt.strSearchOutFormat, 0, L"D,S,A"},
	{1, REG_SZ,     NKeySystem, "SearchOutFormatWidth",&Opt.FindOpt.strSearchOutFormatWidth, 0, L"14,13,0"},
	{1, REG_DWORD,  NKeySystem, "FindFolders",&Opt.FindOpt.FindFolders, 1, 0},
	{1, REG_DWORD,  NKeySystem, "FindSymLinks",&Opt.FindOpt.FindSymLinks, 1, 0},
	{1, REG_DWORD,  NKeySystem, "UseFilterInSearch",&Opt.FindOpt.UseFilter,0,0},
	{1, REG_DWORD,  NKeySystem, "FindCodePage",&Opt.FindCodePage, CP_AUTODETECT, 0},
	{0, REG_DWORD,  NKeySystem, "CmdHistoryRule",&Opt.CmdHistoryRule,0, 0},
	{0, REG_DWORD,  NKeySystem, "SetAttrFolderRules",&Opt.SetAttrFolderRules,1, 0},
	{0, REG_DWORD,  NKeySystem, "MaxPositionCache",&Opt.MaxPositionCache,POSCACHE_MAX_ELEMENTS, 0},
	{0, REG_SZ,     NKeySystem, "ConsoleDetachKey", &strKeyNameConsoleDetachKey, 0, L"CtrlAltTab"},
	{0, REG_DWORD,  NKeySystem, "SilentLoadPlugin",  &Opt.LoadPlug.SilentLoadPlugin, 0, 0},
	{1, REG_DWORD,  NKeySystem, "OEMPluginsSupport",  &Opt.LoadPlug.OEMPluginsSupport, 1, 0},
	{1, REG_DWORD,  NKeySystem, "ScanSymlinks",  &Opt.LoadPlug.ScanSymlinks, 1, 0},
	{1, REG_DWORD,  NKeySystem, "MultiMakeDir",&Opt.MultiMakeDir,0, 0},
	{0, REG_DWORD,  NKeySystem, "MsWheelDelta", &Opt.MsWheelDelta, 1, 0},
	{0, REG_DWORD,  NKeySystem, "MsWheelDeltaView", &Opt.MsWheelDeltaView, 1, 0},
	{0, REG_DWORD,  NKeySystem, "MsWheelDeltaEdit", &Opt.MsWheelDeltaEdit, 1, 0},
	{0, REG_DWORD,  NKeySystem, "MsWheelDeltaHelp", &Opt.MsWheelDeltaHelp, 1, 0},
	{0, REG_DWORD,  NKeySystem, "MsHWheelDelta", &Opt.MsHWheelDelta, 1, 0},
	{0, REG_DWORD,  NKeySystem, "MsHWheelDeltaView", &Opt.MsHWheelDeltaView, 1, 0},
	{0, REG_DWORD,  NKeySystem, "MsHWheelDeltaEdit", &Opt.MsHWheelDeltaEdit, 1, 0},
	{0, REG_DWORD,  NKeySystem, "SubstNameRule", &Opt.SubstNameRule, 2, 0},
	{0, REG_DWORD,  NKeySystem, "ShowCheckingFile", &Opt.ShowCheckingFile, 0, 0},
	{0, REG_DWORD,  NKeySystem, "DelThreadPriority", &Opt.DelThreadPriority, 0, 0},
	{0, REG_SZ,     NKeySystem, "QuotedSymbols",&Opt.strQuotedSymbols, 0, L" $&()[]{};|*?!'`\"\\\xA0"}, //xA0 => 160 =>oem(0xFF)
	{0, REG_DWORD,  NKeySystem, "QuotedName",&Opt.QuotedName,QUOTEDNAME_INSERT, 0},
	//{0, REG_DWORD,  NKeySystem, "CPAJHefuayor",&Opt.strCPAJHefuayor,0, 0},
	{0, REG_DWORD,  NKeySystem, "CloseConsoleRule",&Opt.CloseConsoleRule,1, 0},
	{0, REG_DWORD,  NKeySystem, "PluginMaxReadData",&Opt.PluginMaxReadData,0x40000, 0},
	{0, REG_DWORD,  NKeySystem, "UseNumPad",&Opt.UseNumPad,1, 0},
	{0, REG_DWORD,  NKeySystem, "CASRule",&Opt.CASRule,0xFFFFFFFFU, 0},
	{0, REG_DWORD,  NKeySystem, "AllCtrlAltShiftRule",&Opt.AllCtrlAltShiftRule,0x0000FFFF, 0},
	{1, REG_DWORD,  NKeySystem, "ScanJunction",&Opt.ScanJunction,1, 0},
	{1, REG_DWORD,  NKeySystem, "OnlyFilesSize",&Opt.OnlyFilesSize, 0, 0},
	{0, REG_DWORD,  NKeySystem, "UsePrintManager",&Opt.UsePrintManager,1, 0},
	{0, REG_DWORD,  NKeySystem, "WindowMode",&Opt.WindowMode, 0, 0},

	{0, REG_DWORD,  NKeySystemNowell, "MoveRO",&Opt.Nowell.MoveRO,1, 0},

	{0, REG_DWORD,  NKeySystemExecutor, "RestoreCP",&Opt.RestoreCPAfterExecute,1, 0},
	{0, REG_DWORD,  NKeySystemExecutor, "UseAppPath",&Opt.ExecuteUseAppPath,1, 0},
	{0, REG_DWORD,  NKeySystemExecutor, "ShowErrorMessage",&Opt.ExecuteShowErrorMessage,1, 0},
	{0, REG_SZ,     NKeySystemExecutor, "BatchType",&Opt.strExecuteBatchType,0,constBatchExt},
	{0, REG_DWORD,  NKeySystemExecutor, "FullTitle",&Opt.ExecuteFullTitle,0, 0},
	{0, REG_DWORD,  NKeySystemExecutor, "SilentExternal",&Opt.ExecuteSilentExternal,0, 0},

	{0, REG_DWORD,  NKeyPanelTree, "MinTreeCount",&Opt.Tree.MinTreeCount, 4, 0},
	{0, REG_DWORD,  NKeyPanelTree, "TreeFileAttr",&Opt.Tree.TreeFileAttr, FILE_ATTRIBUTE_HIDDEN, 0},
	{0, REG_DWORD,  NKeyPanelTree, "LocalDisk",&Opt.Tree.LocalDisk, 2, 0},
	{0, REG_DWORD,  NKeyPanelTree, "NetDisk",&Opt.Tree.NetDisk, 2, 0},
	{0, REG_DWORD,  NKeyPanelTree, "RemovableDisk",&Opt.Tree.RemovableDisk, 2, 0},
	{0, REG_DWORD,  NKeyPanelTree, "NetPath",&Opt.Tree.NetPath, 2, 0},
	{1, REG_DWORD,  NKeyPanelTree, "AutoChangeFolder",&Opt.Tree.AutoChangeFolder,0, 0}, // ???

	{0, REG_DWORD,  NKeyHelp, "ActivateURL",&Opt.HelpURLRules,1, 0},

	{1, REG_SZ,     NKeyLanguage, "Help",&Opt.strHelpLanguage, 0, L"English"},
	{1, REG_SZ,     NKeyLanguage, "Main",&Opt.strLanguage, 0, L"English"},

	{1, REG_DWORD,  NKeyConfirmations, "Copy",&Opt.Confirm.Copy,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "Move",&Opt.Confirm.Move,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "RO",&Opt.Confirm.RO,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "Drag",&Opt.Confirm.Drag,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "Delete",&Opt.Confirm.Delete,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "DeleteFolder",&Opt.Confirm.DeleteFolder,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "Esc",&Opt.Confirm.Esc,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "RemoveConnection",&Opt.Confirm.RemoveConnection,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "RemoveSUBST",&Opt.Confirm.RemoveSUBST,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "DetachVHD",&Opt.Confirm.DetachVHD,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "RemoveHotPlug",&Opt.Confirm.RemoveHotPlug,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "AllowReedit",&Opt.Confirm.AllowReedit,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "HistoryClear",&Opt.Confirm.HistoryClear,1, 0},
	{1, REG_DWORD,  NKeyConfirmations, "Exit",&Opt.Confirm.Exit,1, 0},
	{0, REG_DWORD,  NKeyConfirmations, "EscTwiceToInterrupt",&Opt.Confirm.EscTwiceToInterrupt,0, 0},

	{1, REG_DWORD,  NKeyPluginConfirmations,  "OpenFilePlugin", &Opt.PluginConfirm.OpenFilePlugin, 0, 0},
	{1, REG_DWORD,  NKeyPluginConfirmations,  "StandardAssociation", &Opt.PluginConfirm.StandardAssociation, 0, 0},
	{1, REG_DWORD,  NKeyPluginConfirmations,  "EvenIfOnlyOnePlugin", &Opt.PluginConfirm.EvenIfOnlyOnePlugin, 0, 0},
	{1, REG_DWORD,  NKeyPluginConfirmations,  "SetFindList", &Opt.PluginConfirm.SetFindList, 0, 0},
	{1, REG_DWORD,  NKeyPluginConfirmations,  "Prefix", &Opt.PluginConfirm.Prefix, 0, 0},

	{0, REG_DWORD,  NKeyPanel, "ShellRightLeftArrowsRule",&Opt.ShellRightLeftArrowsRule,0, 0},
	{1, REG_DWORD,  NKeyPanel, "ShowHidden",&Opt.ShowHidden,1, 0},
	{1, REG_DWORD,  NKeyPanel, "Highlight",&Opt.Highlight,1, 0},
	{1, REG_DWORD,  NKeyPanel, "SortFolderExt",&Opt.SortFolderExt,0, 0},
	{1, REG_DWORD,  NKeyPanel, "SelectFolders",&Opt.SelectFolders,0, 0},
	{1, REG_DWORD,  NKeyPanel, "ReverseSort",&Opt.ReverseSort,1, 0},
	{0, REG_DWORD,  NKeyPanel, "RightClickRule",&Opt.PanelRightClickRule,2, 0},
	{0, REG_DWORD,  NKeyPanel, "CtrlFRule",&Opt.PanelCtrlFRule,1, 0},
	{0, REG_DWORD,  NKeyPanel, "CtrlAltShiftRule",&Opt.PanelCtrlAltShiftRule,0, 0},
	{0, REG_DWORD,  NKeyPanel, "RememberLogicalDrives",&Opt.RememberLogicalDrives, 0, 0},
	{1, REG_DWORD,  NKeyPanel, "AutoUpdateLimit",&Opt.AutoUpdateLimit, 0, 0},

	{1, REG_DWORD,  NKeyPanelLeft, "Type",&Opt.LeftPanel.Type,0, 0},
	{1, REG_DWORD,  NKeyPanelLeft, "Visible",&Opt.LeftPanel.Visible,1, 0},
	{1, REG_DWORD,  NKeyPanelLeft, "Focus",&Opt.LeftPanel.Focus,1, 0},
	{1, REG_DWORD,  NKeyPanelLeft, "ViewMode",&Opt.LeftPanel.ViewMode,2, 0},
	{1, REG_DWORD,  NKeyPanelLeft, "SortMode",&Opt.LeftPanel.SortMode,1, 0},
	{1, REG_DWORD,  NKeyPanelLeft, "SortOrder",&Opt.LeftPanel.SortOrder,1, 0},
	{1, REG_DWORD,  NKeyPanelLeft, "SortGroups",&Opt.LeftPanel.SortGroups,0, 0},
	{1, REG_DWORD,  NKeyPanelLeft, "NumericSort",&Opt.LeftPanel.NumericSort,0, 0},
	{1, REG_DWORD,  NKeyPanelLeft, "CaseSensitiveSortNix",&Opt.LeftPanel.CaseSensitiveSort,1, 0},
	{1, REG_SZ,     NKeyPanelLeft, "Folder",&Opt.strLeftFolder, 0, L""},
	{1, REG_SZ,     NKeyPanelLeft, "CurFile",&Opt.strLeftCurFile, 0, L""},
	{1, REG_DWORD,  NKeyPanelLeft, "SelectedFirst",&Opt.LeftSelectedFirst,0,0},
	{1, REG_DWORD,  NKeyPanelLeft, "DirectoriesFirst",&Opt.LeftPanel.DirectoriesFirst,1,0},

	{1, REG_DWORD,  NKeyPanelRight, "Type",&Opt.RightPanel.Type,0, 0},
	{1, REG_DWORD,  NKeyPanelRight, "Visible",&Opt.RightPanel.Visible,1, 0},
	{1, REG_DWORD,  NKeyPanelRight, "Focus",&Opt.RightPanel.Focus,0, 0},
	{1, REG_DWORD,  NKeyPanelRight, "ViewMode",&Opt.RightPanel.ViewMode,2, 0},
	{1, REG_DWORD,  NKeyPanelRight, "SortMode",&Opt.RightPanel.SortMode,1, 0},
	{1, REG_DWORD,  NKeyPanelRight, "SortOrder",&Opt.RightPanel.SortOrder,1, 0},
	{1, REG_DWORD,  NKeyPanelRight, "SortGroups",&Opt.RightPanel.SortGroups,0, 0},
	{1, REG_DWORD,  NKeyPanelRight, "NumericSort",&Opt.RightPanel.NumericSort,0, 0},
	{1, REG_DWORD,  NKeyPanelRight, "CaseSensitiveSortNix",&Opt.RightPanel.CaseSensitiveSort,1, 0},
	{1, REG_SZ,     NKeyPanelRight, "Folder",&Opt.strRightFolder, 0,L""},
	{1, REG_SZ,     NKeyPanelRight, "CurFile",&Opt.strRightCurFile, 0,L""},
	{1, REG_DWORD,  NKeyPanelRight, "SelectedFirst",&Opt.RightSelectedFirst,0, 0},
	{1, REG_DWORD,  NKeyPanelRight, "DirectoriesFirst",&Opt.RightPanel.DirectoriesFirst,1,0},

	{1, REG_DWORD,  NKeyPanelLayout, "ColumnTitles",&Opt.ShowColumnTitles,1, 0},
	{1, REG_DWORD,  NKeyPanelLayout, "StatusLine",&Opt.ShowPanelStatus,1, 0},
	{1, REG_DWORD,  NKeyPanelLayout, "TotalInfo",&Opt.ShowPanelTotals,1, 0},
	{1, REG_DWORD,  NKeyPanelLayout, "FreeInfo",&Opt.ShowPanelFree,0, 0},
	{1, REG_DWORD,  NKeyPanelLayout, "Scrollbar",&Opt.ShowPanelScrollbar,0, 0},
	{0, REG_DWORD,  NKeyPanelLayout, "ScrollbarMenu",&Opt.ShowMenuScrollbar,1, 0},
	{1, REG_DWORD,  NKeyPanelLayout, "ScreensNumber",&Opt.ShowScreensNumber,1, 0},
	{1, REG_DWORD,  NKeyPanelLayout, "SortMode",&Opt.ShowSortMode,1, 0},

	{1, REG_DWORD,  NKeyLayout, "LeftHeightDecrement",&Opt.LeftHeightDecrement,0, 0},
	{1, REG_DWORD,  NKeyLayout, "RightHeightDecrement",&Opt.RightHeightDecrement,0, 0},
	{1, REG_DWORD,  NKeyLayout, "WidthDecrement",&Opt.WidthDecrement,0, 0},
	{1, REG_DWORD,  NKeyLayout, "FullscreenHelp",&Opt.FullScreenHelp,0, 0},

	{1, REG_SZ,     NKeyDescriptions, "ListNames",&Opt.Diz.strListNames, 0, L"Descript.ion,Files.bbs"},
	{1, REG_DWORD,  NKeyDescriptions, "UpdateMode",&Opt.Diz.UpdateMode,DIZ_UPDATE_IF_DISPLAYED, 0},
	{1, REG_DWORD,  NKeyDescriptions, "ROUpdate",&Opt.Diz.ROUpdate,0, 0},
	{1, REG_DWORD,  NKeyDescriptions, "SetHidden",&Opt.Diz.SetHidden,1, 0},
	{1, REG_DWORD,  NKeyDescriptions, "StartPos",&Opt.Diz.StartPos,0, 0},
	{1, REG_DWORD,  NKeyDescriptions, "AnsiByDefault",&Opt.Diz.AnsiByDefault,0, 0},
	{1, REG_DWORD,  NKeyDescriptions, "SaveInUTF",&Opt.Diz.SaveInUTF,0, 0},

	{0, REG_DWORD,  NKeyKeyMacros, "MacroReuseRules",&Opt.Macro.MacroReuseRules,0, 0},
	{0, REG_SZ,     NKeyKeyMacros, "DateFormat",&Opt.Macro.strDateFormat, 0, L"%a %b %d %H:%M:%S %Z %Y"},
	{0, REG_SZ,     NKeyKeyMacros, "CONVFMT",&Opt.Macro.strMacroCONVFMT, 0, L"%.6g"},
	{0, REG_DWORD,  NKeyKeyMacros, "CallPluginRules",&Opt.Macro.CallPluginRules,0, 0},

	{0, REG_DWORD,  NKeyPolicies, "ShowHiddenDrives",&Opt.Policies.ShowHiddenDrives,1, 0},
	{0, REG_DWORD,  NKeyPolicies, "DisabledOptions",&Opt.Policies.DisabledOptions,0, 0},


	{0, REG_DWORD,  NKeySystem, "ExcludeCmdHistory",&Opt.ExcludeCmdHistory,0, 0}, //AN

	{1, REG_DWORD,  NKeyCodePages, "CPMenuMode2",&Opt.CPMenuMode,1,0},

	{1, REG_SZ,     NKeySystem, "FolderInfo",&Opt.InfoPanel.strFolderInfoFiles, 0, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me"},

	{1, REG_DWORD,  NKeyVMenu, "LBtnClick",&Opt.VMenu.LBtnClick, VMENUCLICK_CANCEL, 0},
	{1, REG_DWORD,  NKeyVMenu, "RBtnClick",&Opt.VMenu.RBtnClick, VMENUCLICK_CANCEL, 0},
	{1, REG_DWORD,  NKeyVMenu, "MBtnClick",&Opt.VMenu.MBtnClick, VMENUCLICK_APPLY, 0},
};

static bool g_config_ready = false;

void ReadConfig()
{
	FARString strKeyNameFromReg;
	FARString strPersonalPluginsPath;
	size_t I;

	ConfigReader cfg_reader;

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	cfg_reader.SelectSection(NKeySystem);
	Opt.LoadPlug.strPersonalPluginsPath = cfg_reader.GetString("PersonalPluginsPath", L"");
	bool ExplicitWindowMode=Opt.WindowMode!=FALSE;
	//Opt.LCIDSort=LOCALE_USER_DEFAULT; // проинициализируем на всякий случай
	/* *************************************************** </ПРЕПРОЦЕССЫ> */

	for (I=0; I < ARRAYSIZE(CFG); ++I)
	{
		cfg_reader.SelectSection(CFG[I].KeyName);
		switch (CFG[I].ValType)
		{
			case REG_DWORD:
				if ((int *)CFG[I].ValPtr == &Opt.Confirm.Exit) {
					// when background mode available then exit dialog allows also switch to background
					// so saved settings must differ for that two modes
					CFG[I].ValName = WINPORT(ConsoleBackgroundMode)(FALSE) ? "ExitOrBknd" : "Exit";
				}
				*(unsigned int *)CFG[I].ValPtr = cfg_reader.GetUInt(CFG[I].ValName, (unsigned int)CFG[I].DefDWord);
				break;
			case REG_SZ:
				*(FARString *)CFG[I].ValPtr = cfg_reader.GetString(CFG[I].ValName, CFG[I].DefStr);
				break;
			case REG_BINARY:
				int Size = cfg_reader.GetBytes((BYTE*)CFG[I].ValPtr, CFG[I].DefDWord, CFG[I].ValName, (BYTE*)CFG[I].DefStr);
				if (Size > 0 && Size < (int)CFG[I].DefDWord)
					memset(((BYTE*)CFG[I].ValPtr)+Size,0,CFG[I].DefDWord-Size);

				break;
		}
	}

	/* <ПОСТПРОЦЕССЫ> *************************************************** */

	SanitizeHistoryCounts();

	if (Opt.ShowMenuBar)
		Opt.ShowMenuBar=1;

	if (Opt.PluginMaxReadData < 0x1000) // || Opt.PluginMaxReadData > 0x80000)
		Opt.PluginMaxReadData=0x20000;

	if(ExplicitWindowMode)
	{
		Opt.WindowMode=TRUE;
	}

	Opt.HelpTabSize=8; // пока жестко пропишем...
	//   Уточняем алгоритм "взятия" палитры.
	for (I=COL_PRIVATEPOSITION_FOR_DIF165ABOVE-COL_FIRSTPALETTECOLOR+1;
	        I < (COL_LASTPALETTECOLOR-COL_FIRSTPALETTECOLOR);
	        ++I)
	{
		if (!Palette[I])
		{
			if (!Palette[COL_PRIVATEPOSITION_FOR_DIF165ABOVE-COL_FIRSTPALETTECOLOR])
				Palette[I]=DefaultPalette[I];
			else if (Palette[COL_PRIVATEPOSITION_FOR_DIF165ABOVE-COL_FIRSTPALETTECOLOR] == 1)
				Palette[I]=BlackPalette[I];

			/*
			else
			  в других случаях нифига ничего не делаем, т.к.
			  есть другие палитры...
			*/
		}
	}

	Opt.ViOpt.ViewerIsWrap&=1;
	Opt.ViOpt.ViewerWrap&=1;

	// Исключаем случайное стирание разделителей ;-)
	if (Opt.strWordDiv.IsEmpty())
		Opt.strWordDiv = WordDiv0;

	// Исключаем случайное стирание разделителей
	if (Opt.XLat.strWordDivForXlat.IsEmpty())
		Opt.XLat.strWordDivForXlat = WordDivForXlat0;

	Opt.PanelRightClickRule%=3;
	Opt.PanelCtrlAltShiftRule%=3;
	Opt.ConsoleDetachKey=KeyNameToKey(strKeyNameConsoleDetachKey);

	if (Opt.EdOpt.TabSize<1 || Opt.EdOpt.TabSize>512)
		Opt.EdOpt.TabSize=8;

	if (Opt.ViOpt.TabSize<1 || Opt.ViOpt.TabSize>512)
		Opt.ViOpt.TabSize=8;

	cfg_reader.SelectSection(NKeyKeyMacros);

	strKeyNameFromReg = cfg_reader.GetString("KeyRecordCtrlDot", szCtrlDot);

	if ((Opt.Macro.KeyMacroCtrlDot=KeyNameToKey(strKeyNameFromReg)) == KEY_INVALID)
		Opt.Macro.KeyMacroCtrlDot=KEY_CTRLDOT;

	strKeyNameFromReg = cfg_reader.GetString("KeyRecordCtrlShiftDot", szCtrlShiftDot);

	if ((Opt.Macro.KeyMacroCtrlShiftDot=KeyNameToKey(strKeyNameFromReg)) == KEY_INVALID)
		Opt.Macro.KeyMacroCtrlShiftDot=KEY_CTRLSHIFTDOT;

	Opt.EdOpt.strWordDiv = Opt.strWordDiv;
	FileList::ReadPanelModes(cfg_reader);

	if (Opt.strExecuteBatchType.IsEmpty()) // предохраняемся
		Opt.strExecuteBatchType=constBatchExt;

	{
		Opt.XLat.CurrentLayout=0;
		memset(Opt.XLat.Layouts,0,sizeof(Opt.XLat.Layouts));
		cfg_reader.SelectSection(NKeyXLat);
		FARString strXLatLayouts = cfg_reader.GetString("Layouts", L"");

		if (!strXLatLayouts.IsEmpty())
		{
			wchar_t *endptr;
			const wchar_t *ValPtr;
			UserDefinedList DestList;
			DestList.SetParameters(L';',0,ULF_UNIQUE);
			DestList.Set(strXLatLayouts);
			I=0;

			for (size_t DLI = 0; nullptr!=(ValPtr=DestList.Get(DLI)); ++DLI)
			{
				DWORD res=(DWORD)wcstoul(ValPtr, &endptr, 16);
				Opt.XLat.Layouts[I]=(HKL)(LONG_PTR)(HIWORD(res)? res : MAKELONG(res,res));
				++I;

				if (I >= ARRAYSIZE(Opt.XLat.Layouts))
					break;
			}

			if (I <= 1) // если указано меньше двух - "откключаем" эту
				Opt.XLat.Layouts[0]=0;
		}
	}

	memset(Opt.FindOpt.OutColumnTypes,0,sizeof(Opt.FindOpt.OutColumnTypes));
	memset(Opt.FindOpt.OutColumnWidths,0,sizeof(Opt.FindOpt.OutColumnWidths));
	memset(Opt.FindOpt.OutColumnWidthType,0,sizeof(Opt.FindOpt.OutColumnWidthType));
	Opt.FindOpt.OutColumnCount=0;


	if (!Opt.FindOpt.strSearchOutFormat.IsEmpty())
	{
		if (Opt.FindOpt.strSearchOutFormatWidth.IsEmpty())
			Opt.FindOpt.strSearchOutFormatWidth=L"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0";
		TextToViewSettings(Opt.FindOpt.strSearchOutFormat.CPtr(),Opt.FindOpt.strSearchOutFormatWidth.CPtr(),
                                  Opt.FindOpt.OutColumnTypes,Opt.FindOpt.OutColumnWidths,Opt.FindOpt.OutColumnWidthType,
                                  Opt.FindOpt.OutColumnCount);
	}

	FileFilter::InitFilter(cfg_reader);

	g_config_ready = true;
	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}

void ApplyConfig()
{
	ApplySudoConfiguration();
	ApplyConsoleTweaks();
}

void AssertConfigLoaded()
{
	if (!g_config_ready)
	{
		fprintf(stderr, "%s: oops\n", __FUNCTION__);
		abort();
	}
}

void SaveConfig(int Ask)
{
	if (Opt.Policies.DisabledOptions&0x20000) // Bit 17 - Сохранить параметры
		return;

	if (Ask && Message(0,2,MSG(MSaveSetupTitle),MSG(MSaveSetupAsk1),MSG(MSaveSetupAsk2),MSG(MSaveSetup),MSG(MCancel)))
		return;

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	Panel *LeftPanel=CtrlObject->Cp()->LeftPanel;
	Panel *RightPanel=CtrlObject->Cp()->RightPanel;
	Opt.LeftPanel.Focus=LeftPanel->GetFocus();
	Opt.LeftPanel.Visible=LeftPanel->IsVisible();
	Opt.RightPanel.Focus=RightPanel->GetFocus();
	Opt.RightPanel.Visible=RightPanel->IsVisible();

	if (LeftPanel->GetMode()==NORMAL_PANEL)
	{
		Opt.LeftPanel.Type=LeftPanel->GetType();
		Opt.LeftPanel.ViewMode=LeftPanel->GetViewMode();
		Opt.LeftPanel.SortMode=LeftPanel->GetSortMode();
		Opt.LeftPanel.SortOrder=LeftPanel->GetSortOrder();
		Opt.LeftPanel.SortGroups=LeftPanel->GetSortGroups();
		Opt.LeftPanel.NumericSort=LeftPanel->GetNumericSort();
		Opt.LeftPanel.CaseSensitiveSort=LeftPanel->GetCaseSensitiveSort();
		Opt.LeftSelectedFirst=LeftPanel->GetSelectedFirstMode();
		Opt.LeftPanel.DirectoriesFirst=LeftPanel->GetDirectoriesFirst();
	}

	LeftPanel->GetCurDir(Opt.strLeftFolder);
	LeftPanel->GetCurBaseName(Opt.strLeftCurFile);

	if (RightPanel->GetMode()==NORMAL_PANEL)
	{
		Opt.RightPanel.Type=RightPanel->GetType();
		Opt.RightPanel.ViewMode=RightPanel->GetViewMode();
		Opt.RightPanel.SortMode=RightPanel->GetSortMode();
		Opt.RightPanel.SortOrder=RightPanel->GetSortOrder();
		Opt.RightPanel.SortGroups=RightPanel->GetSortGroups();
		Opt.RightPanel.NumericSort=RightPanel->GetNumericSort();
		Opt.RightPanel.CaseSensitiveSort=RightPanel->GetCaseSensitiveSort();
		Opt.RightSelectedFirst=RightPanel->GetSelectedFirstMode();
		Opt.RightPanel.DirectoriesFirst=RightPanel->GetDirectoriesFirst();
	}

	RightPanel->GetCurDir(Opt.strRightFolder);
	RightPanel->GetCurBaseName(Opt.strRightCurFile);
	CtrlObject->HiFiles->SaveHiData();

	ConfigWriter cfg_writer;

	/* *************************************************** </ПРЕПРОЦЕССЫ> */
	cfg_writer.SelectSection(NKeySystem);
	cfg_writer.SetString("PersonalPluginsPath", Opt.LoadPlug.strPersonalPluginsPath);
//	cfg_writer.SetString(NKeyLanguage, "Main", Opt.strLanguage);

	for (size_t I=0; I < ARRAYSIZE(CFG); ++I)
	{
		if (CFG[I].IsSave)
		{
			cfg_writer.SelectSection(CFG[I].KeyName);
			switch (CFG[I].ValType)
			{
				case REG_DWORD:
					cfg_writer.SetUInt(CFG[I].ValName, *(unsigned int *)CFG[I].ValPtr);
					break;
				case REG_SZ:
					cfg_writer.SetString(CFG[I].ValName, ((const FARString *)CFG[I].ValPtr)->CPtr());
					break;
				case REG_BINARY:
					cfg_writer.SetBytes(CFG[I].ValName, (const BYTE*)CFG[I].ValPtr, CFG[I].DefDWord);
					break;
			}
		}
	}

	/* <ПОСТПРОЦЕССЫ> *************************************************** */
	FileFilter::SaveFilters(cfg_writer);
	FileList::SavePanelModes(cfg_writer);

	if (Ask)
		CtrlObject->Macro.SaveMacros();

	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}

void LanguageSettings()
{
	VMenu *LangMenu, *HelpMenu;

	if (Select(FALSE, &LangMenu))
	{
		Lang.Close();

		if (!Lang.Init(g_strFarPath, true, MNewFileName))
		{
			Message(MSG_WARNING, 1, L"Error", L"Cannot load language data", L"Ok");
			exit(0);
		}

		Select(TRUE,&HelpMenu);
		delete HelpMenu;
		LangMenu->Hide();
		CtrlObject->Plugins.ReloadLanguage();
		setenv("FARLANG", Opt.strLanguage.GetMB().c_str(), 1);
		PrepareStrFTime();
		PrepareUnitStr();
		FrameManager->InitKeyBar();
		CtrlObject->Cp()->RedrawKeyBar();
		CtrlObject->Cp()->SetScreenPosition();
		ApplySudoConfiguration();
	}
	delete LangMenu; //???? BUGBUG
}
