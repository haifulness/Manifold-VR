@echo off

::	----------------------------------------------------------------
::	Copy languages
::	----------------------------------------------------------------

rmdir "..\Curved Spaces - Win\Languages" /s /q
mkdir "..\Curved Spaces - Win\Languages"
xcopy "..\Source-Common\Assets\Languages" "..\Curved Spaces - Win\Languages" /E /Y

::	----------------------------------------------------------------
::	Copy shaders
::	----------------------------------------------------------------

rmdir "..\Curved Spaces - Win\Shaders" /s /q
mkdir "..\Curved Spaces - Win\Shaders"
xcopy "..\Source-Common\Assets\Shaders" "..\Curved Spaces - Win\Shaders" /E /Y

::	----------------------------------------------------------------
::	Copy textures
::	----------------------------------------------------------------

rmdir "..\Curved Spaces - Win\Textures" /s /q
mkdir "..\Curved Spaces - Win\Textures"
xcopy "..\Source-Common\Assets\Textures" "..\Curved Spaces - Win\Textures" /E /Y

::	----------------------------------------------------------------
::	Copy sample spaces
::	----------------------------------------------------------------

rmdir "..\Curved Spaces - Win\Sample Spaces" /s /q
mkdir "..\Curved Spaces - Win\Sample Spaces"
xcopy "..\Source-Common\Assets\Sample Spaces" "..\Curved Spaces - Win\Sample Spaces" /E /Y

::	----------------------------------------------------------------
::	Copy help pages
::	----------------------------------------------------------------

rmdir "..\Curved Spaces - Win\Help" /s /q
mkdir "..\Curved Spaces - Win\Help"
xcopy "..\Source-Common\Assets\Help" "..\Curved Spaces - Win\Help" /E /Y

::	----------------------------------------------------------------
::	Copy thanks pages
::	----------------------------------------------------------------

rmdir "..\Curved Spaces - Win\Thanks" /s /q
mkdir "..\Curved Spaces - Win\Thanks"
xcopy "..\Source-Common\Assets\Thanks" "..\Curved Spaces - Win\Thanks" /E /Y

pause
