@echo off

set dir=%cd%
if "%1" == "setup" (
    for /f %%i in ('dir /ad/b "%dir%"') do @(@if exist %%i/nmake.bat @(@cd %%i & @nmake.bat & @cd ..) else @(@if exist %%i/Makefile @(@cd %%i & @nmake.exe & @cd ..)))  
    @cd core
    @nmake.bat %1 & @cd ..
) else (
    for /f %%i in ('dir /ad/b "%dir%"') do @(@if exist %%i/nmake.bat @(@cd %%i & @nmake.bat %1 & @cd ..) else @(@if exist %%i/Makefile @(@cd %%i & @nmake.exe %1 & @cd ..)))
)