# Microsoft Developer Studio Generated NMAKE File, Based on BinViewer.dsp
!IF "$(CFG)" == ""
CFG=BinViewer - Win32 Debug
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの BinViewer - Win32 Debug を設定します。
!ENDIF 

!IF "$(CFG)" != "BinViewer - Win32 Release" && "$(CFG)" != "BinViewer - Win32 Debug"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "BinViewer.mak" CFG="BinViewer - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "BinViewer - Win32 Release" ("Win32 (x86) Application" 用)
!MESSAGE "BinViewer - Win32 Debug" ("Win32 (x86) Application" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "BinViewer - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\BinViewer.exe" "$(OUTDIR)\BinViewer.bsc"


CLEAN :
	-@erase "$(INTDIR)\bgb_manager.obj"
	-@erase "$(INTDIR)\bgb_manager.sbr"
	-@erase "$(INTDIR)\BitmapView.obj"
	-@erase "$(INTDIR)\BitmapView.sbr"
	-@erase "$(INTDIR)\configdlg.obj"
	-@erase "$(INTDIR)\configdlg.sbr"
	-@erase "$(INTDIR)\dc_manager.obj"
	-@erase "$(INTDIR)\dc_manager.sbr"
	-@erase "$(INTDIR)\dialog.obj"
	-@erase "$(INTDIR)\dialog.sbr"
	-@erase "$(INTDIR)\drawinfo.obj"
	-@erase "$(INTDIR)\drawinfo.sbr"
	-@erase "$(INTDIR)\jumpdlg.obj"
	-@erase "$(INTDIR)\jumpdlg.sbr"
	-@erase "$(INTDIR)\LargeFileReader.obj"
	-@erase "$(INTDIR)\LargeFileReader.sbr"
	-@erase "$(INTDIR)\LF_Notify.obj"
	-@erase "$(INTDIR)\LF_Notify.sbr"
	-@erase "$(INTDIR)\lock.obj"
	-@erase "$(INTDIR)\lock.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\mainwnd.res"
	-@erase "$(INTDIR)\searchdlg.obj"
	-@erase "$(INTDIR)\searchdlg.sbr"
	-@erase "$(INTDIR)\strutils.obj"
	-@erase "$(INTDIR)\strutils.sbr"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\thread.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\viewframe.obj"
	-@erase "$(INTDIR)\viewframe.sbr"
	-@erase "$(OUTDIR)\BinViewer.bsc"
	-@erase "$(OUTDIR)\BinViewer.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP_PROJ=/nologo /MT /W3 /GR /GX /Zi /O2 /I "." /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D SIDEBYSIDE_COMMONCONTROLS=1 /D _WIN32_WINNT=0x500 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\BinViewer.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\mainwnd.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BinViewer.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\bgb_manager.sbr" \
	"$(INTDIR)\BitmapView.sbr" \
	"$(INTDIR)\configdlg.sbr" \
	"$(INTDIR)\dc_manager.sbr" \
	"$(INTDIR)\dialog.sbr" \
	"$(INTDIR)\drawinfo.sbr" \
	"$(INTDIR)\jumpdlg.sbr" \
	"$(INTDIR)\LargeFileReader.sbr" \
	"$(INTDIR)\LF_Notify.sbr" \
	"$(INTDIR)\lock.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\searchdlg.sbr" \
	"$(INTDIR)\strutils.sbr" \
	"$(INTDIR)\thread.sbr" \
	"$(INTDIR)\viewframe.sbr"

"$(OUTDIR)\BinViewer.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /profile /debug /machine:I386 /out:"$(OUTDIR)\BinViewer.exe" 
LINK32_OBJS= \
	"$(INTDIR)\bgb_manager.obj" \
	"$(INTDIR)\BitmapView.obj" \
	"$(INTDIR)\configdlg.obj" \
	"$(INTDIR)\dc_manager.obj" \
	"$(INTDIR)\dialog.obj" \
	"$(INTDIR)\drawinfo.obj" \
	"$(INTDIR)\jumpdlg.obj" \
	"$(INTDIR)\LargeFileReader.obj" \
	"$(INTDIR)\LF_Notify.obj" \
	"$(INTDIR)\lock.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\searchdlg.obj" \
	"$(INTDIR)\strutils.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\viewframe.obj" \
	"$(INTDIR)\mainwnd.res"

"$(OUTDIR)\BinViewer.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "BinViewer - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\BinViewer.exe" "$(OUTDIR)\BinViewer.bsc"


CLEAN :
	-@erase "$(INTDIR)\bgb_manager.obj"
	-@erase "$(INTDIR)\bgb_manager.sbr"
	-@erase "$(INTDIR)\BitmapView.obj"
	-@erase "$(INTDIR)\BitmapView.sbr"
	-@erase "$(INTDIR)\configdlg.obj"
	-@erase "$(INTDIR)\configdlg.sbr"
	-@erase "$(INTDIR)\dc_manager.obj"
	-@erase "$(INTDIR)\dc_manager.sbr"
	-@erase "$(INTDIR)\dialog.obj"
	-@erase "$(INTDIR)\dialog.sbr"
	-@erase "$(INTDIR)\drawinfo.obj"
	-@erase "$(INTDIR)\drawinfo.sbr"
	-@erase "$(INTDIR)\jumpdlg.obj"
	-@erase "$(INTDIR)\jumpdlg.sbr"
	-@erase "$(INTDIR)\LargeFileReader.obj"
	-@erase "$(INTDIR)\LargeFileReader.sbr"
	-@erase "$(INTDIR)\LF_Notify.obj"
	-@erase "$(INTDIR)\LF_Notify.sbr"
	-@erase "$(INTDIR)\lock.obj"
	-@erase "$(INTDIR)\lock.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\mainwnd.res"
	-@erase "$(INTDIR)\searchdlg.obj"
	-@erase "$(INTDIR)\searchdlg.sbr"
	-@erase "$(INTDIR)\strutils.obj"
	-@erase "$(INTDIR)\strutils.sbr"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\thread.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\viewframe.obj"
	-@erase "$(INTDIR)\viewframe.sbr"
	-@erase "$(OUTDIR)\BinViewer.bsc"
	-@erase "$(OUTDIR)\BinViewer.exe"
	-@erase "$(OUTDIR)\BinViewer.ilk"
	-@erase "$(OUTDIR)\BinViewer.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "." /I ".." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D SIDEBYSIDE_COMMONCONTROLS=1 /D _WIN32_WINNT=0x500 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\BinViewer.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\mainwnd.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BinViewer.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\bgb_manager.sbr" \
	"$(INTDIR)\BitmapView.sbr" \
	"$(INTDIR)\configdlg.sbr" \
	"$(INTDIR)\dc_manager.sbr" \
	"$(INTDIR)\dialog.sbr" \
	"$(INTDIR)\drawinfo.sbr" \
	"$(INTDIR)\jumpdlg.sbr" \
	"$(INTDIR)\LargeFileReader.sbr" \
	"$(INTDIR)\LF_Notify.sbr" \
	"$(INTDIR)\lock.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\searchdlg.sbr" \
	"$(INTDIR)\strutils.sbr" \
	"$(INTDIR)\thread.sbr" \
	"$(INTDIR)\viewframe.sbr"

"$(OUTDIR)\BinViewer.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\BinViewer.pdb" /debug /machine:I386 /out:"$(OUTDIR)\BinViewer.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\bgb_manager.obj" \
	"$(INTDIR)\BitmapView.obj" \
	"$(INTDIR)\configdlg.obj" \
	"$(INTDIR)\dc_manager.obj" \
	"$(INTDIR)\dialog.obj" \
	"$(INTDIR)\drawinfo.obj" \
	"$(INTDIR)\jumpdlg.obj" \
	"$(INTDIR)\LargeFileReader.obj" \
	"$(INTDIR)\LF_Notify.obj" \
	"$(INTDIR)\lock.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\searchdlg.obj" \
	"$(INTDIR)\strutils.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\viewframe.obj" \
	"$(INTDIR)\mainwnd.res"

"$(OUTDIR)\BinViewer.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("BinViewer.dep")
!INCLUDE "BinViewer.dep"
!ELSE 
!MESSAGE Warning: cannot find "BinViewer.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "BinViewer - Win32 Release" || "$(CFG)" == "BinViewer - Win32 Debug"
SOURCE=..\bgb_manager.cpp

"$(INTDIR)\bgb_manager.obj"	"$(INTDIR)\bgb_manager.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\BitmapView.cpp

"$(INTDIR)\BitmapView.obj"	"$(INTDIR)\BitmapView.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\configdlg.cpp

"$(INTDIR)\configdlg.obj"	"$(INTDIR)\configdlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dc_manager.cpp

"$(INTDIR)\dc_manager.obj"	"$(INTDIR)\dc_manager.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dialog.cpp

"$(INTDIR)\dialog.obj"	"$(INTDIR)\dialog.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\drawinfo.cpp

"$(INTDIR)\drawinfo.obj"	"$(INTDIR)\drawinfo.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\jumpdlg.cpp

"$(INTDIR)\jumpdlg.obj"	"$(INTDIR)\jumpdlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\LargeFileReader.cpp

"$(INTDIR)\LargeFileReader.obj"	"$(INTDIR)\LargeFileReader.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=..\LF_Notify.cpp

"$(INTDIR)\LF_Notify.obj"	"$(INTDIR)\LF_Notify.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\lock.cpp

"$(INTDIR)\lock.obj"	"$(INTDIR)\lock.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\main.cpp

"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\searchdlg.cpp

"$(INTDIR)\searchdlg.obj"	"$(INTDIR)\searchdlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\strutils.cpp

"$(INTDIR)\strutils.obj"	"$(INTDIR)\strutils.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\thread.cpp

"$(INTDIR)\thread.obj"	"$(INTDIR)\thread.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\viewframe.cpp

"$(INTDIR)\viewframe.obj"	"$(INTDIR)\viewframe.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mainwnd.rc

"$(INTDIR)\mainwnd.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

