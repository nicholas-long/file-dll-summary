// InjectFileMonitor.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include "LogFile.h"

using namespace std;

typedef HANDLE(APIENTRY *lpfnCreateFileW)(LPCTSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
//Loading DLLs
typedef HMODULE(APIENTRY *funcptrLoadLibraryW)(LPCWSTR);
typedef HMODULE(APIENTRY *funcptrLoadLibraryExW)(LPCWSTR, HANDLE, DWORD);
typedef HMODULE(APIENTRY *funcptrLoadLibraryA)(LPCSTR);
typedef HMODULE(APIENTRY *funcptrLoadLibraryExA)(LPCSTR, HANDLE, DWORD);

namespace original
{
   lpfnCreateFileW CreateFileW;
   BYTE* originalAddress;

   funcptrLoadLibraryW LoadLibraryW;
   funcptrLoadLibraryExW LoadLibraryExW;

   funcptrLoadLibraryA LoadLibraryA;
   funcptrLoadLibraryExA LoadLibraryExA;
}

namespace hooks
{
   vector<BYTE*> addedShortJumps;

   // hook for CreateFileW
   HANDLE WINAPI CreateFileHook(
      LPCTSTR               lpFileName,
      DWORD                 dwDesiredAccess,
      DWORD                 dwShareMode,
      LPSECURITY_ATTRIBUTES lpSecurityAttributes,
      DWORD                 dwCreationDisposition,
      DWORD                 dwFlagsAndAttributes,
      HANDLE                hTemplateFile
   )
   {
      string filename;
      {
         wstring w(lpFileName);
         filename = string(w.begin(), w.end());
      }

      boost::filesystem::path filePath = boost::filesystem::path(wstring(lpFileName));
      if (!filePath.is_absolute())
      {
         boost::filesystem::path a = boost::filesystem::current_path().append(filePath.wstring());
         LogFile::Log(a.string().c_str());
      }
      else
      {
         LogFile::Log(filePath.string().c_str());
      }

      return original::CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
   }

   //Hooking load and free library functions so we can track when DLLs get loaded and update list of modules

   void InformLoadModule(HMODULE hmod)
   {
      char buf[MAX_PATH];
      if (GetModuleFileNameA(hmod, buf, MAX_PATH))
         LogFile::Log(buf);
   }

   //HMODULE WINAPI LoadLibrary(
   //	_In_  LPCTSTR lpFileName
   //	);
   HMODULE APIENTRY LoadLibraryA(LPCSTR lpFileName)
   {
      HMODULE m = original::LoadLibraryA(lpFileName);

      InformLoadModule(m);

      return m;
   }

   //HMODULE WINAPI LoadLibrary(
   //	_In_  LPCTSTR lpFileName
   //	);
   HMODULE APIENTRY LoadLibraryW(LPCWSTR lpFileName)
   {
      HMODULE m = original::LoadLibraryW(lpFileName);
      InformLoadModule(m);
      return m;
   }


   //HMODULE WINAPI LoadLibraryEx(
   //_In_        LPCTSTR lpFileName,
   //_Reserved_  HANDLE hFile,
   //_In_        DWORD dwFlags
   //);
   HMODULE APIENTRY LoadLibraryExA(LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags)
   {
      HMODULE m = original::LoadLibraryExA(lpFileName, hFile, dwFlags);
      InformLoadModule(m);
      return m;
   }

   //HMODULE WINAPI LoadLibraryEx(
   //_In_        LPCTSTR lpFileName,
   //_Reserved_  HANDLE hFile,
   //_In_        DWORD dwFlags
   //);
   HMODULE APIENTRY LoadLibraryExW(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags)
   {
      HMODULE m = original::LoadLibraryExW(lpFileName, hFile, dwFlags);
      InformLoadModule(m);
      return m;
   }

}

//Utility functions for hacks
void ApiHook(BYTE* src, BYTE* dest, BYTE** originalAddress)
{
   DWORD dwback;
   VirtualProtect(src - 5, 7, PAGE_EXECUTE_READWRITE, &dwback);

   //cout << "Hooking address " << hex << (DWORD)src << dec << endl;
   //cout << "2 bytes: " << hex << *((unsigned short*)src) << dec << endl;

   if (*((unsigned short*)src) == 0xFF8B) // do windows API hook on "mov edi, edi"
   {
      DWORD jmpDiff = (dest - src);
      BYTE* longJump = src - 5;
      DWORD* jmpDiffAddy = (DWORD*)(longJump + 1);
      BYTE* shortJmp = src;

      //VirtualProtect(src - 5, 7, PAGE_EXECUTE_READWRITE, &dwback); // moved

      //set long jump before func
      *longJump = 0xe9;
      *jmpDiffAddy = jmpDiff;

      //set short jump in func inital nop
      *((unsigned short*)shortJmp) = 0xF9EB; // jmp -7

      //VirtualProtect(src - 5, 7, dwback, &dwback); // see if this breaks

      *originalAddress = (src + 2);
   }
   else if (*((unsigned short*)src) == 0x25FF) // there's a weird thunk
   {
      cout << "Found a thunk, hooking the target instead" << endl;
      // the thunk will be like jmp dword ptr [0x12345], so we just read the address of this function pointer and hook that instead
      BYTE*** address = (BYTE***)((DWORD)(src + 2));
      ApiHook(**address, dest, originalAddress);
   }
   else
   {
      cout << "PROBLEM INSERTING HOOK!" << endl;
   }
}

void InsertHooks()
{
   BYTE* reroutedOriginal;

   HMODULE kernelHmod = GetModuleHandleA("kernel32.dll");

   original::originalAddress = (BYTE*)GetProcAddress(kernelHmod, "LoadLibraryW");
   ApiHook((BYTE*)original::originalAddress, (BYTE*)hooks::LoadLibraryW, &reroutedOriginal);
   original::LoadLibraryW = (funcptrLoadLibraryW)(reroutedOriginal);
   hooks::addedShortJumps.push_back(original::originalAddress);

   original::originalAddress = (BYTE*)GetProcAddress(kernelHmod, "LoadLibraryExW");
   ApiHook((BYTE*)original::originalAddress, (BYTE*)hooks::LoadLibraryExW, &reroutedOriginal);
   original::LoadLibraryExW = (funcptrLoadLibraryExW)(reroutedOriginal);
   hooks::addedShortJumps.push_back(original::originalAddress);

   original::originalAddress = (BYTE*)GetProcAddress(kernelHmod, "LoadLibraryA");
   ApiHook((BYTE*)original::originalAddress, (BYTE*)hooks::LoadLibraryA, &reroutedOriginal);
   original::LoadLibraryA = (funcptrLoadLibraryA)(reroutedOriginal);
   hooks::addedShortJumps.push_back(original::originalAddress);

   original::originalAddress = (BYTE*)GetProcAddress(kernelHmod, "LoadLibraryExA");
   ApiHook((BYTE*)original::originalAddress, (BYTE*)hooks::LoadLibraryExA, &reroutedOriginal);
   original::LoadLibraryExA = (funcptrLoadLibraryExA)(reroutedOriginal);
   hooks::addedShortJumps.push_back(original::originalAddress);

   original::originalAddress = (BYTE*)GetProcAddress(kernelHmod, "CreateFileW");
   ApiHook((BYTE*)original::originalAddress, (BYTE*)hooks::CreateFileHook, &reroutedOriginal);
   original::CreateFileW = (lpfnCreateFileW)(reroutedOriginal);
   hooks::addedShortJumps.push_back(reroutedOriginal - 2); // there is a thunk rerouting CreateFileW
}

void RemoveHooks()
{
   for (BYTE* shortJmp : hooks::addedShortJumps)
   {
      *((unsigned short*)shortJmp) = 0xFF8B; // set the short jumps back to 2 byte nops
   }
}