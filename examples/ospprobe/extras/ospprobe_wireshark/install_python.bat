@ECHO OFF
REM install-python.bat - creates virtual python env for an OSP extcap in WiresharkPortable64
REM Note the "SET PYTHONDIR" below

SET WIRESHARKDATA=..\WiresharkPortable64\Data
IF NOT EXIST %WIRESHARKDATA% (
   ECHO Expected directory %WIRESHARKDATA%
   EXIT /b 1
)
IF NOT EXIST %WIRESHARKDATA%\extcap (
   ECHO Creating directory %WIRESHARKDATA%\extcap
   MKDIR %WIRESHARKDATA%\extcap
)

REM Set the PYTHONDIR to path for python.exe (3.7 or higher: uses f-strings) (must end in \)
SET PYTHONDIR=C:\Programs\Python\
IF NOT EXIST %PYTHONDIR%python.exe (
  ECHO No python.exe in %PYTHONDIR%
  ECHO Patch line 16 in %~f0
  EXIT /b
)

ECHO Creating virtual python environment
%PYTHONDIR%python.exe -m venv %WIRESHARKDATA%\extcap\pyenv
CALL %WIRESHARKDATA%\extcap\pyenv\Scripts\activate.bat

ECHO Upgrading pip
python -m pip install -q --upgrade pip setuptools wheel

ECHO Adding python packages
pip install -q pyserial crcmod


Echo Done installing python for wireshark extcap
