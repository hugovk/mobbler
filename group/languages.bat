languages.pl %1
pushd languages
mkdir unsigned
for /f %%a IN ('dir /b unsigned\*.pkg') do call makesis unsigned\%%a
ren unsigned\*.SIS *.sis
for /f %%a IN ('dir /b unsigned\*.sis') do call signsis unsigned\%%a %%ax ../mobbler.cer ../mobbler.key
REM ren *.SISX *.sisx
del /q unsigned\*.*
rmdir unsigned
popd
