# | canvas module for LuaRT
# | Luart.org, Copyright (c) Tine Samir 2024.
# | See Copyright Notice in LICENSE.TXT
# |--------------------------------------------------------------
# | Makefile
# | Please set LUART_PATH to your LuaRT folder if autodetection fails
# |--------------------------------------------------------------
# | Usage (default release build)			 : make
# | Usage (debug build) 		  			 : make debug
# | Usage (clean all)	 				 	 : make clean
# |-------------------------------------------------------------
VERBOSE= >nul 2>&1

MODULE=		canvas
VERSION=	1.0
SRC= 		src\Canvas.obj src\Direct2D.obj src\Image.obj src\Gradient.obj

LUALIB= "$(LUART_PATH)\lib\lua54.lib"
CFLAGS = 

!if "$(BUILD)" == "debug"
CFLAGS = /nologo /DLUA_BUILD_AS_DLL /D_WIN32_WINNT=0x0603 /DLUA_COMPAT_5_3 /DLUA_ARCH=\"$(_ARCH)\" /DLUART_MINOR=$(LUART_MINOR) /DLUART_MAJOR=$(LUART_MAJOR) /DLUART_RELEASE=$(LUART_RELEASE) /I"$(LUART_PATH)\include" /I"." /Z7 /EHsc
LDFLAGS= /nologo /DEBUG 
MODNAME=$(MODULE)
VERBOSE=
PROGRESS= 
!else
!if "$(BUILD)" == "static"
LUALIB= "$(LUART_PATH)\lib\lua54-static.lib"
CFLAGS = /nologo /DRTSTATIC /DLUA_BUILD_AS_DLL /D_WIN32_WINNT=0x0603 /DLUA_COMPAT_5_3 /DLUA_ARCH=\"$(_ARCH)\" /DLUART_MINOR=$(LUART_MINOR) /DLUART_MAJOR=$(LUART_MAJOR) /DLUART_RELEASE=$(LUART_RELEASE) /I"$(LUART_PATH)\include" /I"." /MD /Gy /GF /GR- /GL /GS- /Oi /O1 /GA /EHsc
LDFLAGS= /nologo /ignore:4217 /OPT:REF /OPT:ICF /LTCG /NODEFAULTLIB:libcmt
MODNAME=$(MODULE)-static
!else
CFLAGS = /nologo /DLUA_BUILD_AS_DLL /D_WIN32_WINNT=0x0603 /DLUA_COMPAT_5_3 /DLUA_ARCH=\"$(_ARCH)\" /DLUART_MINOR=$(LUART_MINOR) /DLUART_MAJOR=$(LUART_MAJOR) /DLUART_RELEASE=$(LUART_RELEASE) /I"$(LUART_PATH)\include" /I"." /MD /Gy /GF /GR- /GL /GS- /Oi /O1 /GA /EHsc
LDFLAGS= /nologo /OPT:REF /OPT:REF /OPT:ICF /LTCG
MODNAME=$(MODULE)
!endif
PROGRESS= "■"
!endif

LIBS= $(LUALIB) d2d1.lib dwrite.lib ole32.lib windowscodecs.lib comctl32.lib user32.lib gdi32.lib
RM= del /Q
CP= copy /Y
LD= link.exe

all: infomodule $(MODNAME).dll

static: 
	@$(MAKE) /nologo "BUILD=static"

debug:
	@$(MAKE) /nologo "BUILD=debug"

infomodule: 
	@chcp 65001 >nul 2>&1
!if "$(BUILD)" != "static"
	@cmd /c echo.
	@echo|set /p dummy="▸  Building module $(MODNAME) $(VERSION)	  "
!endif

.c.obj: 
	@$(CC) $(CFLAGS) /c /Fo$@ $< $(VERBOSE)
	@echo|set /p dummy=$(PROGRESS)

.cpp.obj: 
	@$(CC) $(CFLAGS) /c /Fo$@ $< $(VERBOSE)
	@echo|set /p dummy=$(PROGRESS)

$(MODNAME).dll: $(SRC)
	@$(LD) /DLL $(LDFLAGS) $(LIBS) $(SRC) /OUT:$@ $(VERBOSE)
	@-mkdir $(LUART_PATH)\modules\$(MODULE) >nul 2>&1
	@$(CP) $@ $(LUART_PATH)\modules\$(MODULE) >nul 2>&1
	@$(RM) $(SRC)
!if "$(BUILD)" != "static"
	@$(MAKE) /nologo static
!endif

clean:
	@echo|set /p dummy="▸  Cleaning $(MODULE) module... "
	@$(RM) $(MODULE)*.* >nul 2>&1
	@$(RM) $(SRC) >nul 2>&1
	@echo ✓
	@-$(RM) $(LUART_PATH)\modules\$(MODULE)\$(MODULE).dll >nul 2>&1
	@-$(RM) $(LUART_PATH)\modules\$(MODULE)\$(MODULE)-static.dll >nul 2>&1
	@-rmdir /S /Q $(LUART_PATH)\modules\$(MODULE) >nul 2>&1 