# Microsoft Developer Studio Generated NMAKE File, Based on BGB_Manager_Test.dsp
!IF "$(CFG)" == ""
CFG=BGB_Manager_Test - Win32 Debug
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの BGB_Manager_Test - Win32 Debug を設定します。
!ENDIF 

!IF "$(CFG)" != "BGB_Manager_Test - Win32 Release" && "$(CFG)" != "BGB_Manager_Test - Win32 Debug"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "BGB_Manager_Test.mak" CFG="BGB_Manager_Test - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "BGB_Manager_Test - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "BGB_Manager_Test - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
F90=df.exe
RSC=rc.exe

!IF  "$(CFG)" == "BGB_Manager_Test - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\BGB_Manager_Test.exe" "$(OUTDIR)\BGB_Manager_Test.bsc"


CLEAN :
	-@erase "$(INTDIR)\bgb_manager.obj"
	-@erase "$(INTDIR)\bgb_manager.sbr"
	-@erase "$(INTDIR)\LargeFileReader.obj"
	-@erase "$(INTDIR)\LargeFileReader.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\BGB_Manager_Test.bsc"
	-@erase "$(OUTDIR)\BGB_Manager_Test.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90_PROJ=/compile_only /include:"$(INTDIR)\\" /nologo /warn:nofileopt /module:"Release/" /object:"Release/" 
F90_OBJS=.\Release/
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I ".." /I "..\win32" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\BGB_Manager_Test.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BGB_Manager_Test.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\bgb_manager.sbr" \
	"$(INTDIR)\LargeFileReader.sbr" \
	"$(INTDIR)\main.sbr"

"$(OUTDIR)\BGB_Manager_Test.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\BGB_Manager_Test.pdb" /machine:I386 /out:"$(OUTDIR)\BGB_Manager_Test.exe" 
LINK32_OBJS= \
	"$(INTDIR)\bgb_manager.obj" \
	"$(INTDIR)\LargeFileReader.obj" \
	"$(INTDIR)\main.obj"

"$(OUTDIR)\BGB_Manager_Test.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "BGB_Manager_Test - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\BGB_Manager_Test.exe" "$(OUTDIR)\BGB_Manager_Test.bsc"


CLEAN :
	-@erase "$(INTDIR)\bgb_manager.obj"
	-@erase "$(INTDIR)\bgb_manager.sbr"
	-@erase "$(INTDIR)\LargeFileReader.obj"
	-@erase "$(INTDIR)\LargeFileReader.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\BGB_Manager_Test.bsc"
	-@erase "$(OUTDIR)\BGB_Manager_Test.exe"
	-@erase "$(OUTDIR)\BGB_Manager_Test.ilk"
	-@erase "$(OUTDIR)\BGB_Manager_Test.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90_PROJ=/browser:"Debug/" /check:bounds /compile_only /debug:full /include:"$(INTDIR)\\" /nologo /traceback /warn:argument_checking /warn:nofileopt /module:"Debug/" /object:"Debug/" /pdbfile:"Debug/DF60.PDB" 
F90_OBJS=.\Debug/
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".." /I "..\win32" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\BGB_Manager_Test.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BGB_Manager_Test.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\bgb_manager.sbr" \
	"$(INTDIR)\LargeFileReader.sbr" \
	"$(INTDIR)\main.sbr"

"$(OUTDIR)\BGB_Manager_Test.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\BGB_Manager_Test.pdb" /debug /machine:I386 /out:"$(OUTDIR)\BGB_Manager_Test.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\bgb_manager.obj" \
	"$(INTDIR)\LargeFileReader.obj" \
	"$(INTDIR)\main.obj"

"$(OUTDIR)\BGB_Manager_Test.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.SUFFIXES: .fpp

.for{$(F90_OBJS)}.obj:
   $(F90) $(F90_PROJ) $<  

.f{$(F90_OBJS)}.obj:
   $(F90) $(F90_PROJ) $<  

.f90{$(F90_OBJS)}.obj:
   $(F90) $(F90_PROJ) $<  

.fpp{$(F90_OBJS)}.obj:
   $(F90) $(F90_PROJ) $<  


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("BGB_Manager_Test.dep")
!INCLUDE "BGB_Manager_Test.dep"
!ELSE 
!MESSAGE Warning: cannot find "BGB_Manager_Test.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "BGB_Manager_Test - Win32 Release" || "$(CFG)" == "BGB_Manager_Test - Win32 Debug"
SOURCE=..\bgb_manager.cpp

"$(INTDIR)\bgb_manager.obj"	"$(INTDIR)\bgb_manager.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\win32\LargeFileReader.cpp

"$(INTDIR)\LargeFileReader.obj"	"$(INTDIR)\LargeFileReader.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\main.cpp

"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)"



!ENDIF 

