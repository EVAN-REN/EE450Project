#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include <vector>

#define SERVER_IP "127.0.0.1"
#define SERVERM_TPORT 45531
#define SERVERM_UPORT 44531

std::vector<std::string> readMessage(char mes[], int len){
    std::vector<std::string> result;
    int front = 0, back = 0;
    while(back < len){
        if(mes[back] == ','){
            std::string info(mes + front, back - front);
            result.push_back(info);
            back++;
            front = back;
        }else if(mes[back] == '|'){
            break;
        }else{
            back++;
        }
    }
    std::string info(mes + front, back - front);
    result.push_back(info);
    return result;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server_taddr, server_uaddr;
    socklen_t addr_len;
    int server_tsock, server_usock;

    //create tcp socket
    server_tsock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_tsock < 0){
        perror("create tcp socket error, reason");
        exit(EXIT_FAILURE);
    }

    //initialize tcp address
    bzero(&server_taddr, sizeof(server_taddr));
    server_taddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server_taddr.sin_addr);
    server_taddr.sin_port = htons(SERVERM_TPORT);

    //bind tcp address
    if(bind(server_tsock, (struct sockaddr*)&server_taddr, sizeof(server_taddr)) < 0){
        perror("bind tcp socket error, reason");
        close(server_tsock);
        exit(EXIT_FAILURE);
    }

    //create ucp socket
    server_usock = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_usock < 0){
        perror("create ucp socket error, reason");
        close(server_tsock);
        exit(EXIT_FAILURE);
    }

    //initialize ucp address
    bzero(&server_uaddr, sizeof(server_uaddr));
    server_uaddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server_uaddr.sin_addr);
    server_uaddr.sin_port = htons(SERVERM_UPORT);

    //bind ucp address
    if(bind(server_usock, (struct sockaddr*)&server_uaddr, sizeof(server_uaddr)) < 0){
        perror("bind ucp socket error, reason");
        close(server_tsock);
        close(server_usock);
        exit(EXIT_FAILURE);
    }

    //create a map to store the book list
    std::vector<std::string> bookList;

    //listen tcp socket
    listen(server_tsock, 64);

    std::cout << "Main Server is up and running." << std::endl;

 //Main Server received the book code list by udp
    struct sockaddr_in serverS_addr;
    socklen_t saddr_len = sizeof(serverS_addr);
    struct sockaddr_in serverL_addr;
    socklen_t laddr_len = sizeof(serverL_addr);
    struct sockaddr_in serverH_addr;
    socklen_t haddr_len = sizeof(serverH_addr);
    for(int i = 0; i < 3; i++){   
        //received from serverSLH
        char buf[1024];
        int len;

        if(i == 0){
            len = recvfrom(server_usock, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&serverS_addr, &saddr_len);
            buf[len] = '\0';

            //store the data in ServerM
            std::vector<std::string> bList;
            bList = readMessage(buf, len);
            bookList.insert(bookList.end(), bList.begin(), bList.end());

            std::cout << "Main Server received the book code list from server S using UDP over port " << ntohs(server_uaddr.sin_port) << std::endl;
        }else if(i == 1){
            len = recvfrom(server_usock, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&serverL_addr, &laddr_len);
            buf[len] = '\0';

            //store the data in ServerM
            std::vector<std::string> bList;
            bList = readMessage(buf, len);
            bookList.insert(bookList.end(), bList.begin(), bList.end());

            std::cout << "Main Server received the book code list from server L using UDP over port " << ntohs(server_uaddr.sin_port) << std::endl;
        }else{
            len = recvfrom(server_usock, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&serverH_addr, &haddr_len);
            buf[len] = '\0';

            //store the data in ServerM
            std::vector<std::string> bList;
            bList = readMessage(buf, len);
            bookList.insert(bookList.end(), bList.begin(), bList.end());

            std::cout << "Main Server received the book code list from server H using UDP over port " << ntohs(server_uaddr.sin_port) << std::endl;
        }  
    }
    
    //loading the member data
    std::ifstream m_file("member.txt");
    if (!m_file.is_open()) {
        std::cerr << "fail to open file" << std::endl;
        std::cerr << "reason: " << strerror(errno) << std::endl;
        close(server_tsock);
        close(server_usock);
        exit(EXIT_FAILURE);
    }

    //store the member data
    std::unordered_map<std::string, std::string> memberMap;
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
            memberMap.insert(std::pair<std::string, std::string>(key, value));  
        }
    }
    std::cout << "Main Server loaded the member list." << std::endl;

    //connect to client
    struct sockaddr_in client_addr;
    int client_sock;
    socklen_t clientaddr_len;
    client_sock = accept(server_tsock, (struct sockaddr *)&client_addr, &clientaddr_len);
    if(client_sock < 0){
        perror("client accept failed!");
    }

    bool logging = true;
    bool ifAdmin = false;
    while(logging){
        //receive the user's information from client
        char r_buf[1024];
        int r_len;
        r_len = recv(client_sock, r_buf, sizeof(r_buf) - 1, 0);
        r_buf[r_len] = '\0';
        
        std::cout << "Main Server received the username and password from the client using TCP over port " << ntohs(server_taddr.sin_port) << std::endl;

        std::string username;
        std::string password;
        std::vector<std::string> r_message = readMessage(r_buf, r_len);
        username = r_message[0];
        password = r_message[1];

        //deal with the user message
        std::unordered_map<std::string, std::string>::iterator it = memberMap.find(username);
        if(it == memberMap.end()){
            std::cout << username << " is not registered. Send a reply to the client." << std::endl;
            char w_buf[1000] = "0";
            send(client_sock, w_buf, sizeof(w_buf), 0);
        }else{
            if(it->second == password){
                std::cout << "Password " << password << " matches the username. Send a reply to the client." << std::endl;
                char w_buf[1000];
                if(username == "Firns"){
                    w_buf[0] = '3';
                    ifAdmin = true;
                }else{
                    w_buf[0] = '2';
                }
                send(client_sock, w_buf, sizeof(w_buf), 0);
                logging = false;
            }else{
                std::cout << "Password " << password << " does not matches the username. Send a reply to the client." << std::endl;
                char w_buf[1000] = "1";
                send(client_sock, w_buf, sizeof(w_buf), 0);
            }
        }
    }

    bool searching = true;
    while(searching){
        //receive the book code
        char r_buf[1024];
        int r_len;
        r_len = recv(client_sock, r_buf, sizeof(r_buf) - 1, 0);
        r_buf[r_len] = '\0';

        std::string bookCode;
        std::vector<std::string> r_message = readMessage(r_buf, r_len);
        bookCode = r_message[0]; 

        std::cout << "Main Server received the book request from client using TCP over port " << ntohs(server_taddr.sin_port) << std::endl;

        //find book code from main server
        std::string bookStatus;
        auto it = std::find(bookList.begin(), bookList.end(), bookCode);
        if (it == bookList.end()) {
            std::cout << "Did not find " << bookCode << " in the book code list." << std::endl;
            char w_buf[1000] = "0";
            send(client_sock, w_buf, sizeof(w_buf), 0);
            continue;
        }

        //find book status from backend servers
        char st_buf[1000];
        std::string st_message;
        if(ifAdmin){
            st_message = "1" + bookCode + "|";
        }else{
            st_message = "0" + bookCode + "|";
        }
        
        strcpy(st_buf, st_message.c_str());

        if(bookCode[0] == 'S'){
            sendto(server_usock, st_buf, strlen(st_buf), 0, (struct sockaddr*)&serverS_addr, sizeof(serverS_addr));
            std::cout << "Found " << bookCode << " located at Server S. Send to Server S." << std::endl;

            //receive book status from backend servers
            char rf_buf[1024];
            int rf_len;
            rf_len = recvfrom(server_usock, rf_buf, sizeof(rf_buf) - 1, 0, (struct sockaddr*)&serverS_addr, &addr_len);
            rf_buf[rf_len] = '\0';
            bookStatus = readMessage(rf_buf,rf_len)[0];

            std::cout << "Main Server received from server S the book status result using UDP over port " << ntohs(server_uaddr.sin_port) << ":" << std::endl;
        }else if (bookCode[0] == 'L'){
            sendto(server_usock, st_buf, strlen(st_buf), 0, (struct sockaddr*)&serverL_addr, sizeof(serverL_addr));
            std::cout << "Found " << bookCode << " located at Server L. Send to Server L." << std::endl;

            //receive book status from backend servers
            char rf_buf[1024];
            int rf_len;
            rf_len = recvfrom(server_usock, rf_buf, sizeof(rf_buf) - 1, 0, (struct sockaddr*)&serverL_addr, &addr_len);
            rf_buf[rf_len] = '\0';
            bookStatus = readMessage(rf_buf,rf_len)[0];

            std::cout << "Main Server received from server L the book status result using UDP over port " << ntohs(server_uaddr.sin_port) << ":" << std::endl;
        }else{
            sendto(server_usock, st_buf, strlen(st_buf), 0, (struct sockaddr*)&serverH_addr, sizeof(serverH_addr));
            std::cout << "Found " << bookCode << " located at Server H. Send to Server H." << std::endl;

            //receive book status from backend servers
            char rf_buf[1024];
            int rf_len;
            rf_len = recvfrom(server_usock, rf_buf, sizeof(rf_buf) - 1, 0, (struct sockaddr*)&serverH_addr, &addr_len);
            rf_buf[rf_len] = '\0';
            bookStatus = readMessage(rf_buf,rf_len)[0];

            std::cout << "Main Server received from server H the book status result using UDP over port " << ntohs(server_uaddr.sin_port) << ":" << std::endl;
        }
        
        std::cout << "Number of books " << bookCode << " available is: " << bookStatus << std::endl;
        
        //send book status to client
        char w_buf[1000];
        std::string w_message;
        w_message = "1" + bookStatus + "|";
        strcpy(w_buf, w_message.c_str());
        send(client_sock, w_buf, sizeof(w_buf), 0);

        std::cout << "Main Server sent the book status to the client." << std::endl;
    }

    close(server_usock);
    close(server_tsock);
    return 0;
}
