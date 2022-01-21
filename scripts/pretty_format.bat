@echo off

if not "%MESON_SOURCE_ROOT%" == "" (
    goto Begin
)

pushd "%~dp0%.."
set MESON_SOURCE_ROOT=%cd%
popd

:Begin
for /r %MESON_SOURCE_ROOT%\src %%i in (*.c, *.cpp, *.cc, *.cxx, *.h, *.hpp) do (%MESON_SOURCE_ROOT%\wintools\bin\clang-format.exe -i %%i)