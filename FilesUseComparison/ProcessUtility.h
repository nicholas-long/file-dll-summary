#pragma once

#include <vector>
#include <utility>
#include <string>

DWORD GetProcessByName(const char* exeName);
std::vector<std::string> GetLoadedModules(HANDLE proc);
bool InjectDLL(std::string dllName, HANDLE hProcess);
bool UnloadRemoteDLL(std::string dllName, HANDLE proc);