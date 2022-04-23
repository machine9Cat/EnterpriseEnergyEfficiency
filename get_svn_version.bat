@echo off

for /f "delims=" %%t in ('SubWcRev %CD% -f ^| findstr "Updated to revision"') do set version=%%t

if "%version%" equ "" (
  for /f "delims=" %%t in ('SubWcRev %CD% -f ^| findstr "Mixed revision range"') do set version=%%t 
  if "%version%" equ "" (
    echo "version string was empty"
    goto ERROR_EXIT
  ) else (
    echo "version:" %version%
  )
) else (
  echo "version:" %version%
)

REM
REM version = "Updated to revision 12345" , or,  "Mixed revision range 34046:34064"
REM 
for /f "tokens=4,*" %%a in ("%version%") do (
  REM echo %%a
  set version=%%a

  if "%version%" equ "" (
    echo "version number was empty"
    goto ERROR_EXIT
  )
  
  goto FIND_VERSION
)

:FIND_VERSION


REM
REM generate version.h
REM 
echo #ifndef VERSION_H > version.h
echo #define VERSION_H >> version.h
echo #define SOURCE_CODE_VERSION "%version%" >> version.h
echo #endif >> version.h

exit 0

:ERROR_EXIT

REM
REM generate version.h with #error to tell C compiler stop
REM 
echo "generate #error to stop C compiler"
echo #ifndef VERSION_H > version.h
echo #define VERSION_H >> version.h
echo #error "could not extract source code version" >> version.h
echo #endif >> version.h

exit 1