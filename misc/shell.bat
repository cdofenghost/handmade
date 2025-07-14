@echo off
subst a: c:\handmade
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set path=a:\handmade\misc\;%path%
a:
cd a:\handmade