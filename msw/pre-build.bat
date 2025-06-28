@echo off

if "%1" == "Debug" goto Debug
if "%1" == "Release" goto Release
goto End

@echo on

:Debug
copy /y ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tif_config.vc.h ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tif_config.h
copy /y ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tiffconf.vc.h ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tiffconf.h
goto End

:Release
copy /y ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tif_config.vc.h ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tif_config.h
copy /y ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tiffconf.vc.h ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tiffconf.h
goto End

:End
