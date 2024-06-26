/*
Miranda IM Country Flags Plugin
Copyright (C) 2006-1007 H. Herkenrath

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (Flags-License.txt); if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"

bool bUseUnknown, bShowStatusIcon, bShowExtraIcon, bUseIpToCountry;

#define M_ENABLE_SUBCTLS  (WM_APP+1)

static INT_PTR CALLBACK ExtraImgOptDlgProc(HWND hwndDlg, UINT msg, WPARAM, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		CheckDlgButton(hwndDlg, IDC_CHECK_SHOWSTATUSICONFLAG, bShowStatusIcon ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHECK_SHOWEXTRAIMGFLAG, bShowExtraIcon ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHECK_USEUNKNOWNFLAG, bUseUnknown ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHECK_USEIPTOCOUNTRY, bUseIpToCountry ? BST_CHECKED : BST_UNCHECKED);

		SendMessage(hwndDlg, M_ENABLE_SUBCTLS, 0, 0);
		return TRUE; /* default focus */

	case M_ENABLE_SUBCTLS:
	{
		BOOL checked = IsDlgButtonChecked(hwndDlg, IDC_CHECK_SHOWEXTRAIMGFLAG);
		EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_EXTRAIMGFLAGCOLUMN), checked);
		if (!checked)
			checked = IsDlgButtonChecked(hwndDlg, IDC_CHECK_SHOWSTATUSICONFLAG);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_USEUNKNOWNFLAG), checked);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_USEIPTOCOUNTRY), checked);
	}
	return TRUE;

	case WM_COMMAND:
		PostMessage(hwndDlg, M_ENABLE_SUBCTLS, 0, 0);
		PostMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0); /* enable apply */
		return FALSE;

	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->code) {
		case PSN_APPLY: /* setting change hook will pick these up  */
			bool bChanged = false, bTemp;

			if ((bTemp = IsDlgButtonChecked(hwndDlg, IDC_CHECK_USEUNKNOWNFLAG) != 0) != bUseUnknown)
				g_plugin.setByte("UseUnknownFlag", bUseUnknown = bTemp), bChanged = true;
			if ((bTemp = IsDlgButtonChecked(hwndDlg, IDC_CHECK_USEIPTOCOUNTRY) != 0) != bUseIpToCountry)
				g_plugin.setByte("UseIpToCountry", bUseIpToCountry = bTemp), bChanged = true;
			/* Status Icon */
			if ((bTemp = IsDlgButtonChecked(hwndDlg, IDC_CHECK_SHOWSTATUSICONFLAG) != 0) != bShowStatusIcon)
				g_plugin.setByte("ShowStatusIconFlag", bShowStatusIcon = bTemp), bChanged = true;
			/* Extra Image */
			if ((bTemp = IsDlgButtonChecked(hwndDlg, IDC_CHECK_SHOWEXTRAIMGFLAG) != 0) != bShowExtraIcon)
				g_plugin.setByte("ShowExtraImgFlag", bShowExtraIcon = bTemp), bChanged = true;

			if (bChanged) {
				UpdateExtraImages();
				UpdateStatusIcons(0);
			}
			return TRUE;
		}
		break;
	}
	return FALSE;
}

int OnOptionsInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = {};
	odp.flags = ODPF_BOLDGROUPS;
	odp.position = 900000002;
	odp.szGroup.a = LPGEN("Customize");
	odp.szTitle.a = LPGEN("Icons");
	odp.szTab.a = LPGEN("Country Flags");
	odp.pfnDlgProc = ExtraImgOptDlgProc;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_EXTRAIMG);
	g_plugin.addOptions(wParam, &odp);
	return 0;
}
