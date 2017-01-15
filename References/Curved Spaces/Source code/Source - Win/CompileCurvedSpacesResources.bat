@echo off

::	----------------------------------------------------------------
::	Recompile language-independent resources.
::	----------------------------------------------------------------

::	Compile the icon to a .obj file for direct inclusion in the .exe file,
::	so Windows will use the icon to represent the program.
::	Other language-independent resources may be included here as well.

del "CurvedSpaces.obj"
"D:\ProgramFiles2\Go\GoRC\GoRC.exe" /o /fo "CurvedSpaces.obj" "CurvedSpaces.rc"

pause
