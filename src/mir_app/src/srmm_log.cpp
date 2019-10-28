/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (C) 2012-19 Miranda NG team,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/////////////////////////////////////////////////////////////////////////////////////////
// SRMM log container

#include "stdafx.h"
#include "chat.h"

struct LoggerClass
{
	LoggerClass(const char *p1, const wchar_t *p2, pfnSrmmLogCreator p3) :
		szShortName(mir_strdup(p1)),
		wszScreenName(mir_wstrdup(p2)),
		pfnBuilder(p3)
	{}

	ptrA szShortName;
	ptrW wszScreenName;
	pfnSrmmLogCreator pfnBuilder;
};

static OBJLIST<LoggerClass> g_arLogClasses(1, PtrKeySortT);

static CMOption<BYTE> g_bEnableCustomLogs("SRMM", "EnableCustomLogs", 0);

/////////////////////////////////////////////////////////////////////////////////////////

MIR_APP_DLL(HANDLE) RegisterSrmmLog(const char *pszShortName, const wchar_t *pwszScreenName, pfnSrmmLogCreator fnBuilder)
{
	if (!pszShortName || !pwszScreenName || !fnBuilder)
		return nullptr;

	auto *p = new LoggerClass(pszShortName, pwszScreenName, fnBuilder);
	g_arLogClasses.insert(p);
	return p;
}

MIR_APP_DLL(void) UnregisterSrmmLog(HANDLE pLogger)
{
	g_arLogClasses.remove((LoggerClass *)pLogger);
}

/////////////////////////////////////////////////////////////////////////////////////////

CSrmmLogWindow* Srmm_GetLogWindow(CMsgDialog *pDlg)
{
	if (!pDlg->isChat() || g_bEnableCustomLogs) {
		ptrA szCurr(db_get_sa(pDlg->m_hContact, "SRMsg", "Logger"));
		if (!szCurr)
			szCurr = db_get_sa(0, "SRMM", "Logger", "built-in");

		for (auto &it : g_arLogClasses)
			if (!mir_strcmp(szCurr, it->szShortName))
				return it->pfnBuilder(*pDlg);
	}

	for (auto &it : g_arLogClasses)
		if (!mir_strcmp(it->szShortName, "built-in"))
			return it->pfnBuilder(*pDlg);

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////
// options dialog

class CSrmmLogOptionsDlg : public CDlgBase
{
	CCtrlListBox m_list;
	CCtrlCheck chkCustomLogs;

public:
	CSrmmLogOptionsDlg() :
		CDlgBase(g_plugin, IDD_OPT_SRMMLOG),
		m_list(this, IDC_LIST),
		chkCustomLogs(this, IDC_ENABLE_CUSTOM)
	{
		CreateLink(chkCustomLogs, g_bEnableCustomLogs);

		m_list.OnSelChange = Callback(this, &CSrmmLogOptionsDlg::onChange_List);
	}

	bool OnInitDialog() override
	{
		ptrA szCurr(db_get_sa(0, "SRMM", "Logger", "built-in"));

		for (auto &it : g_arLogClasses) {
			int idx = m_list.AddString(it->wszScreenName, LPARAM(it));
			if (!mir_strcmp(szCurr, it->szShortName))
				m_list.SetCurSel(idx);
		}

		return true;
	}

	bool OnApply() override
	{
		int idx = m_list.GetCurSel();
		if (idx == -1)
			return false;

		auto *pLogger = (LoggerClass *)m_list.GetItemData(idx);
		db_set_s(0, "SRMM", "Logger", pLogger->szShortName);
		return true;
	}

	void onChange_List(CCtrlListBox *)
	{
		NotifyChange();
	}
};

void SrmmLogOptionsInit(WPARAM wParam)
{
	OPTIONSDIALOGPAGE odp = {};
	odp.position = 910000000;
	odp.szGroup.a = LPGEN("Message sessions");
	odp.szTitle.a = LPGEN("Log viewer");
	odp.flags = ODPF_BOLDGROUPS;
	odp.pDialog = new CSrmmLogOptionsDlg();
	g_plugin.addOptions(wParam, &odp);
}
