/*

WhatsAppWeb plugin for Miranda NG
Copyright © 2019 George Hazan

*/

#pragma once

#pragma warning(disable:4996)
#pragma warning(disable:4290)

#include <time.h>
#include <windows.h>

#include <newpluginapi.h>
#include <m_avatars.h>
#include <m_chat.h>
#include <m_clist.h>
#include <m_database.h>
#include <m_history.h>
#include <m_imgsrvc.h>
#include <m_ignore.h>
#include <m_json.h>
#include <m_langpack.h>
#include <m_message.h>
#include <m_netlib.h>
#include <m_options.h>
#include <m_popup.h>
#include <m_protocols.h>
#include <m_protosvc.h>
#include <m_protoint.h>
#include <m_skin.h>
#include <m_string.h>
#include <statusmodes.h>
#include <m_userinfo.h>
#include <m_icolib.h>
#include <m_utils.h>
#include <m_xml.h>
#include <m_hotkeys.h>
#include <m_folders.h>
#include <m_json.h>
#include <m_gui.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "../../libs/libqrencode/src/qrencode.h"

#include "../../libs/libsignal/src/curve.h"
#include "../../libs/libsignal/src/hkdf.h"
#include "../../libs/libsignal/src/signal_protocol.h"

struct signal_buffer
{
	size_t len;
	uint8_t data[];
};

struct signal_type_base
{
	unsigned int ref_count = 0;
	void (*destroy)(signal_type_base *instance) = 0;
};

struct ec_public_key : public signal_type_base
{
	uint8_t data[32];
};

struct ec_private_key : public signal_type_base
{
	uint8_t data[32];
};

#include "db.h"
#include "proto.h"
#include "resource.h"

#pragma comment(lib, "libeay32.lib")