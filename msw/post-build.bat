@echo off

if "%1" == "Debug" goto Debug
if "%1" == "Release" goto Release
goto End

@echo on

:Debug
if not exist ..\bin\freetype.dll copy /y ..\thirdparty\freetype-2.13.3\objs\x64\Debug\freetype.dll ..\bin
if not exist ..\bin\Irrlicht.dll copy /y ..\thirdparty\irrlicht-1.8.5\bin\Win64-VisualStudio\Irrlicht.dll ..\bin
if not exist ..\bin\wxbase324ud.dll copy /y ..\thirdparty\wxWidgets-3.2.4\lib\vc_x64_dll\wxbase324ud.dll ..\bin
if not exist ..\bin\wxbase324ud_xml.dll copy /y ..\thirdparty\wxWidgets-3.2.4\lib\vc_x64_dll\wxbase324ud_xml.dll ..\bin
if not exist ..\bin\wxmsw324ud_aui.dll copy /y ..\thirdparty\wxWidgets-3.2.4\lib\vc_x64_dll\wxmsw324ud_aui.dll ..\bin
if not exist ..\bin\wxmsw324ud_core.dll copy /y ..\thirdparty\wxWidgets-3.2.4\lib\vc_x64_dll\wxmsw324ud_core.dll ..\bin
if not exist ..\bin\wxmsw324ud_gl.dll copy /y ..\thirdparty\wxWidgets-3.2.4\lib\vc_x64_dll\wxmsw324ud_gl.dll ..\bin
if not exist ..\bin\wxmsw324ud_propgrid.dll copy /y ..\thirdparty\wxWidgets-3.2.4\lib\vc_x64_dll\wxmsw324ud_propgrid.dll ..\bin
if not exist ..\bin\wxmsw324ud_stc.dll copy /y ..\thirdparty\wxWidgets-3.2.4\lib\vc_x64_dll\wxmsw324ud_stc.dll ..\bin
goto End

:Release

goto End

:End

@echo off
cd ..
thirdparty\doxygen\doxygen.exe Doxyfile
