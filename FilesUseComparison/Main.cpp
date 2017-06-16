#include <string>
#include <exception>
#include <set>
#include <iostream>
#include <iterator>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <windows.h>

#include "ProcessUtility.h"
#include "Checksum.h"
#include "../ExternalLogFile.h"

using namespace std;

namespace po = boost::program_options;

typedef set<pair<string, string>> ChecksumSet;

const std::string DLL_NAME = "injectfilemonitor.dll";

template <class S>
void WriteChecksums(S& s, ChecksumSet& modules)
{
   for (auto m : modules)
   {
      s << m.first << " = " << m.second << endl;
   }
}

int main(int ac, char* av[])
{
   HANDLE proc = NULL;

   try {

      po::options_description desc("Allowed options");
      desc.add_options()
         ("help", "produce help message")
         ("name", po::value<string>(), "process executable name")
         ("pid", po::value<int>(), "process ID")
         ("file", po::value<string>(), "name of single file to checksum")
         ("watch", "option: analyze all file accesses")
         ("interactive", "option for watch: wait and complete watching on key press")
         ("path", "option: use full paths instead of file names")
         ;

      po::variables_map vm;
      po::store(po::parse_command_line(ac, av, desc), vm);
      po::notify(vm);

      if (vm.count("help"))
      {
         cout << desc << "\n";
         return 0;
      }

      bool watch = vm.count("watch") != 0;
      bool fullPaths = vm.count("path") != 0;
      bool interactiveWatch = vm.count("interactive") != 0;

      DWORD pid = 0;
      if (vm.count("pid"))
      {
         pid = (DWORD)vm["pid"].as<int>();
      }
      else if (vm.count("name"))
      {
         string name = vm["name"].as<string>();
         pid = GetProcessByName(name.c_str());
      }
      else if (vm.count("file"))
      {
         string file = vm["file"].as<string>();
         boost::filesystem::path p(file);
         ChecksumSet f;

         string displayPath = fullPaths ? file : p.filename().string();
         boost::to_lower(displayPath);

         auto checksum = FileChecksum(file);
         f.insert(make_pair(displayPath, checksum));
         WriteChecksums(cout, f);

         return 0;
      }
      else
      {
         cout << "either process name or process ID is required" << endl;
         return 1;
      }



      if (!pid) throw exception("Process not found");

      HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);

      if (!proc) // try again in case this is windows xp or server 2003 (need different access rights defined)
      {
         cerr << "Attempting to open process with XP/2003 style access rights" << endl;
         proc = OpenProcess((STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF), false, pid);
      }

      if (!proc)
         throw exception("Unable to open process handle");

      auto result = GetLoadedModules(proc);

      ChecksumSet loadedModules;

      for (auto fullPath : result)
      {
         boost::filesystem::path p(fullPath);
         string displayPath = fullPaths ? fullPath : p.filename().string();
         boost::to_lower(displayPath);

         auto checksum = FileChecksum(fullPath);

         loadedModules.insert(make_pair(displayPath, checksum));
      }

      WriteChecksums(cout, loadedModules);

      if (watch)
      {
         clog << "Beginning to watch process" << endl;

         // inject
         if (!InjectDLL(DLL_NAME, proc))
            throw exception("Failed to inject DLL");

         // wait for termination
         if (!interactiveWatch)
         {
            WaitForSingleObject(proc, INFINITE);
         }
         else
         {
            cin.get();
            UnloadRemoteDLL(DLL_NAME, proc);
         }

         set<string> fileSet;

         // read in log file file
         string logFilePath = GetProcessLogFilePath(pid);
         cerr << "Diagnostic: using log file " << logFilePath << endl;
         ifstream in(logFilePath);
         if (!in.is_open())
            throw exception("log file not opened");

         while (!in.eof())
         {
            const size_t BUF_SIZE = 2000;
            char buf[BUF_SIZE];
            in.getline(buf, BUF_SIZE);
            string line(buf);
            fileSet.insert(line);
         }

         in.close();
         //TODO: delete log file

         ChecksumSet accessedModules;

         for (auto fullPath : fileSet)
         {
            boost::filesystem::path p(fullPath);
            string displayPath = fullPaths ? fullPath : p.filename().string();
            boost::to_lower(displayPath);

            auto checksum = FileChecksum(fullPath);

            accessedModules.insert(make_pair(displayPath, checksum));
         }

         cout << endl << endl << "[Accessed]" << endl;
         WriteChecksums(cout, accessedModules);
      }


      CloseHandle(proc);
      proc = NULL;
   }
   catch (exception& e)
   {
      cerr << "error: " << e.what() << endl;
      cerr << "GetLastError returned " << GetLastError() << endl;
      if (proc)
         CloseHandle(proc);
      return 1;
   }
   catch (...)
   {
      cerr << "Exception of unknown type!" << endl;
   }

   return 0;
}