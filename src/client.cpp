#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define SERVERM_TPORT 45531

std::string encrypteData(std::string mes){
    for(int i = 0; i < mes.size(); i++){
        int asciiValue = static_cast<int>(mes[i]);
        if(asciiValue <= 57 && asciiValue >= 48){
            asciiValue = (asciiValue - 48 + 5) % 10 + 48;
            mes[i] = static_cast<char>(asciiValue);
        }else if(asciiValue <= 90 && asciiValue >= 65){
            asciiValue = (asciiValue - 65 + 5) % 26 + 65;
            mes[i] = static_cast<char>(asciiValue);
        }else if(asciiValue <= 122 && asciiValue >= 97){
            asciiValue = (asciiValue - 97 + 5) % 26 + 97;
            mes[i] = static_cast<char>(asciiValue);
        }
    }
    return mes;
} 

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

int main(int argc, char *argv[])
{
    struct sockaddr_in serverM_addr;

    int client_sock;

    //create tcp socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(client_sock < 0){
        perror("create tcp socket error, reason");
        exit(EXIT_FAILURE);
    }

    //initialize tcp address
    bzero(&serverM_addr, sizeof(serverM_addr));
    serverM_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serverM_addr.sin_addr);
    serverM_addr.sin_port = htons(SERVERM_TPORT);

    //connect to server
    if(connect(client_sock, (struct sockaddr *)&serverM_addr, sizeof(serverM_addr))){
        std::cerr << "Connection failed" << strerror(errno) << std::endl;
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    std::cout << "Client is up and running." << std::endl;

    //get the local address information
    struct sockaddr_in local_addr;
    socklen_t addr_len;
    int gsn = getsockname(client_sock, (struct sockaddr*)&local_addr, &addr_len);
    if (gsn == -1) {
        perror("Getsockname failed");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    bool logging = true;
    bool ifAdmin = false;
    std::string username, password;
    while(logging){
        //input login message
        std::cout << "Please enter the username:";
        std::cin >> username;
        std::cout << "Please enter the password:";
        std::cin >> password;

        std::string e_username, e_password;
        //encrypte data
        e_username = encrypteData(username);
        e_password = encrypteData(password);
        std::string message = e_username + "," + e_password + "|"; 


        //login in server
        char w_buf[1000];
        strcpy(w_buf, message.c_str());
        if(send(client_sock, w_buf, sizeof(message), 0) == -1){
            std::cerr << "send error" << std::endl;
        }

        std::cout << username << " sent an authentication request to the Main Server." << std::endl;

        //receive response
        char r_buf[1024];
        int r_len;
        r_len = recv(client_sock, r_buf, sizeof(r_buf) - 1, 0);
        r_buf[r_len] = '\0';

        if(r_buf[0] == '2'){
            std::cout << username << " received the result of authentication from Main Server using TCP over port " << ntohs(local_addr.sin_port) << ". Authentication is successful." << std::endl;
            logging = false;
        }else if(r_buf[0] == '1'){
            std::cout << username << " received the result of authentication from Main Server using TCP over port " << ntohs(local_addr.sin_port) << ". Authentication failed: Password does not match." << std::endl;
        }else if(r_buf[0] == '3'){
            std::cout << username << " received the result of authentication from Main Server using TCP over port " << ntohs(local_addr.sin_port) << ". Authentication is successful." << std::endl;
            logging = false;
            ifAdmin = true;
        }else{
            std::cout << username << " received the result of authentication from Main Server using TCP over port " << ntohs(local_addr.sin_port) << ". Authentication failed: Username not found." << std::endl;
        }
    }

    bool searching = true;
    while(searching){
        //input the query code
        std::cout << "Please enter book code to query:";
        std::string queryCode;
        std::cin >> queryCode;
        std::string w_message = queryCode + "|";

        //send the query code
        char w_buf[1000];
        strcpy(w_buf, w_message.c_str());
        if(send(client_sock, w_buf, sizeof(w_buf), 0) == -1){
            std::cerr << "send error" << std::endl;
        }
        
        if(ifAdmin){
            std::cout << "Request sent to the Main Server with Admin rights." << std::endl;
        }else{
            std::cout << username << " sent the request to the Main Server." << std::endl;
        }
        

        //receive the response
        char r_buf[1024];
        int r_len;
        r_len = recv(client_sock, r_buf, sizeof(r_buf) - 1, 0);
        r_buf[r_len] = '\0';
    
        std::cout << "Response received from the Main Server on TCP port: " << ntohs(local_addr.sin_port) << std::endl;
        
        if(r_buf[0] == '0'){
            std::cout << "Not able to find the book-code " << queryCode << " in the system." << std::endl;
            std::cout << "—- Start a new query —-" << std::endl;
            continue;
        }

        //check the book status
        std::string bookStatus;
        bookStatus = readMessage(r_buf, r_len);
        if(ifAdmin){
            std::cout << "Total number of book " << queryCode << " available = " << bookStatus << std::endl;
        }else{
            if(bookStatus == "0"){
                std::cout << "The requested book " << queryCode << " is NOT available in the library." << std::endl;
            }else{
                std::cout << "The requested book " << queryCode << " is available in the library." << std::endl;
            }
        }
        std::cout << "—- Start a new query —-" << std::endl;
    }

    close(client_sock);
    return 0;
}