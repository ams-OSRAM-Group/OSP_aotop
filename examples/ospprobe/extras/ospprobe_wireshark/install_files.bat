@ECHO OFF
REM install_files.bat - install extcap and dissector for OSP in WiresharkPortable64


SET WIRESHARKDATA=..\WiresharkPortable64\Data
IF NOT EXIST %WIRESHARKDATA% (
   ECHO Expected directory %WIRESHARKDATA%
   EXIT /b 1
)
IF NOT EXIST %WIRESHARKDATA%\extcap (
   ECHO Creating directory %WIRESHARKDATA%\extcap
   MKDIR %WIRESHARKDATA%\extcap
)
IF NOT EXIST %WIRESHARKDATA%\plugins (
   ECHO Creating directory %WIRESHARKDATA%\plugins
   MKDIR %WIRESHARKDATA%\plugins
)
IF NOT EXIST %WIRESHARKDATA%\profiles (
   ECHO Creating directory %WIRESHARKDATA%\profiles
   MKDIR %WIRESHARKDATA%\profiles
)


ECHO Installing extcap (external capture utility)
COPY /Y ospprobe_extcap.bat.src  %WIRESHARKDATA%\extcap\ospprobe_extcap.bat > NUL
COPY /Y ospprobe_extcap.py.src   %WIRESHARKDATA%\extcap\ospprobe_extcap.py  > NUL

ECHO Installing dissector (osp telegrams)
COPY /Y ospprobe-dissect.lua.src  %WIRESHARKDATA%\plugins\ospprobe-dissect.lua > NUL

ECHO Installing profile (osp view)
XCOPY /Y /I /Q profile-OSP %WIRESHARKDATA%\profiles\OSP > NUL


ECHO Done installing files