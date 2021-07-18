@echo off
rem Test script written by Stepan Anokhin, debugged by Leonid Yadrennikov
rem May 2021

set m20=%~f1
set test_dir=%~f2
set timestamp=%DATE:/=-%_%TIME::=-%
set timestamp=%timestamp: =%
set timestamp=%timestamp:,=.%
set run_dir=%test_dir%\run-%timestamp%-%RANDOM%

echo RUN TESTS
echo.
echo M-20 Emulator:     %M20%
echo Tests Directory:   %test_dir%
echo Run Directory:     %run_dir%
echo.

rem Copy files to run directory
cd %test_dir%
if not exist %run_dir% mkdir %run_dir%
for %%I in (*.simh *.cdr *.m20 *.drum0) do copy %%I %run_dir% >NUL
cd %run_dir%

rem Execute tests
set test_count=0
for /r %%I in (test_*.simh) do set /a test_count=test_count+1
echo Running %test_count% tests

set failed_count=0
set success_count=0
for /r %%I in (test_*.simh) do (
    echo | set /p="%%~nxI ... "
    %m20% %%I > %%I.output 2>&1
    if errorlevel 1 (
        echo ERROR
        set /a failed_count=failed_count+1
    ) else (
        findstr /c:"Breakpoint" "%%~nI_debug.txt" >nul
        if errorlevel 1 (
            echo FAILED
            set /a failed_count=failed_count+1
        ) else (
	    echo SUCCESS
            set /a success_count=success_count+1
	                  
        )
    )
)

rem Get test results
set /a executed_count=success_count+failed_count
echo Executed %executed_count% tests, %success_count% success, %failed_count% failed

if %failed_count% == 0 (
    exit /b 0
) else (
    exit /b 1
)
