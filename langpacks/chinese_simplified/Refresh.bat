@echo off

REM ====== VARIABLES ========

set "language=chinese_simplified"
set "oldLangpackPath="
set "mirandaPath=D:\!utils\Miranda-IM-NG\"

REM =========================

cd "..\..\tools\lpgen\"
call refresher.bat %language% "%oldLangpackPath%"

REM if previous command failed, exit
if %ERRORLEVEL% NEQ 0 exit /B 1

REM copy and reload langpack in your Miranda (uncomment next line + enable cmdline.dll plugin to use it)
call installer.bat %language% "%mirandaPath%"
