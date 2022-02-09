@echo off

set WXBOX_PROJ_ROOT=%MESON_SOURCE_ROOT%
set WXBOX_MAIN_PROGRAM_NAME=%1%
set WXBOX_BUILD_TYPE=%2%
set WXBOX_VERSION=%3%
set ZLIB_BIN=%4%
set ZLIB_BIN=%ZLIB_BIN:/=\%
set OPENSSL_BIN_ROOT=%5%

set WXBOX_PREFIX=wxbox-%WXBOX_VERSION%
set WXBOX_PDB_PREFIX=%WXBOX_PREFIX%-pdb

:: project and symbols dist base filename
set WXBOX_DIST_FILE_NAME=%WXBOX_PREFIX%-%WXBOX_BUILD_TYPE%
set WXBOX_PDB_DIST_FILE_NAME=%WXBOX_DIST_FILE_NAME%-pdb

set WXBOX_DIST_ROOT=%WXBOX_PROJ_ROOT%\build\dist\%WXBOX_PREFIX%
set WXBOX_INSTALL_ROOT=%WXBOX_PROJ_ROOT%\build\install\%WXBOX_BUILD_TYPE%\%WXBOX_PREFIX%
set WXBOX_PDB_ROOT=%WXBOX_INSTALL_ROOT%\..\%WXBOX_PDB_PREFIX%

set VC_REDIST_EXTRACT_ROOT=%WXBOX_INSTALL_ROOT%\vc_redist_extract
set VC_REDIST_CAB_PATH=%VC_REDIST_EXTRACT_ROOT%\AttachedContainer\packages\vcRuntimeMinimum_x86\cab1.cab
set VCRUNTIME_EXTRACT_ROOT=%WXBOX_INSTALL_ROOT%\vcruntime_extract

set WINTOOLS_BIN_ROOT=%WXBOX_PROJ_ROOT%\wintools\bin
set WINTOOLS_DARK=%WINTOOLS_BIN_ROOT%\dark.exe
set WINTOOLS_7Z=%WINTOOLS_BIN_ROOT%\7za.exe
set WINTOOLS_TAR=%WINTOOLS_BIN_ROOT%\tar.exe
set WINTOOLS_XZ=%WINTOOLS_BIN_ROOT%\xz.exe

:: make sure folder exist
mkdir %WXBOX_DIST_ROOT% 2>nul
mkdir %WXBOX_PDB_ROOT% 2>nul
mkdir %VCRUNTIME_EXTRACT_ROOT% 2>nul

:: copy zlib
copy /Y %ZLIB_BIN% %WXBOX_INSTALL_ROOT%

:: copy openssl bin
copy /Y %OPENSSL_BIN_ROOT%\libcrypto*.dll %WXBOX_INSTALL_ROOT%
copy /Y %OPENSSL_BIN_ROOT%\libssl*.dll %WXBOX_INSTALL_ROOT%
copy /Y %OPENSSL_BIN_ROOT%\libeay*.dll %WXBOX_INSTALL_ROOT%
copy /Y %OPENSSL_BIN_ROOT%\ssleay*.dll %WXBOX_INSTALL_ROOT%

:: collect pdb
move %WXBOX_INSTALL_ROOT%\*.pdb %WXBOX_PDB_ROOT%

:: handle qt dependency
windeployqt %WXBOX_INSTALL_ROOT%\%WXBOX_MAIN_PROGRAM_NAME% --verbose 0 --no-svg --no-angle --no-opengl --no-opengl-sw --no-system-d3d-compiler --compiler-runtime --no-translations --no-webkit2 --no-virtualkeyboard

:: extract vc_redist
if not exist %WXBOX_INSTALL_ROOT%\vc_redist.x86.exe goto SKIP_EXTRACT_VCRUNTIME
%WINTOOLS_DARK% -nologo -x %VC_REDIST_EXTRACT_ROOT% %WXBOX_INSTALL_ROOT%\vc_redist.x86.exe
expand -F:* %VC_REDIST_CAB_PATH% %VCRUNTIME_EXTRACT_ROOT%

:: copy vcruntime
copy /Y %VCRUNTIME_EXTRACT_ROOT%\vcruntime*.dll %WXBOX_INSTALL_ROOT%
copy /Y %VCRUNTIME_EXTRACT_ROOT%\api_ms_win_crt_runtime*.dll %WXBOX_INSTALL_ROOT%
copy /Y %VCRUNTIME_EXTRACT_ROOT%\api_ms_win_crt_heap*.dll %WXBOX_INSTALL_ROOT%
copy /Y %VCRUNTIME_EXTRACT_ROOT%\api_ms_win_crt_string*.dll %WXBOX_INSTALL_ROOT%
copy /Y %VCRUNTIME_EXTRACT_ROOT%\api_ms_win_crt_stdio*.dll %WXBOX_INSTALL_ROOT%
copy /Y %VCRUNTIME_EXTRACT_ROOT%\api_ms_win_crt_convert*.dll %WXBOX_INSTALL_ROOT%

:SKIP_EXTRACT_VCRUNTIME

:: delete useless things
del /f /q %WXBOX_INSTALL_ROOT%\*.lib 2>nul
del /f /q %WXBOX_INSTALL_ROOT%\vc_redist* 2>nul
del /f /q %WXBOX_PDB_DIST_FILE_NAME%.tar 2>nul
del /f /q %WXBOX_DIST_FILE_NAME%.tar 2>nul
del /f /q %WXBOX_DIST_ROOT%\%WXBOX_PDB_DIST_FILE_NAME%.tar.xz 2>nul
del /f /q %WXBOX_DIST_ROOT%\%WXBOX_DIST_FILE_NAME%.tar.xz 2>nul
rmdir /s /q %VC_REDIST_EXTRACT_ROOT% 2>nul
rmdir /s /q %VCRUNTIME_EXTRACT_ROOT% 2>nul

:: pack symbols
%WINTOOLS_TAR% cvf %WXBOX_PDB_DIST_FILE_NAME%.tar -C %WXBOX_INSTALL_ROOT%\.. %WXBOX_PDB_PREFIX%
move %WXBOX_PDB_DIST_FILE_NAME%.tar %WXBOX_DIST_ROOT%
%WINTOOLS_XZ% -z -9 %WXBOX_DIST_ROOT%\%WXBOX_PDB_DIST_FILE_NAME%.tar
%WINTOOLS_7Z% a %WXBOX_DIST_ROOT%\%WXBOX_PDB_DIST_FILE_NAME%.zip %WXBOX_PDB_ROOT%

:: pack dist
%WINTOOLS_TAR% cvf %WXBOX_DIST_FILE_NAME%.tar -C %WXBOX_INSTALL_ROOT%\.. %WXBOX_PREFIX%
move %WXBOX_DIST_FILE_NAME%.tar %WXBOX_DIST_ROOT%
%WINTOOLS_XZ% -z -9 %WXBOX_DIST_ROOT%\%WXBOX_DIST_FILE_NAME%.tar
%WINTOOLS_7Z% a %WXBOX_DIST_ROOT%\%WXBOX_DIST_FILE_NAME%.zip %WXBOX_INSTALL_ROOT%

:: clean
rmdir /s /q %WXBOX_INSTALL_ROOT% 2>nul
rmdir /s /q %WXBOX_PDB_ROOT% 2>nul

exit 0