NAME = kuwahara
#SDKDIR = SPECIFY_PATH_TO_PLUGIN_SDK # ex. C:/adobe_photoshop_sdk_cc_2017_win/pluginsdk
SDKDIR = F:\VisualStudio\adobe_photoshop_sdk_cc_2017_win\pluginsdk

TARGETNAME = $(NAME).8bf
OUTDIR=.\output

CXXFLAGS=  \
    /GS /W3 /Zc:wchar_t \
    /I"$(SDKDIR)\PhotoshopAPI" \
    /I"$(SDKDIR)\PhotoshopAPI\Photoshop" \
    /I"$(SDKDIR)\PhotoshopAPI\PICA_SP" \
    /I"$(SDKDIR)\samplecode\common\Includes" \
    /I F:\VisualStudio\opencv\build\include \
    /Gm- /Od /Zc:inline /fp:precise \
    /D "ISOLATION_AWARE_ENABLED=1" /D "WIN32=1"  \
    /D "_CRT_SECURE_NO_DEPRECATE" /D "_SCL_SECURE_NO_DEPRECATE" \
    /D "_WINDOWS" /D "_USRDLL" \
    /D "_VC80_UPGRADE=0x0710" /D "_WINDLL" /D "_MBCS" \
    /WX- /Zc:forScope /RTC1 /Gd /MT /std:c++14 /EHsc

LDFLAGS= \
    /OUT:"$(OUTDIR)\$(NAME).8bf" /MANIFEST \
    /DYNAMICBASE:NO "odbc32.lib" "odbccp32.lib" "version.lib" "kernel32.lib" "user32.lib" \
    "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" \
    "oleaut32.lib" "uuid.lib" \
    F:\VisualStudio\opencv\build\x64\vc15\lib\opencv_world410.lib \
    /IMPLIB:"$(OUTDIR)/$(NAME).lib" /DLL /MACHINE:X64 /INCREMENTAL \
    /MAP /ERRORREPORT:PROMPT /NOLOGO /TLBID:1 


CXX=cl
LINK=link

OBJECTS=$(NAME).obj PIUFile.obj DialogUtilitiesWin.obj PIDLLInstance.obj PIUSuites.obj PIUtilities.obj PIUtilitiesWin.obj 

ALL : 
# PIPL
    cl /I$(SDKDIR)\samplecode\common\Includes /I$(SDKDIR)\PhotoshopAPI\Photoshop \
        /I$(SDKDIR)\PhotoshopAPI\PICA_SP /I$(SDKDIR)\samplecode\common\Resources \
        /EP /DMSWindows=1 /DWIN32=1 /Tc"./common/$(NAME).r" > "$(OUTDIR)\$(NAME).rr"
    $(SDKDIR)\samplecode\resources\cnvtpipl.exe "$(OUTDIR)\$(NAME).rr" "$(OUTDIR)\$(NAME).pipl"
    del "$(OUTDIR)\$(NAME).rr"

# RC
    rc /I"..\common" /I"$(SDKDIR)\samplecode\common\resources" /I"$(OUTDIR)" /D "_VC80_UPGRADE=0x0710" /l 0x0409 /nologo /fo"$(OUTDIR)\$(NAME).res"  ./win/$(NAME).rc

# Library compilation
    $(CXX) /c $(CXXFLAGS) "$(SDKDIR)\samplecode\common\sources\PIUFile.cpp"
    $(CXX) /c $(CXXFLAGS) "$(SDKDIR)\samplecode\common\sources\DialogUtilitiesWin.cpp"
    $(CXX) /c $(CXXFLAGS) "$(SDKDIR)\samplecode\common\sources\PIDLLInstance.cpp"
    $(CXX) /c $(CXXFLAGS) "$(SDKDIR)\samplecode\common\sources\PIUSuites.cpp"
    $(CXX) /c $(CXXFLAGS) "$(SDKDIR)\samplecode\common\sources\PIUtilities.cpp"
    $(CXX) /c $(CXXFLAGS) "$(SDKDIR)\samplecode\common\sources\PIUtilitiesWin.cpp"
    
# Plugin source compilation
    $(CXX) /c $(CXXFLAGS) ".\common\$(NAME).cpp"
    
# Linking
    $(LINK)   /OUT:$(TARGETNAME) $(LDFLAGS) $(OBJECTS) "$(OUTDIR)\$(NAME).res"

    copy "$(OUTDIR)\$(TARGETNAME)" "C:\Program Files\Adobe\Adobe Photoshop CS4 (64 Bit)\Plug-ins\Yoshimizu\"

.PHONY:clean
clean:
    del $(PROGRAM)  *.obj
    -@erase /Q $(OUTDIR)\*
