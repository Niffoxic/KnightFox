@echo off
setlocal enabledelayedexpansion

echo --------------------------------------------
echo   Copying Engine DLLs and LIBs
echo --------------------------------------------

set "SOLUTION_DIR=%~1"
set "PLATFORM=%~2"
set "CONFIG=%~3"

set "ENGINE_OUT=%SOLUTION_DIR%Engine\Bin\%PLATFORM%\%CONFIG%"
set "APP_BIN=%SOLUTION_DIR%Application\Bin\%PLATFORM%\%CONFIG%"
set "APP_LIB=%SOLUTION_DIR%Application"

echo Engine Output  : %ENGINE_OUT%
echo App DLL Output : %APP_BIN%
echo App LIB Output : %APP_LIB%
echo.

if not exist "%APP_BIN%" (
    mkdir "%APP_BIN%"
)

if not exist "%APP_LIB%" (
    mkdir "%APP_LIB%"
)

echo Copying DLL files...
copy /Y "%ENGINE_OUT%\*.dll" "%APP_BIN%" >nul 2>&1

if %ERRORLEVEL%==0 (
    echo DLLs copied successfully.
) else (
    echo No DLLs found or failed to copy.
)

echo Copying LIB files...
copy /Y "%ENGINE_OUT%\*.lib" "%APP_LIB%" >nul 2>&1

if %ERRORLEVEL%==0 (
    echo LIBs copied successfully.
) else (
    echo No LIBs found or failed to copy.
)

echo --------------------------------------------
echo   Done!
echo --------------------------------------------

endlocal
exit /b 0
