@echo off

copy /Y ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tif_config.vc.h ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tif_config.h
copy /Y ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tiffconf.vc.h ..\thirdparty\wxWidgets-3.2.4\src\tiff\libtiff\tiffconf.h

msbuild ManifoldEditor.sln /p:Configuration="%1"

post-build.bat "%1"
