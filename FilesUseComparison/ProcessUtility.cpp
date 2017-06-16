
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <string>
#include <exception>
#include <boost/filesystem.hpp>
#include "ProcessUtility.h"
#include "Checksum.h"

#pragma comment(lib, "psapi.lib")

using namespace std;


DWORD GetProcessByName(const char* exeName)
{
   string name(exeName);

   DWORD pid = 0;

   // Create toolhelp snapshot.
   HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   PROCESSENTRY32 process;
   ZeroMemory(&process, sizeof(process));
   process.dwSize = sizeof(process);

   // Walkthrough all processes.
   if (Process32First(snapshot, &process))
   {
      do
      {
         // Compare process.szExeFile based on format of name, i.e., trim file path
         // trim .exe if necessary, etc.
         wstring w(process.szExeFile);
         string proc(w.begin(), w.end());

         if (_stricmp(proc.c_str(), name.c_str()) == 0)
         {
            pid = process.th32ProcessID;
            break;
         }
      } while (Process32Next(snapshot, &process));
   }

   CloseHandle(snapshot);

   if (pid != 0)
   {
      return pid;
   }

   // Not found

   return NULL;
}

std::vector<std::string> GetLoadedModules(HANDLE proc)
{
   vector<string> result;

   const int MAX_MODULES = 9000;
   HMODULE modules[MAX_MODULES];
   ZeroMemory(modules, sizeof(HMODULE) * MAX_MODULES);
   DWORD needed;
   EnumProcessModules(proc, modules, MAX_MODULES, &needed);
   needed /= sizeof(HMODULE);
   result.reserve((size_t)needed);

   for (DWORD m = 0; m < needed; m++)
   {
      HMODULE hmod = modules[m];
      char moduleNameBuf[MAX_PATH];
      if (GetModuleFileNameExA(proc, hmod, moduleNameBuf, MAX_PATH) != 0)
      {
         string name(moduleNameBuf);
         result.push_back(name);
      }
   }

   return result;
}

bool InjectDLL(std::string dllName, HANDLE hProcess)
{
   char buf[MAX_PATH];
   if (!GetModuleFileNameA(NULL, buf, MAX_PATH))
      return false;

   string fullPath;
   {
      wstring wstrFullPath;
      {
         string s(buf);
         wstrFullPath = wstring(s.begin(), s.end());
      }
      wstring dllNameW(dllName.begin(), dllName.end());

      boost::filesystem::path dllPath = boost::filesystem::path(wstrFullPath)
         .parent_path()
         .append(dllNameW);
      fullPath = dllPath.string();
   }

   size_t strLen = fullPath.length() + 1;

   // thanks http://stackoverflow.com/questions/10930353/injecting-c-dll/10981735
   LPVOID LoadLibAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
   LPVOID dereercomp = VirtualAllocEx(hProcess, NULL, strLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
   if (!dereercomp) throw exception("Injecting DLL: Could not allocate remote memory");
   if (!WriteProcessMemory(hProcess, dereercomp, fullPath.c_str(), strLen, NULL))
      throw exception("Injecting DLL: Could not write to remote memory");
   HANDLE asdc = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibAddr, dereercomp, 0, NULL);
   if (!asdc) throw exception("Injecting DLL: Could not create remote thread");
   WaitForSingleObject(asdc, INFINITE);
   VirtualFreeEx(hProcess, dereercomp, strLen, MEM_RELEASE);
   CloseHandle(asdc);
   return true;
}

bool UnloadRemoteDLL(std::string dllName, HANDLE proc)
{
   const int MAX_MODULES = 9000;
   HMODULE modules[MAX_MODULES];
   ZeroMemory(modules, sizeof(HMODULE) * MAX_MODULES);
   DWORD needed;
   EnumProcessModules(proc, modules, MAX_MODULES, &needed);
   needed /= sizeof(HMODULE);

   HMODULE found = NULL;

   for (DWORD m = 0; m < needed; m++)
   {
      HMODULE hmod = modules[m];
      char moduleNameBuf[MAX_PATH];
      if (GetModuleBaseNameA(proc, hmod, moduleNameBuf, MAX_PATH) != 0)
      {
         string name(moduleNameBuf);
         if (_stricmp(dllName.c_str(), name.c_str()) == 0)
         {
            found = hmod;
            break;
         }
      }
   }

   if (found)
   {
      LPVOID freeLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary"); // find the address of freelibrary in this process.  assume it's the same in the other.
      HANDLE asdc = CreateRemoteThread(proc, NULL, NULL, (LPTHREAD_START_ROUTINE)freeLibraryAddr, found, 0, NULL); // start a thread there and pass in the HMODULE of our DLL to unload
      WaitForSingleObject(asdc, INFINITE);
      CloseHandle(asdc);
      return true;
   }
   else
      return false;
}