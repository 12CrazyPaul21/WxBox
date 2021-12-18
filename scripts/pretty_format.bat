@echo off
for /r %MESON_SOURCE_ROOT%\WxBot %%i in (*.c, *.cpp, *.cc, *.cxx, *.h, *.hpp) do (%MESON_SOURCE_ROOT%\wintools\bin\clang-format.exe -i %%i)
for /r %MESON_SOURCE_ROOT%\WxBox %%i in (*.c, *.cpp, *.cc, *.cxx, *.h, *.hpp) do (%MESON_SOURCE_ROOT%\wintools\bin\clang-format.exe -i %%i)