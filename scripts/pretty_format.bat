@echo off
for /r .\WxBot %%i in (*.cpp, *.cc, *.cxx, *.h, *.hpp) do (.\wintools\bin\clang-format.exe -i %%i)
for /r .\WxBox %%i in (*.cpp, *.cc, *.cxx, *.h, *.hpp) do (.\wintools\bin\clang-format.exe -i %%i)