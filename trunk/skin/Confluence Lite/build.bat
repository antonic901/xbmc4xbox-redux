@echo off
ECHO ----------------------------------------
echo Creating Confluence Lite Build Folder
rmdir BUILD /S /Q
md BUILD

Echo .svn>exclude.txt
Echo Thumbs.db>>exclude.txt
Echo Desktop.ini>>exclude.txt
Echo dsstdfx.bin>>exclude.txt
Echo exclude.txt>>exclude.txt

ECHO ----------------------------------------
ECHO Creating XPR File...
START /B /WAIT ..\..\Tools\XBMCTex\XBMCTex -input media -output media -noprotect

ECHO ----------------------------------------
ECHO Copying XPR File...
xcopy "media\Textures.xpr" "BUILD\Confluence Lite\media\" /Q /I /Y

ECHO ----------------------------------------
ECHO Cleaning Up...
del "media\Textures.xpr"

ECHO ----------------------------------------
ECHO XPR Texture Files Created...
ECHO Building Skin Directory...
xcopy "720p" "BUILD\Confluence Lite\720p" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "NTSC16x9" "BUILD\Confluence Lite\NTSC16x9" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "PAL16x9" "BUILD\Confluence Lite\PAL16x9" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "fonts" "BUILD\Confluence Lite\fonts" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "backgrounds" "BUILD\Confluence Lite\backgrounds" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "sounds\*.*" "BUILD\Confluence Lite\sounds\" /Q /I /Y /EXCLUDE:exclude.txt
xcopy "colors\*.*" "BUILD\Confluence Lite\colors\" /Q /I /Y /EXCLUDE:exclude.txt
xcopy "language" "BUILD\Confluence Lite\language" /E /Q /I /Y /EXCLUDE:exclude.txt
xcopy "scripts" "BUILD\Confluence Lite\scripts" /E /Q /I /Y /EXCLUDE:exclude.txt

del exclude.txt

copy *.xml "BUILD\Confluence Lite\"
copy *.txt "BUILD\Confluence Lite\"
 	  	 
