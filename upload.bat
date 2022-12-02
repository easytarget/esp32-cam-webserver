set hour=%time:~0,2%
if "%hour:~0,1%" == " " set hour=0%hour:~1,1%
echo hour=%hour%
set min=%time:~3,2%
if "%min:~0,1%" == " " set min=0%min:~1,1%
echo min=%min%
set secs=%time:~6,2%
if "%secs:~0,1%" == " " set secs=0%secs:~1,1%
echo secs=%secs%
set year=%date:~-4%
echo year=%year%
set month=%date:~3,2%
if "%month:~0,1%" == " " set month=0%month:~1,1%
echo month=%month%
set day=%date:~0,2%
if "%day:~0,1%" == " " set day=0%day:~1,1%
echo day=%day%
set datetimef=%year%%month%%day%_%hour%%min%%secs%

rem cd build\esp32.esp32.esp32\
set fld=\\beelink\web\espfw\esp32-cam\
set dst=esp32_cam
set src=esp32-cam-webserver.ino.esp32.bin

set f=%fld%%dst%_last.bin
del %f%
copy %src% %f%

set f=%fld%%dst%_%datetimef%%1.bin
move %src% %f%