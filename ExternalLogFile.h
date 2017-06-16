#pragma once

#include <string>
#include <boost/filesystem.hpp>
#include <sstream>
#include <Windows.h>

std::string GetProcessLogFilePath(DWORD processId)
{
   std::stringstream ss;
   ss << "ProcessFileLog_" << processId << ".log";
   boost::filesystem::path p = boost::filesystem::temp_directory_path().append(ss.str());
   return p.string();
}