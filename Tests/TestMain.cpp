#include <windows.h>
#include <iostream>
#include <string>
#include "../ExternalLogFile.h"

using namespace std;

//string InjectDLL_Name_Test(std::string dllName, HANDLE hProcess)
//{
//   char buf[MAX_PATH];
//   if (!GetModuleFileNameA(NULL, buf, MAX_PATH))
//      return false;
//
//   wstring fullPath;
//   {
//      string s(buf);
//      fullPath = wstring(s.begin(), s.end());
//   }
//   wstring dllNameW(dllName.begin(), dllName.end());
//
//   boost::filesystem::path dllPath = boost::filesystem::path(fullPath)
//      .parent_path()
//      .append(dllNameW);
//
//
//
//   return dllPath.string();
//}
//
//void Test_File_Absolute_Path(wstring lpFileName)
//{
//   boost::filesystem::path filePath = boost::filesystem::path(wstring(lpFileName));
//   if (!filePath.is_absolute())
//   {
//      boost::filesystem::path a = boost::filesystem::current_path().append(filePath.wstring());
//      cout << (a.string().c_str()) << endl;
//   }
//   else
//   {
//      cout << (filePath.string().c_str()) << endl;
//   }
//}

int main()
{
   string logFile = GetProcessLogFilePath(123);
   cout << logFile << endl;

   //cout << InjectDLL_Name_Test("test.dll", NULL);

   //Test_File_Absolute_Path(wstring(L"example.path.dat"));
   //Test_File_Absolute_Path(wstring(L"files\\example.path.dat"));

   HMODULE hmod = LoadLibraryA("InjectFileMonitor.dll");
   cout << "Loaded " << hmod << endl;

   if (!hmod)
      cout << "GetLastError returned " << GetLastError() << endl;

   cin.get();

   return 0;
}