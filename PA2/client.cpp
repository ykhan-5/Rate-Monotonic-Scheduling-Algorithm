#include <unistd.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iomanip>
#include <vector>
#include <pthread.h>
#include <sstream>
#include <string>

// Sockets code provided by Proffesor Rincon

// I seperated the code into parts, throughout the client and server, you can follow
// my thought process on where the code goes. Hope you enjoy!

// allows me to send multiple pieces of data in one go
struct args
{
    std::string input;   // line of input
    std::string *output; // were output will be stored
    int iteration;       // iteration of the thread
    std::string address; // the server address
    int portNum;         // the portnumber

    args(std::string inp, std::string *outp, int itera, std::string addr, int portN)
    {
        input = inp;
        output = outp;
        iteration = itera;
        address = addr;
        portNum = portN;
    }
};

void *threading(void *ptr)
{

    args *sailBoat = (args *)ptr;

    std::string input;
    int iteration = sailBoat->iteration;

    // Socket Initialization
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // portnumber
    portno = sailBoat->portNum;

    // Error handling from prof.Rincon
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        std::cerr << "ERROR opening socket";
    server = gethostbyname(sailBoat->address.c_str());
    if (server == NULL)
    {
        std::cerr << "ERROR, no such host\n";
        exit(0);
    }

    // PART4 CONNECT AND SEND  ----------------> Server ------------------------

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    // Establish connection from prof.Rincon
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "ERROR connecting";
        exit(1);
    }

    std::string st;

    // buffer to send, "|" tells me where the seperation is (input | iteration Number)
    std::string buffer = sailBoat->input + "|" + std::to_string(iteration);
    input = buffer;
    int msgSize = sizeof(buffer);

    // send size and send data
    n = write(sockfd, &msgSize, sizeof(int));
    n = write(sockfd, buffer.c_str(), msgSize);

    // PART 10 TIME TO READ -------- Server <-----------------------------------

    buffer = "";

    // Read Size of message from server from prof.Rincon
    n = read(sockfd, &msgSize, sizeof(int));
    if (n < 0)
    {
        std::cerr << "ERROR reading from socket" << std::endl;
        exit(0);
    }
    char *tempBuffer = new char[msgSize + 1];
    bzero(tempBuffer, msgSize + 1);

    // Read message from socket
    n = read(sockfd, tempBuffer, msgSize);
    if (n < 0)
    {
        std::cerr << "ERROR reading from socket" << std::endl;
        exit(0);
    }

    buffer = tempBuffer;
    // delete tempBuffer to protect memory
    delete[] tempBuffer;

    std::string *output = sailBoat->output;
    *output = buffer;

    close(sockfd);
    return nullptr;
}

int main(int argc, char *argv[])
{
    int count = 0;

    // PART 1 Get Inputs and set up vars ----------------------------------------

    std::vector<std::string> inputs;

    std::string input;
    while (std::getline(std::cin, input))
    {
        if (!input.empty())
        {
            inputs.push_back(input);
            count++; // counts how many lines of input
        }
    }

    std::vector<std::string> out(count); // output strings so we can print at the end
    std::vector<pthread_t> tid(count);   // will hold "count" number of threads
    std::vector<args> sailBoat;          // holds the args for each input that we will send to server

    std::string address = argv[1]; // will hold our address
    int port = atoi(argv[2]);      // our port number

    // PART 2 Add to vector -----------------------------------------------------

    for (int i = 0; i < inputs.size(); i++)
    {
        sailBoat.emplace_back(inputs[i], &out[i], i + 1, address, port);
    }

    // PART 3 Send to threads ---------------------------------------------------

    for (int i = 0; i < inputs.size(); i++)
    {
        pthread_create(&tid[i], nullptr, threading, &sailBoat[i]);
    }

    // PART 11 Join Threads  ---------------------------------------------------

    for (int i = 0; i < inputs.size(); i++)
    {
        pthread_join(tid[i], nullptr);
    }

    // PART 12 And the result isss... ------------------------------------------

    for (int i = 0; i < out.size(); i++)
    {
        if (i != out.size() - 1)
        {
            std::cout << "CPU " << i + 1 << std::endl;
            std::cout << out.at(i) << std::endl;
        }
        else
        {
            std::cout << "CPU " << i + 1 << std::endl;
            std::cout << out.at(i);
        }
    }
    return 0;
}