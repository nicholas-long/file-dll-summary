#include <boost/crc.hpp>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

const size_t BUFFER_SIZE = 4096;

//https://codereview.stackexchange.com/questions/133483/calculate-the-crc32-of-the-contents-of-a-file-using-boost
std::size_t fileSize(std::ifstream& file)
{
   std::streampos current = file.tellg();
   file.seekg(0, std::ios::end);
   std::size_t result = (std::size_t)file.tellg();
   file.seekg(current, std::ios::beg);
   return result;
}

string FileChecksum(std::string filePath)
{
   boost::crc_32_type crc; //TODO: use a bigger checksum than CRC to avoid 1:4.2 billion collions

   char buffer[BUFFER_SIZE];
   
   ifstream in(filePath, ios::in | ios::binary);
   if (!in.is_open())
      return "FAILED TO OPEN";

   size_t size = fileSize(in);

   while (!in.eof())
   {
      in.read(buffer, BUFFER_SIZE);
      size_t count = (size_t)in.gcount();
      crc.process_bytes(buffer, count);
   }
   //while (in.read(buffer, BUFFER_SIZE))

   stringstream ss;
   ss << std::dec << size << "_" << std::hex << crc.checksum();
   return ss.str();
}