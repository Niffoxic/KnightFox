@echo off
setlocal enabledelayedexpansion

echo --------------------------------------------
echo   Copying Engine DLLs, LIBs, and Shaders
echo --------------------------------------------

set "SOLUTION_DIR=%~1"
set "PLATFORM=%~2"
set "CONFIG=%~3"

set "ENGINE_OUT=%SOLUTION_DIR%Engine\Bin\%PLATFORM%\%CONFIG%"
set "APP_BIN=%SOLUTION_DIR%Application\Bin\%PLATFORM%\%CONFIG%"
set "APP_LIB=%SOLUTION_DIR%Application"
set "SHADER_SRC=%SOLUTION_DIR%Application\shaders"
set "SHADER_DST=%APP_BIN%\shaders"

echo Engine Output  : %ENGINE_OUT%
echo App DLL Output : %APP_BIN%
echo App LIB Output : %APP_LIB%
echo Shader Source  : %SHADER_SRC%
echo Shader Dest    : %SHADER_DST%
echo.

if not exist "%APP_BIN%" (
    mkdir "%APP_BIN%"
)

if not exist "%APP_LIB%" (
    mkdir "%APP_LIB%"
)

:: --------------------------------------------------
:: COPY DLL FILES
:: --------------------------------------------------
echo Copying DLL files...
copy /Y "%ENGINE_OUT%\*.dll" "%APP_BIN%" >nul 2>&1

if %ERRORLEVEL%==0 (
    echo DLLs copied successfully.
) else (
    echo No DLLs found or failed to copy.
)

:: --------------------------------------------------
:: COPY LIB FILES
:: --------------------------------------------------
echo Copying LIB files...
copy /Y "%ENGINE_OUT%\*.lib" "%APP_LIB%" >nul 2>&1

if %ERRORLEVEL%==0 (
    echo LIBs copied successfully.
) else (
    echo No LIBs found or failed to copy.
)

:: --------------------------------------------------
:: COPY SHADERS RECURSIVELY
:: --------------------------------------------------
echo Copying Shaders folder recursively...

if exist "%SHADER_DST%" (
    rmdir /S /Q "%SHADER_DST%"
)

xcopy "%SHADER_SRC%" "%SHADER_DST%" /E /I /Q /Y >nul 2>&1

if %ERRORLEVEL% lss 4 (
    echo Shaders copied successfully.
) else (
    echo Failed to copy shaders.
)

echo   Done!

endlocal
exit /b 0
