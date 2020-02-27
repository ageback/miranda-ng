/*
Copyright (C) 2006 Ricardo Pescuma Domenecci

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
*/

#include "stdafx.h"

static IconItem mainIcons[] = {
	{ LPGEN("Main"),     "main",    IDI_MAIN    },
	{ LPGEN("Dialpad"),  "dialpad", IDI_DIALPAD },
	{ LPGEN("Secure"),   "secure",  IDI_SECURE  },
};

static IconItem stateIcons[] = {
	{ LPGEN("Talking"),  "talking", IDI_TALKING },
	{ LPGEN("Ringing"),  "ringing", IDI_RINGING },
	{ LPGEN("Calling"),  "calling", IDI_CALLING },
	{ LPGEN("On Hold"),  "onhold",  IDI_ON_HOLD },
	{ LPGEN("Ended"),    "ended",   IDI_ENDED   },
	{ LPGEN("Busy"),     "busy", 	  IDI_BUSY    },
};

static IconItem actionIcons[] = {
	{ LPGEN("Make Voice Call"),   "call",   IDI_ACTION_CALL   },
	{ LPGEN("Answer Voice Call"), "answer", IDI_ACTION_ANSWER },
	{ LPGEN("Hold Voice Call"),   "hold",   IDI_ACTION_HOLD   },
	{ LPGEN("Drop Voice Call"),   "drop",   IDI_ACTION_DROP   },
};

SoundDescr g_sounds[NUM_STATES] = {
	{ "voice_started", LPGENW("Started talking") },
	{ "voice_ringing", LPGENW("Ringing") },
	{ "voice_calling", LPGENW("Calling a contact") },
	{ "voice_holded",  LPGENW("Put a call on Hold") },
	{ "voice_ended",   LPGENW("End of call") },
	{ "voice_busy",    LPGENW("Busy signal") },
};

/////////////////////////////////////////////////////////////////////////////////////////

static vector<HGENMENU> hCMCalls;
static HGENMENU hCMCall = NULL;
static HGENMENU hCMAnswer = NULL;
static HGENMENU hCMDrop = NULL;
static HGENMENU hCMHold = NULL;

OBJLIST<VoiceProvider> modules(1, PtrKeySortT);
OBJLIST<VoiceCall> calls(1, PtrKeySortT);

HFONT fonts[NUM_STATES] = { 0 };
COLORREF font_colors[NUM_STATES] = { 0 };
int font_max_height;

COLORREF bkg_color = { 0 };
HBRUSH bk_brush = NULL;

static INT_PTR CListDblClick(WPARAM wParam, LPARAM lParam);

static INT_PTR Service_CanCall(WPARAM wParam, LPARAM lParam);
static INT_PTR Service_Call(WPARAM wParam, LPARAM lParam);
static INT_PTR CMAnswer(WPARAM wParam, LPARAM lParam);
static INT_PTR CMHold(WPARAM wParam, LPARAM lParam);
static INT_PTR CMDrop(WPARAM wParam, LPARAM lParam);

static VOID CALLBACK ClearOldVoiceCalls(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

class CallingMethod
{
public:
	VoiceProvider *provider;
	MCONTACT hContact;
	wchar_t number[128];

	CallingMethod(VoiceProvider *provider, MCONTACT hContact, const wchar_t *number = NULL)
		: provider(provider), hContact(hContact)
	{
		if (number == NULL)
			this->number[0] = 0;
		else
			lstrcpyn(this->number, number, _countof(this->number));
	}

	void Call()
	{
		provider->Call(hContact, number);
	}
};

static int sttCompareCallingMethods(const CallingMethod *p1, const CallingMethod *p2)
{
	if (p1->hContact != p2->hContact)
		return (int)p2->hContact - (int)p1->hContact;

	BOOL noNum1 = (IsEmptyW(p1->number) ? 1 : 0);
	BOOL noNum2 = (IsEmptyW(p2->number) ? 1 : 0);
	if (noNum1 != noNum2)
		return noNum2 - noNum1;

	if (!noNum1) {
		int numDif = lstrcmp(p1->number, p2->number);
		if (numDif != 0)
			return numDif;
	}

	BOOL isProto1 = Proto_IsProtoOnContact(p1->hContact, p1->provider->name);
	BOOL isProto2 = Proto_IsProtoOnContact(p2->hContact, p2->provider->name);
	if (isProto1 != isProto2)
		return isProto2 - isProto1;

	return lstrcmp(p1->provider->description, p2->provider->description);
}

static void AddMethodsFrom(OBJLIST<CallingMethod> *list, MCONTACT hContact)
{
	for (int i = 0; i < modules.getCount(); i++) {
		VoiceProvider *provider = &modules[i];
		if (provider->CanCall(hContact))
			list->insert(new CallingMethod(provider, hContact));
	}
}

static void AddMethodsFrom(OBJLIST<CallingMethod> *list, MCONTACT hContact, const wchar_t *number)
{
	for (int i = 0; i < modules.getCount(); i++) {
		VoiceProvider *provider = &modules[i];
		if (provider->CanCall(number))
			list->insert(new CallingMethod(provider, hContact, number));
	}
}

static void BuildCallingMethodsList(OBJLIST<CallingMethod> *list, MCONTACT hContact)
{
	AddMethodsFrom(list, hContact);

	// Fetch contact number
	char *proto = Proto_GetBaseAccountName(hContact);

	CMStringW protoNumber(db_get_wsm(hContact, proto, "Number"));
	if (!protoNumber.IsEmpty())
		AddMethodsFrom(list, hContact, protoNumber);

	for (int i = 0; ; i++) {
		char tmp[128];
		mir_snprintf(tmp, _countof(tmp), "MyPhone%d", i);

		CMStringW number(db_get_wsm(hContact, "UserInfo", tmp));
		if (!number.IsEmpty())
			AddMethodsFrom(list, hContact, number);

		if (number.IsEmpty() && i >= 4)
			break;
	}
}

// Functions ////////////////////////////////////////////////////////////////////////////

static MCONTACT ConvertMetacontact(MCONTACT hContact)
{
	MCONTACT hTmp = db_mc_getMostOnline(hContact);
	if (hTmp != NULL)
		return hTmp;

	return hContact;
}

static void AddAccount(PROTOACCOUNT *acc)
{
	if (!acc->IsEnabled())
		return;
	if (IsEmptyA(acc->szModuleName))
		return;
	if (!ProtoServiceExists(acc->szModuleName, PS_VOICE_CAPS))
		return;

	int flags = CallProtoService(acc->szModuleName, PS_VOICE_CAPS, 0, 0);

	if ((flags & VOICE_CAPS_VOICE) == 0)
		return;

	VOICE_MODULE vm = { 0 };
	vm.cbSize = sizeof(vm);
	vm.name = acc->szModuleName;
	vm.description = acc->tszAccountName;
	vm.flags = flags;
	VoiceRegister((WPARAM)&vm, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////

static int AccListChanged(WPARAM wParam, LPARAM lParam)
{
	PROTOACCOUNT *acc = (PROTOACCOUNT *)lParam;
	if (acc == NULL)
		return 0;

	VoiceProvider *provider = FindModule(acc->szModuleName);

	switch (wParam) {
	case PRAC_ADDED:
		AddAccount(acc);
		break;

	case PRAC_CHANGED:
		if (provider != NULL)
			lstrcpyn(provider->description, acc->tszAccountName, _countof(provider->description));
		break;

	case PRAC_CHECKED:
		{
			BOOL enabled = acc->IsEnabled();

			if (!enabled) {
				if (provider != NULL)
					VoiceUnregister((WPARAM)acc->szModuleName, 0);
			}
			else {
				if (provider == NULL)
					AddAccount(acc);
			}
			break;
		}
	case PRAC_REMOVED:
		if (provider != NULL)
			VoiceUnregister((WPARAM)acc->szModuleName, 0);
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

static INT_PTR Service_CallItem(WPARAM wParam, LPARAM, LPARAM param)
{
	MCONTACT hContact = (MCONTACT)wParam;
	int index = (int)param;

	if (hContact == NULL)
		return -1;

	hContact = ConvertMetacontact(hContact);

	OBJLIST<CallingMethod> methods(10, sttCompareCallingMethods);
	BuildCallingMethodsList(&methods, hContact);

	if (index < 0 || index >= methods.getCount())
		return -2;

	methods[index].Call();
	return 0;
}

static int PreBuildContactMenu(WPARAM wParam, LPARAM)
{
	Menu_ShowItem(hCMCall, false);
	Menu_ShowItem(hCMAnswer, false);
	Menu_ShowItem(hCMHold, false);
	Menu_ShowItem(hCMDrop, false);
	for (unsigned int i = 0; i < hCMCalls.size(); ++i)
		Menu_ShowItem(hCMCalls[i], false);

	MCONTACT hContact = (MCONTACT)wParam;
	if (hContact == NULL)
		return -1;

	hContact = ConvertMetacontact(hContact);

	// There is a current call already?
	VoiceCall *call = FindVoiceCall(hContact);
	if (call == nullptr) {
		OBJLIST<CallingMethod> methods(10, sttCompareCallingMethods);
		BuildCallingMethodsList(&methods, hContact);

		if (methods.getCount() == 1) {
			CallingMethod *method = &methods[0];

			wchar_t name[128];
			if (!IsEmptyW(method->number))
				mir_snwprintf(name, _countof(name), TranslateT("Call %s with %s"),
				method->number, method->provider->description);
			else
				mir_snwprintf(name, _countof(name), TranslateT("Call with %s"),
				method->provider->description);

			Menu_ModifyItem(hCMCall, name);
			Menu_ShowItem(hCMCall, true);
		}
		else if (methods.getCount() > 1) {
			Menu_ModifyItem(hCMCall, TranslateT("Call"));
			Menu_ShowItem(hCMCall, true);

			for (int i = 0; i < methods.getCount(); ++i) {
				CallingMethod *method = &methods[i];

				HICON hIcon = method->provider->GetIcon();

				wchar_t name[128];
				if (!IsEmptyW(method->number))
					mir_snwprintf(name, _countof(name), TranslateT("%s with %s"),
					method->number, method->provider->description);
				else
					mir_snwprintf(name, _countof(name), TranslateT("with %s"),
					method->provider->description);

				char service[128];
				mir_snprintf(service, _countof(service), "VoiceService/ContactMenu/Call_%d", i);

				if (i == hCMCalls.size()) {
					CreateServiceFunctionParam(service, Service_CallItem, i);

					CMenuItem mi(&g_plugin);
					mi.position = i;
					mi.flags = CMIF_UNICODE;
					mi.name.w = name;
					mi.hIcon = hIcon;
					mi.pszService = service;
					mi.root = hCMCall;
					hCMCalls.push_back(Menu_AddContactMenuItem(&mi));
				}
				else Menu_ModifyItem(hCMCalls[i], name, hIcon);

				method->provider->ReleaseIcon(hIcon);
			}
		}
	}
	else {
		switch (call->state) {
		case VOICE_STATE_CALLING:
			Menu_ShowItem(hCMDrop, true);
			break;

		case VOICE_STATE_TALKING:
			if (call->module->CanHold())
				Menu_ShowItem(hCMHold, true);
			Menu_ShowItem(hCMDrop, true);
			break;

		case VOICE_STATE_RINGING:
		case VOICE_STATE_ON_HOLD:
			Menu_ShowItem(hCMAnswer, true);
			Menu_ShowItem(hCMDrop, true);
			break;
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

static int IconsChanged(WPARAM, LPARAM)
{
	if (hwnd_frame != NULL)
		PostMessage(hwnd_frame, WMU_REFRESH, 0, 0);

	return 0;
}

static int ReloadColor(WPARAM, LPARAM)
{
	ColourIDW ci = { 0 };
	lstrcpyn(ci.group, TranslateT("Voice Calls"), _countof(ci.group));
	lstrcpyn(ci.name, TranslateT("Background"), _countof(ci.name));

	bkg_color = Colour_GetW(ci);

	if (bk_brush != NULL)
		DeleteObject(bk_brush);
	bk_brush = CreateSolidBrush(bkg_color);

	if (hwnd_frame != NULL)
		InvalidateRect(hwnd_frame, NULL, TRUE);

	return 0;
}

VoiceProvider *FindModule(const char *szModule)
{
	for (int i = 0; i < modules.getCount(); i++)
		if (strcmp(modules[i].name, szModule) == 0)
			return &modules[i];

	return NULL;
}

static bool IsCall(VoiceCall *call, const char *szModule, const char *id)
{
	return strcmp(call->module->name, szModule) == 0
		&& call->id != NULL && strcmp(call->id, id) == 0;
}

VoiceCall *FindVoiceCall(const char *szModule, const char *id, bool add)
{
	for (int i = 0; i < calls.getCount(); i++) {
		if (IsCall(&calls[i], szModule, id)) {
			return &calls[i];
		}
	}

	if (add) {
		VoiceProvider *module = FindModule(szModule);
		if (module == NULL)
			return NULL;

		VoiceCall *tmp = new VoiceCall(module, id);
		calls.insert(tmp);
		return tmp;
	}

	return NULL;
}

VoiceCall* FindVoiceCall(MCONTACT hContact)
{
	for (int i = 0; i < calls.getCount(); i++) {
		if (calls[i].state != VOICE_STATE_ENDED && calls[i].hContact == hContact) {
			return &calls[i];
		}
	}

	return NULL;
}

static VOID CALLBACK ClearOldVoiceCalls(HWND, UINT, UINT_PTR, DWORD)
{
	DWORD now = GetTickCount();
	BOOL refresh = FALSE;
	for (int i = calls.getCount() - 1; i >= 0; --i) {
		VoiceCall *call = &calls[i];

		if (call->state == VOICE_STATE_ENDED && call->end_time + TIME_TO_SHOW_ENDED_CALL < now) {
			calls.remove(i);
			refresh = TRUE;
		}
	}

	if (refresh && hwnd_frame != NULL)
		PostMessage(hwnd_frame, WMU_REFRESH, 0, 0);
}

bool CanCall(MCONTACT hContact, BOOL now)
{
	for (int i = 0; i < modules.getCount(); i++) {
		if (modules[i].CanCall(hContact, now))
			return true;
	}

	return false;
}

bool CanCall(const wchar_t *number)
{
	for (int i = 0; i < modules.getCount(); i++) {
		if (modules[i].CanCall(number))
			return true;
	}

	return false;
}

bool CanCallNumber()
{
	for (int i = 0; i < modules.getCount(); i++) {
		if (modules[i].flags & VOICE_CAPS_CALL_STRING)
			return true;
	}

	return false;
}

bool IsFinalState(int state)
{
	return state == VOICE_STATE_ENDED || state == VOICE_STATE_BUSY;
}

VoiceCall *GetTalkingCall()
{
	for (int i = 0; i < calls.getCount(); ++i) {
		VoiceCall *call = &calls[i];

		if (call->state == VOICE_STATE_TALKING)
			return call;
	}

	return NULL;
}

void HoldOtherCalls(VoiceCall *call)
{
	for (int i = 0; i < calls.getCount(); ++i) {
		VoiceCall *other = &calls[i];

		if (other == call || other->state != VOICE_STATE_TALKING)
			continue;

		if (other->CanHold())
			other->Hold();
		else
			other->Drop();
	}
}

void Answer(VoiceCall *call)
{
	if (!call->CanAnswer())
		return;

	HoldOtherCalls(call);

	// Now annswer it
	call->Answer();
}

/////////////////////////////////////////////////////////////////////////////////////////

static int ReloadFont(WPARAM, LPARAM)
{
	FontID fi = { 0 };
	strncpy_s(fi.group, "Voice Calls", _TRUNCATE);

	font_max_height = 0;
	for (int i = 0; i < NUM_STATES; i++) {
		if (fonts[i] != 0) DeleteObject(fonts[i]);

		strncpy_s(fi.name, stateIcons[i].szName, _TRUNCATE);

		LOGFONTA log_font = { 0 };
		font_colors[i] = Font_Get(fi, &log_font);
		fonts[i] = CreateFontIndirectA(&log_font);

		font_max_height = max(font_max_height, log_font.lfHeight);
	}

	if (hwnd_frame != NULL)
		PostMessage(hwnd_frame, WMU_REFRESH, 0, 0);

	return 0;
}

static INT_PTR Service_CanCall(WPARAM wParam, LPARAM)
{
	MCONTACT hContact = (MCONTACT)wParam;
	if (hContact == NULL)
		return -1;

	hContact = ConvertMetacontact(hContact);
	return CanCall(hContact) ? 1 : 0;
}

static INT_PTR Service_Call(WPARAM wParam, LPARAM)
{
	MCONTACT hContact = (MCONTACT)wParam;
	if (hContact == NULL)
		return -1;

	hContact = ConvertMetacontact(hContact);

	OBJLIST<CallingMethod> methods(10, sttCompareCallingMethods);
	BuildCallingMethodsList(&methods, hContact);

	if (methods.getCount() < 1)
		return -2;

	CallingMethod *method = &methods[0];
	if (!IsEmptyW(method->number))
		return -2;

	method->Call();
	return 0;
}

static INT_PTR CMAnswer(WPARAM wParam, LPARAM)
{
	MCONTACT hContact = (MCONTACT)wParam;
	if (hContact == NULL)
		return -1;

	hContact = ConvertMetacontact(hContact);

	VoiceCall *call = FindVoiceCall(hContact);
	if (call != NULL)
		Answer(call);

	return 0;
}

static INT_PTR CMHold(WPARAM wParam, LPARAM)
{
	MCONTACT hContact = (MCONTACT)wParam;
	if (hContact == NULL)
		return -1;

	hContact = ConvertMetacontact(hContact);

	VoiceCall *call = FindVoiceCall(hContact);
	if (call != NULL)
		call->Hold();

	return 0;
}

static INT_PTR CMDrop(WPARAM wParam, LPARAM)
{
	MCONTACT hContact = (MCONTACT)wParam;
	if (hContact == NULL)
		return -1;

	hContact = ConvertMetacontact(hContact);

	VoiceCall *call = FindVoiceCall(hContact);
	if (call != NULL)
		call->Drop();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Called when all the modules are loaded

int ModulesLoaded(WPARAM, LPARAM)
{
	// add our modules to the KnownModules list
	CallService("DBEditorpp/RegisterSingleModule", (WPARAM)MODULE_NAME, 0);

	// Init icons
	g_plugin.registerIcon(LPGEN("Voice Calls"), mainIcons, "vc");
	g_plugin.registerIcon(LPGEN("Voice Calls"), stateIcons, "vc");
	g_plugin.registerIcon(LPGEN("Voice Calls"), actionIcons, "vca");

	HookEvent(ME_SKIN_ICONSCHANGED, IconsChanged);

	// Init fonts
	{
		FontID fi = {};
		strncpy_s(fi.group, LPGEN("Voice Calls"), _TRUNCATE);
		strncpy_s(fi.dbSettingsGroup, MODULE_NAME, _TRUNCATE);

		for (int i = 0; i < _countof(stateIcons); i++) {
			fi.order = i;
			strncpy_s(fi.name, stateIcons[i].szName, _TRUNCATE);
			g_plugin.addFont(&fi);
		}

		ReloadFont(0, 0);
		HookEvent(ME_FONT_RELOAD, ReloadFont);
	}

	// Init bkg color
	{
		ColourID ci = { 0 };
		strncpy_s(ci.group, LPGEN("Voice Calls"), _TRUNCATE);
		strncpy_s(ci.name, LPGEN("Background"), _TRUNCATE);
		strncpy_s(ci.dbSettingsGroup, MODULE_NAME, _TRUNCATE);
		strncpy_s(ci.setting, "BkgColor", _TRUNCATE);
		ci.defcolour = GetSysColor(COLOR_BTNFACE);
		g_plugin.addColor(&ci);

		ReloadColor(0, 0);
		HookEvent(ME_COLOUR_RELOAD, ReloadColor);
	}

	InitOptions();
	InitFrames();

	// Add menu items
	CMenuItem mi(&g_plugin);
	mi.position = -2000020000;

	mi.name.a = actionIcons[0].szDescr;
	mi.hIcolibItem = g_plugin.getIconHandle(IDI_ACTION_CALL);
	mi.pszService = MS_VOICESERVICE_CM_CALL;
	hCMCall = Menu_AddContactMenuItem(&mi);
	CreateServiceFunction(mi.pszService, Service_Call);

	mi.position++;
	mi.name.a = actionIcons[1].szDescr;
	mi.hIcolibItem = g_plugin.getIconHandle(IDI_ACTION_ANSWER);
	mi.pszService = MS_VOICESERVICE_CM_ANSWER;
	hCMAnswer = Menu_AddContactMenuItem(&mi);
	CreateServiceFunction(mi.pszService, CMAnswer);

	mi.position++;
	mi.position++;
	mi.name.a = actionIcons[2].szDescr;
	mi.hIcolibItem = g_plugin.getIconHandle(IDI_ACTION_HOLD);
	mi.pszService = MS_VOICESERVICE_CM_HOLD;
	hCMHold = Menu_AddContactMenuItem(&mi);
	CreateServiceFunction(mi.pszService, CMHold);

	mi.position++;
	mi.name.a = actionIcons[3].szDescr;
	mi.hIcolibItem = g_plugin.getIconHandle(IDI_ACTION_DROP);
	mi.pszService = MS_VOICESERVICE_CM_DROP;
	hCMDrop = Menu_AddContactMenuItem(&mi);
	CreateServiceFunction(mi.pszService, CMDrop);

	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, PreBuildContactMenu);

	// Util services
	CreateServiceFunction(MS_VOICESERVICE_CALL, Service_Call);
	CreateServiceFunction(MS_VOICESERVICE_CAN_CALL, Service_CanCall);

	// Sounds
	for (auto &it : g_sounds)
		g_plugin.addSound(it.szName, LPGENW("Voice Calls"), it.wszDescr);
	g_plugin.addSound("voice_dialpad", LPGENW("Voice Calls"), LPGENW("Dialpad press"));

	SetTimer(NULL, 0, 1000, ClearOldVoiceCalls);

	// Accounts
	for (auto *pa : Accounts())
		AddAccount(pa);

	HookEvent(ME_PROTO_ACCLISTCHANGED, AccListChanged);
	return 0;
}

int PreShutdown(WPARAM, LPARAM)
{
	DeInitFrames();
	DeInitOptions();
	return 0;
}

int ProtoAck(WPARAM, LPARAM lParam)
{
	ACKDATA *ack = (ACKDATA *)lParam;
	if (ack->type == ACKTYPE_STATUS)
		if (hwnd_frame != NULL)
			PostMessage(hwnd_frame, WMU_REFRESH, 0, 0);

	return 0;
}
