/*

'AutoShutdown'-Plugin for Miranda IM

Copyright 2004-2007 H. Herkenrath

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (Shutdown-License.txt); if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma once

/* Error Output */
void ShowInfoMessage(uint8_t flags,const char *pszTitle,const char *pszTextFmt,...);
char* GetWinErrorDescription(uint32_t dwLastError);

/* Time */
BOOL SystemTimeToTimeStamp(SYSTEMTIME *st,time_t *timestamp);
BOOL TimeStampToSystemTime(time_t timestamp,SYSTEMTIME *st);
BOOL GetFormatedCountdown(wchar_t *pszOut,int nSize,time_t countdown);
BOOL GetFormatedDateTime(wchar_t *pszOut,int nSize,time_t timestamp,BOOL fShowDateEvenToday);

/* Skin */
void AddHotkey();
