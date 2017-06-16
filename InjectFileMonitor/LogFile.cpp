#include "LogFile.h"
#include "../ExternalLogFile.h"
#include <fstream>

using namespace std;

namespace LogFile
{
   ofstream log;

   void Initialize()
   {
      log.open(GetProcessLogFilePath(GetCurrentProcessId()), ios::out);
   }
   void Log(const char * filePath)
   {
      if (log.is_open())
      {
         log << filePath << endl;
      }
   }
   void Cleanup()
   {
      log.close();
   }
}