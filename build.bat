@echo off
del *.
set src_path=src\
set source=main
set main_dll=visualizer
set datetime=%date:~-4,4%%date:~-10,2%%date:~-7,2%%time:~3,2%%time:~3,2%%time:~6,2%

rem ----Old Notes----
rem When we try to compile, the dll has a handle open to it, we have to close that in order for CopyFile to work properly
rem NOTE: Figure out how to stop visual studio from locking the .pdb files if possible
rem	   Or see if pdb information can be stored directly in the executable(debug version)
rem	   same for dll and if possible, just store the pdb data in the binary files
rem	   Until storing the debug information directly in the bin files causes us issues

pushd ..\

rem Del removes some of the message written into std_out filestream(buffer)
del /Q *.*
del *.pdb > NUL 2> NUL

echo ----------------------------- PLATFORM CODE ---------------------------------------

cl /Zi /nologo /GS- %src_path%%source%.c /link /PDB:%source%_%datetime%.pdb /Entry:DSAMain /INCREMENTAL:NO /SUBSYSTEM:WINDOWS /NODEFAULTLIB kernel32.lib user32.lib gdi32.lib Ole32.lib

echo ----------------------------- PLATFORM INDEPENDENT CODE ---------------------------

cl /Zi /GS- /c  %src_path%%main_dll%.c
link /DLL /DEBUG /NODEFAULTLIB /INCREMENTAL:NO /PDB:%main_dll%_%datetime%.pdb /EXPORT:VisualizerMain /nologo %main_dll%.obj

popd