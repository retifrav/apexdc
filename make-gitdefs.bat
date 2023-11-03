@echo off
set file=%1client\gitdefs.inc
set tmpfile=%1client\gitdefs.inc.tmp

if not exist %1.git\ goto :no_git

for /F "tokens=1,2 delims=-" %%a in ('git describe --abbrev"="4') do set short_tag=%%a-%%b
If %short_tag:~-1%==- (set short_tag=%short_tag:~0,-1%)
echo #define GIT_TAG_SHORT "%short_tag%" >> %tmpfile%

for /F "tokens=*" %%a in ('git describe --abbrev"="4 --dirty"="-d') do echo #define GIT_TAG_LONG "%%a" >> %tmpfile%
for /F "tokens=*" %%a in ('git rev-parse HEAD') do echo #define GIT_REF_COMMIT "%%a" >> %tmpfile%
for /F "tokens=*" %%a in ('git rev-list HEAD --count') do echo #define GIT_COMMIT_COUNT %%a >> %tmpfile%
for /F "tokens=*" %%a in ('git log -1 --format"="%%at') do echo #define VERSION_DATE %%a >> %tmpfile%

for /F "tokens=*" %%a in ('git rev-list 1.6.0..HEAD --count') do set /a compat_versionid=2165+%%a
echo #define COMPATIBLE_VERSIONID %compat_versionid% >> %tmpfile%

if not exist %file% goto :version_changed
fc /b %tmpfile% %file% > nul
if errorlevel 1 goto :version_changed

echo No Version changes detected, using the old gitdefs file...
goto :end

:version_changed
	echo Version changes detected updating the gitdefs file...
	copy /y %tmpfile% %file% > nul
goto :end

:no_git
	echo #define NO_VERSIONCONTROL 1 >> %tmpfile%

	echo Source tree not under version control, creating dummy gitdefs file...
	copy /y %tmpfile% %file% > nul
goto :end

:end
echo Y | del %tmpfile%
