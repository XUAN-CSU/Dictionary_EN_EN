@echo off
setlocal

:: Source .bat file (full path)
set "SRC=%~dp0Auto_Add_Version_and_Delete_self.bat"

:: Destination folder (absolute)
set "DST=%~dp0..\..\Dictionary_EN_EN_release"

echo Copying file:
echo   From: %SRC%
echo   To:   %DST%

:: Create destination folder if not exist
if not exist "%DST%" mkdir "%DST%"

:: Copy the .bat file
copy "%SRC%" "%DST%\" /Y

echo Done.
exit /b

