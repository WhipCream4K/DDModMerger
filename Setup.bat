@echo off
SETLOCAL

:: Set the URL and output file name
SET "URL=https://download.firedaemon.com/FireDaemon-OpenSSL/openssl-3.2.1.zip"
SET "FILENAME=openssl-3.2.1.zip"

:: Download the file
PowerShell -Command "& {Invoke-WebRequest -Uri '%URL%' -OutFile '%FILENAME%'}"
echo Download completed.

:: Extract the ZIP file
PowerShell -Command "& {Expand-Archive -Path '%FILENAME%' -DestinationPath '.' -Force}"
echo Extraction completed.

:: Delete the ZIP file
DEL "%FILENAME%"
echo ZIP file deleted.

ENDLOCAL
pause
