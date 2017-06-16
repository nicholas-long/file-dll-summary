# file-dll-summary
Enumerate and checksum all DLLs and files used by a process

The purpose of this project is to create reports of DLLs, files, and checksums.  These reports can be collected from multiple environments and compared using a merging tool to see what DLLs or files are different between environments without comparing entire directories across a network.  Doing so will also limit the scope of files compared to only DLLs and files that are relevant to a particular execution or behavior within the target process.  The DLLs and files are sorted to make this comparison easier within your merging tool.

## Summary

There are two stages to getting a summary of a process.
1. Enumerating the modules
2. Watching the process (optional)

First, the process is opened and all modules are enumerated.  The corresponding files on disk are checksummed.  A report is prepared including the file names.
Next, if you choose to watch the process, a DLL will be injected into it.  It hooks API functions to log new DLLs loaded or files accessed.  You can finish watching the process when it ends or end early using the `--interactive` option and sending a newline to the console.

Reports are sent to standard output and should be written to a file using command line operators. 

## Arguments

`--help`
Get help.

`--pid (process id)`
Open up a process by ID and get a summary of all the modules.

`--name (process name)`
Look up a running process by name instead of process ID.

`--file (file name)`
Do a quick checksum of a single file and print the result.

`--watch`
Begin watching the process for additional DLL loads or file accesses.  Terminates when the process is complete, or when you send a newline if the `--interactive` option is used.

`--path`
Print full filesystem paths in the report.  If this option is not provided, then all paths are limited to just file name.

`--interactive`
Finish watching the process as soon as you press enter.

## Examples

`FilesUseComparison --name process.exe --watch > output.txt`

`FilesUseComparison --pid 1234`

## Technical details

Works on Windows x86 processes only.  I think.  There's a branch for supporting XP and Windows Server 2003.

## Dependencies

* IDE: Visual Studio 2015
* Platform SDK
* boost
