#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <vector>



std::string getCurrentTime()
{
   auto current_time = std::chrono::system_clock::now();
   std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time);
   std::string current_time_string = std::ctime(&current_time_t);
   return current_time_string;
}


std::vector<std::string> getFileContent(std::string file_name)
{
   std::vector<std::string> content;
   std::string myText;
   std::ifstream read_file(file_name);

   while(getline(read_file, myText))
   {
      content.push_back(myText);
   }
   read_file.close();

   return content;
}


void writeToFile(std::string file_name)
{
   std::vector<std::string> content = getFileContent(file_name);
   std::ofstream write_file(file_name);

   for(std::string i : content)
   {
      write_file << i << std::endl;
   }
   std::string current_time = getCurrentTime();
   write_file << current_time << std::endl;

   write_file.close();
}


int main() 
{
   writeToFile("times.txt");
   return 0;
}