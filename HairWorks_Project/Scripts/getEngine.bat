@echo OFF

rem To avoid launching the editor, call `getEngine.bat 0` - default is to launch the editor
rem To set the shared local DDC, call `getEngine.bat 0 1` - default is not to set the shared DDC

set LAUNCH=%1
set NODDC=%2

echo Looking for //notnv/epicgames/scripts/gitpackage/... make sure you have that synced
set EPICGAMES_ROOT=%~dp0
:loop
if exist %EPICGAMES_ROOT%\scripts\gitpackage goto :havepath
set EPICGAMES_ROOT=%EPICGAMES_ROOT%..\
goto :loop

:havepath
echo Found it.
echo Asking packman for available packages...
rem this will initialize packman and python
set PACKMAN=%EPICGAMES_ROOT%\scripts\gitpackage\packman.cmd
call %PACKMAN% -h >NUL

rem echo NODDC = %NODDC%
if "_%NODDC%"=="_" goto GET_PACKAGE
if "_%NODDC%"=="_0" goto GET_PACKAGE

rem set the UE4 DDC cache 
set UE4-DDC-NAME=UE-SharedDataCachePath
set DDC_DIR=%PM_PACKAGES_ROOT%\UE4-DDC
echo setting %DDC_DIR% as the %UE4-DDC-NAME%
setx %UE4-DDC-NAME% %DDC_DIR% > NUL
set %UE4-DDC-NAME%=%DDC_DIR% > NUL
mkdir %DDC_DIR% > NUL
set UE4-DDC-NAME=
set DDC_DIR=

:GET_PACKAGE
rem call the script to fetch and launch the editor
%PM_PYTHON% %EPICGAMES_ROOT%\scripts\gitpackage\get-ue4.py --ddc=%NODDC% --launch=%LAUNCH%

:ENDOFFILE
