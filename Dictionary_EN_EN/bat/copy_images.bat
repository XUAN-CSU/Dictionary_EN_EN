@echo off
set SRC=%~dp0..\images
set DST=%~dp0..\..\WLC_Dictionary_release\images

echo Copying from %SRC% to %DST%

if not exist "%DST%" mkdir "%DST%"
xcopy "%SRC%\*" "%DST%\" /E /Y /I
