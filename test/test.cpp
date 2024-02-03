#include <iostream>
#include <string>
#include <fstream>


int main(){
    std::ifstream m_file("../src/member.txt");
    std::string line;
    while (std::getline(m_file, line)) {
        size_t pos = line.find(",");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value;
            if(static_cast<int>(line.at(line.size() - 1)) != 13){
                value = line.substr(pos + 2);
            }else{
                value = line.substr(pos + 2, line.size() - 3 - pos);
            }      
            std::cout << value.size() << std::endl;
        }
    }
    return 0;
}