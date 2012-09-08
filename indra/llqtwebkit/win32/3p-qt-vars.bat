@echo off
echo Setting up a Qt environment using 3p-qt HG repository
set QTDIR=C:\Work\3p-llqtwebkit\stage
set PATH=C:\Work\3p-llqtwebkit\stage\bin;%PATH%
set QMAKESPEC=win32-msvc2010
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\vsvars32.bat"
