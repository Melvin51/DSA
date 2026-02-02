The build.bat file is used to build the program as a x64 executable using the Microsofts MSVC compiler. Binaries built: .exe, .dll.
At runtime we can modify the source code used when compiling the .dll, recompile and relink it to the executable without having to stop the executable from running.
The .exe contains logic that checks if the .dll has been modified (i.e rebuilt). 
If .dll has been modified, .exe unmaps the current .dll and maps the new .dll that has the new logic, then jumps to the entry point of the .dll

It doesn't link with the standard library. Compiled as a windows PE x64 executable

https://github.com/user-attachments/assets/690f9c66-57ef-4835-a6d1-a6bf0222dfb5
