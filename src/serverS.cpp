#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <unordered_map>

#define SERVER_IP "127.0.0.1"
#define SERVERM_UPORT 44531
#define SERVERS_UPORT 41531

std::string readMessage(char mes[], int len){
    std::string result;
    for(int i = 1; i < len; i++){
        if(mes[i] == '|'){
            result.assign(mes + 1, i - 1);
            break;
        }
    }
    return result;
}

int main(int argc, char *argv[]){

    int serverS_sock;
    struct sockaddr_in serverS_addr;
    socklen_t addr_len;

    //create ucp socket
    serverS_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(serverS_sock < 0){
        perror("create ucp socket error, reason");
        exit(EXIT_FAILURE);
    }

    //initialize ucp address
    bzero(&serverS_addr, sizeof(serverS_addr));
    serverS_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serverS_addr.sin_addr);
    serverS_addr.sin_port = htons(SERVERS_UPORT);

    //bind ucp address
    if(bind(serverS_sock, (struct sockaddr*)&serverS_addr, sizeof(serverS_addr)) < 0){
        perror("bind ucp socket error, reason");
        close(serverS_sock);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server S is up and running using UDP on port " << ntohs(serverS_addr.sin_port) << std::endl;

    //read the data
    std::ifstream l_file("science.txt");
    if (!l_file.is_open()) {
        std::cerr << "fail to open file" << std::endl;
        std::cerr << "reason: " << strerror(errno) << std::endl;
        close(serverS_sock);
        exit(EXIT_FAILURE);
    }
    
    //store the data
    std::unordered_map<std::string, int> s_Map;
    std::string line;
    while (std::getline(l_file, line)) {
        size_t pos = line.find(",");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            int value = std::stoi(line.substr(pos + 2));
            s_Map.insert(std::pair<std::string, int>(key, value));
        }
    }

    //send all the book statuses to main server
    struct sockaddr_in serverM_addr;
    bzero(&serverM_addr, sizeof(serverM_addr));
    serverM_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serverM_addr.sin_addr);
    serverM_addr.sin_port = htons(SERVERM_UPORT);

    std::string message;
    for (auto it = s_Map.begin(); it != s_Map.end(); ++it) {
        if(it == s_Map.begin()){
            message += it->first;
        }else{
            message += ",";
            message += it->first;
        }     
    }
    message += "|";

    char w_buf[1000];
    strcpy(w_buf, message.c_str());
    sendto(serverS_sock, &w_buf, sizeof(w_buf), 0, (struct sockaddr*)&serverM_addr, sizeof(serverM_addr));

    //receive query book code
    bool searching = true;
    while(searching){
        char rf_buf[1024];
        int len;   
        len = recvfrom(serverS_sock, rf_buf, sizeof(rf_buf) - 1, 0, (struct sockaddr*)&serverM_addr, &addr_len);
        rf_buf[len] = '\0';
        std::string bookCode = readMessage(rf_buf, len);

        if(rf_buf[0] == '1'){
            std::cout << "Server S received an inventory status request for code " << bookCode << std::endl;
        }else{
            std::cout << "Server S received " << bookCode << " code from the Main Server." << std::endl;
        }
        
        //search the status of book
        int bookStatus = s_Map[bookCode];
        std::string st_message = std::to_string(bookStatus);
        st_message += "|";

        //reply to main server
        char st_buf[1000];
        strcpy(st_buf, st_message.c_str());
        sendto(serverS_sock, st_buf, sizeof(st_buf) - 1, 0, (struct sockaddr*)&serverM_addr, sizeof(serverM_addr));

        if(rf_buf[0] == '1'){
            std::cout << "Server S finished sending the inventory status to the Main server using UDP on port " << ntohs(serverS_addr.sin_port) << std::endl;
        }else{
            std::cout << "Server S finished sending the availability status of code " << bookCode << " to the Main Server using UDP on port " << ntohs(serverS_addr.sin_port) << std::endl;
        }
    }

    close(serverS_sock);
    return 0;
}

