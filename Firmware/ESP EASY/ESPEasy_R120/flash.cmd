@echo off
set /p comport= Comport (example 3, 4, ..)           :
set /p fsize= Flash Size (example 512, 1024, 4096) :
set /p build= Build (example 71, 72, ..)           :

echo Using com port: %comport%
echo Using bin file: ESPEasy_R%build%_%fsize%.bin

esptool.exe -vv -cd nodemcu -cb 115200 -cp COM%comport% -ca 0x00000 -cf ESPEasy_R%build%_%fsize%.bin

pause
