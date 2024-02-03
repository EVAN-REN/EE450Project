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
#define SERVERH_UPORT 43531

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

    int serverH_sock;
    struct sockaddr_in serverH_addr;
    socklen_t addr_len;

    //create ucp socket
    serverH_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(serverH_sock < 0){
        perror("create ucp socket error, reason");
        exit(EXIT_FAILURE);
    }

    //initialize ucp address
    bzero(&serverH_addr, sizeof(serverH_addr));
    serverH_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serverH_addr.sin_addr);
    serverH_addr.sin_port = htons(SERVERH_UPORT);

    //bind ucp address
    if(bind(serverH_sock, (struct sockaddr*)&serverH_addr, sizeof(serverH_addr)) < 0){
        perror("bind ucp socket error, reason");
        close(serverH_sock);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server H is up and running using UDP on port " << ntohs(serverH_addr.sin_port) << std::endl;

    //read the data
    std::ifstream l_file("history.txt");
    if (!l_file.is_open()) {
        std::cerr << "fail to open file" << std::endl;
        close(serverH_sock);
        exit(EXIT_FAILURE);
    }
    
    //store the data
    std::unordered_map<std::string, int> h_Map;
    std::string line;
    while (std::getline(l_file, line)) {
        size_t pos = line.find(",");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            int value = std::stoi(line.substr(pos + 2));
            h_Map.insert(std::pair<std::string, int>(key, value));
        }
    }

    //send all the book statuses to main server
    struct sockaddr_in serverM_addr;
    bzero(&serverM_addr, sizeof(serverM_addr));
    serverM_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serverM_addr.sin_addr);
    serverM_addr.sin_port = htons(SERVERM_UPORT);

    std::string message;
    for (auto it = h_Map.begin(); it != h_Map.end(); ++it) {
        if(it == h_Map.begin()){
            message += it->first;
        }else{
            message += ",";
            message += it->first;
        }     
    }
    message += "|";

    char w_buf[1000];
    strcpy(w_buf, message.c_str());
    sendto(serverH_sock, &w_buf, sizeof(w_buf), 0, (struct sockaddr*)&serverM_addr, sizeof(serverM_addr));

    //receive query book code
    bool searching = true;
    while(searching){
        char rf_buf[1024];
        int len;   
        len = recvfrom(serverH_sock, rf_buf, sizeof(rf_buf) - 1, 0, (struct sockaddr*)&serverM_addr, &addr_len);
        rf_buf[len] = '\0';
        std::string bookCode = readMessage(rf_buf, len);

        if(rf_buf[0] == '1'){
            std::cout << "Server H received an inventory status request for code " << bookCode << std::endl;
        }else{
            std::cout << "Server H received " << bookCode << " code from the Main Server." << std::endl;
        }
        
        //search the status of book
        int bookStatus = h_Map[bookCode];
        std::string st_message = std::to_string(bookStatus);
        st_message += "|";

        //reply to main server
        char st_buf[1000];
        strcpy(st_buf, st_message.c_str());
        sendto(serverH_sock, st_buf, sizeof(st_buf) - 1, 0, (struct sockaddr*)&serverM_addr, sizeof(serverM_addr));

        if(rf_buf[0] == '1'){
            std::cout << "Server H finished sending the inventory status to the Main server using UDP on port " << ntohs(serverH_addr.sin_port) << std::endl;
        }else{
            std::cout << "Server H finished sending the availability status of code " << bookCode << " to the Main Server using UDP on port " << ntohs(serverH_addr.sin_port) << std::endl;
        }
    }

    close(serverH_sock);
    return 0;
}

