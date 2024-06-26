/*
Popup Plus plugin for Miranda IM

Copyright	© 2002 Luca Santarelli,
© 2004-2007 Victor Pavlychko
© 2010 MPK
© 2010 Merlin_de

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

/*
===============================================================================
Popup plugin

This plugin notifies you when a contact changes his/hers status with a Popup.
You can customize its look (style of popup, font, text...) and behaviour (the
position in the screen, the time to live, the action on a mouse click).

File name: "headers.h"
This file has the purpose of becoming a precompiled header common to every
project file.

Written by: Hrk (Luca Santarelli)
Updated by: Zazoo (Victor Pavlychko)

Miranda NG can be found here:
https://miranda-ng.org/

Miranda NG plugins and tools can be found here:
https://miranda-ng.org/distr/
*/

#ifndef HEADERS_H
#define HEADERS_H

// disable security warnings about "*_s" functions
#define _CRT_SECURE_NO_DEPRECATE

#define OEMRESOURCE

#include <windows.h>
#include <windowsx.h>
#include <time.h>
#include <process.h>
#include <commctrl.h>
#include <shellapi.h>
#include <malloc.h>
#include <Uxtheme.h>

#include <newpluginapi.h>
#include <m_clistint.h>
#include <m_options.h>
#include <m_skin.h>
#include <m_langpack.h>
#include <m_database.h>
#undef db_get_s
#include <m_protosvc.h>
#include <m_button.h>
#include <m_message.h>
#include <m_userinfo.h>
#include <m_contacts.h>
#include <m_clc.h>
#include <m_icolib.h>
#include <m_idle.h>
#include <m_genmenu.h>
#include <m_hotkeys.h>
#include <m_fontservice.h>
#include <m_avatars.h>
#include <m_extraicons.h>
#include <m_popup_int.h>
#include <m_imgsrvc.h>
#include <m_timezones.h>

#include <m_metacontacts.h>
#include <m_folders.h>
#include <m_ieview.h>
#include <m_smileyadd.h>
#include <m_text.h>
#include <m_toptoolbar.h>
#include <m_hpp.h>

#define PU_MODULCLASS   "PopupCLASS" // temp DB modul for this plugin
#define MODULENAME      "Popup"
#define MODULNAME_LONG L"Popup Plus"
#define MODULNAME_PLU   "Popups"
#define MODULNAME_PLUW L"Popups"

struct CMPlugin : public PLUGIN<CMPlugin>
{
	CMPlugin();

	int Load() override;
	int Unload() override;
};

#include "resource.h"
#include "version.h"
#include "config.h"
#include "common.h"
#include "def_settings.h"
#include "opttree.h"
#include "opt_gen.h"
#include "opt_skins.h"
#include "opt_contacts.h"
#include "opt_adv.h"
#include "history.h"
#include "srmm_menu.h"
#include "bitmap_funcs.h"
#include "font.h"
#include "formula.h"
#include "skin.h"
#include "popup_thread.h"
#include "actions.h"
#include "notifications.h"
#include "opt_class.h"
#include "popup_wnd2.h"
#include "effects.h"
#include "avatars.h"
#include "avatars_simple.h"
#include "avatars_flash.h"
#include "avatars_gif.h"
#include "popup_gdiplus.h"

INT_PTR svcEnableDisableMenuCommand(WPARAM, LPARAM);

void InitIcons();
void CreateServices();

int OnIdleChanged(WPARAM, LPARAM lParam);

#endif //HEADERS_H
