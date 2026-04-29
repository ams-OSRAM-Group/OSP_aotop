@ECHO OFF
REM For development, make Programs dir a link to the SVN dir

SET SOURCE=%CD%\ospprobe_wireshark
SET DEST=C:\Programs\ospprobe_wireshark

ECHO Need admin rights
ECHO SOURCE %SOURCE% 
ECHO DEST   %DEST%
ECHO.

IF EXIST %DEST% rmdir /Q %DEST%
mklink /D %DEST%  %SOURCE%

