@echo off
setlocal enabledelayedexpansion

:: ---------------------------------------------------------
:: Always work inside the bat folder (safe for Qt build)
:: ---------------------------------------------------------
pushd "%~dp0"

:: ---------------------------------------------------------
:: Clean unwanted files/folders
:: ---------------------------------------------------------
echo Deleting build cache files...

if exist ".qmake.stash" (
    del ".qmake.stash" /f /q
    echo Deleted .qmake.stash
)

if exist "Makefile" (
    del "Makefile" /f /q
    echo Deleted Makefile
)

:: if exist "english_word_history.txt" (
::    del "english_word_history.txt" /f /q
::    echo Deleted english_word_history.txt
:: )

:: if exist "word_audio" (
::    rmdir /s /q "word_audio"
::    echo Deleted folder: word_audio
:: )

echo Cleanup done.

:: ---------------------------------------------------------
:: Work in the parent folder
:: ---------------------------------------------------------
cd ..
set "PARENT_DIR=%CD%"
cd "%~dp0"

:: ---------------------------------------------------------
:: Find max version Dictionary_EN_EN_release_Vx.x.x
:: ---------------------------------------------------------
set max_major=0
set max_minor=0
set max_patch=0

for /d %%D in ("%PARENT_DIR%\Dictionary_EN_EN_release_V*.*.*") do (
    for /f "tokens=2-4 delims=V." %%a in ("%%~nxD") do (
        set major=%%a
        set minor=%%b
        set patch=%%c

        if !major! gtr !max_major! (
            set max_major=!major!
            set max_minor=!minor!
            set max_patch=!patch!
        ) else if !major! equ !max_major! if !minor! gtr !max_minor! (
            set max_minor=!minor!
            set max_patch=!patch!
        ) else if !major! equ !max_major! if !minor! equ !max_minor! if !patch! gtr !max_patch! (
            set max_patch=!patch!
        )
    )
)

:: Next version patch
set /a next_patch=max_patch+1
set "newname=Dictionary_EN_EN_release_V!max_major!.!max_minor!.!next_patch!"

echo Next version: !newname!

:: ---------------------------------------------------------
:: COPY Dictionary_EN_EN_release → new version folder (excluding specific files)
:: ---------------------------------------------------------
echo Copying Dictionary_EN_EN_release → !newname! 
echo (Excluding: %~nx0, word_audio, english_word_history.txt)

:: Create destination folder first
mkdir "%PARENT_DIR%\!newname!" >nul 2>&1

:: Copy folders except "word_audio" and the new version folder
for /d %%F in (*) do (
    if /i not "%%F"=="!newname!" (
        if /i not "%%F"=="word_audio" (
            xcopy "%%F" "%PARENT_DIR%\!newname!\%%F" /E /I /H /K /Y >nul
            echo Copied folder: %%F
        )
    )
)

:: Copy files except batch file and english_word_history.txt
for %%F in (*.*) do (
    if /i not "%%F"=="%~nx0" (
        if /i not "%%F"=="english_word_history.txt" (
            copy "%%F" "%PARENT_DIR%\!newname!\%%F" >nul
            echo Copied file: %%F
        )
    )
)

echo DONE. Created: !newname!

:: Return to original directory
popd

:: Self-delete
echo Deleting this script file...
start "" /b cmd /c del "%~f0" & exit /b