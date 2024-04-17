#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <string>
#include <tuple>
#include <iomanip>

// Sockets and fireman code provided by Professor Rincon

struct node
{
    std::string name; // task name
    int wceTime;      // task worst case execution time
    int period;       // task period
    int execLeft;     // executions this task has left in the period

    node(std::string n, int w, int p, int e) : name(n), wceTime(w), period(p), execLeft(e) {}

    // this helps us decide what priority will be in our pQueue
    bool operator<(const node &other) const
    {
        if (execLeft == 0 && other.execLeft == 0)
            return period > other.period;

        if (execLeft == 0)
            return execLeft < other.execLeft;

        if (other.execLeft == 0)
            return execLeft < other.execLeft;

        if (period == other.period)
            return name > other.name;

        return period > other.period;
    }
};

int gcd(int a, int b) // helper for lcm
{
    while (b)
    {
        a %= b;
        std::swap(a, b);
    }
    return a;
}

int lcm(int a, int b) // lcm using gcd
{
    int temp = gcd(a, b);

    return temp ? (a / temp * b) : 0;
}

// used to calculate hyperPeriod
int calculateHyperPeriod(const std::priority_queue<node> &tasks)
{
    if (tasks.empty())
    {
        return 0;
    }

    int hyperPeriod = tasks.top().period;

    std::priority_queue<node> temp = tasks;
    while (!temp.empty())
    {
        node current = temp.top();
        temp.pop();
        hyperPeriod = lcm(hyperPeriod, current.period);
    }

    return hyperPeriod;
}

// this calculates the expression where "Task set schedulability is unknown"
double calculateExpression(int n)
{
    return n * (std::pow(2.0, 1.0 / n) - 1);
}

// this function takes the string I create throughout the program and outputs it formatted.
std::string convertToTaskSchedule(const std::string &input)
{
    std::ostringstream resultStream;
    char currentChar = '\0';
    int currentCount = 0;

    for (size_t i = 0; i < input.size(); ++i)
    {
        char c = input[i];
        if (c == currentChar)
        {
            currentCount++;
        }
        else
        {
            if (currentCount > 0)
            {
                if (currentChar == 'I')
                {
                    resultStream << "Idle(" << currentCount << "), ";
                }
                else
                {
                    resultStream << currentChar << "(" << currentCount << "), ";
                }
            }
            currentChar = c;
            currentCount = 1;
        }
    }

    // Add the last character count
    if (currentCount > 0)
    {
        if (currentChar == 'I')
        {
            resultStream << "Idle(" << currentCount << ")";
        }
        else
        {
            resultStream << currentChar << "(" << currentCount << ")";
        }
    }

    return resultStream.str();
}

std::string RMSA(std::string in) // RMSA --> Rate Monotonic Scheduling Algorithm
{

    // PART 9 Calculate and put results into Boat  -----------------------------

    size_t pos = in.find('|');

    std::string input = in.substr(0, pos);
    std::string iter = in.substr(pos + 1);

    // getting input and initializing structures
    std::stringstream Boat(input);
    std::priority_queue<node> pq;
    std::vector<node> Ttasks;
    std::istringstream iss(input);

    // initializing variables
    std::string name;
    std::string output = "";
    int wceTime, period;
    int numTasks = 0;
    double util = 0;

    // pushing into two queues, one to help me print, one to be priority
    while (iss >> name >> wceTime >> period)
    {
        pq.push(node(name, wceTime, period, wceTime));
        Ttasks.push_back(node(name, wceTime, period, wceTime));
    }

    // printing CPU #
    int hyperPeriod = calculateHyperPeriod(pq);
    // std::cout << "CPU " << threadCount << "\n";
    Boat << "Task scheduling information: ";

    // this for-loop gets the utilization number, as well as line one printing
    for (std::vector<node>::const_iterator it = Ttasks.begin(); it != Ttasks.end(); ++it)
    {
        const node &task = *it;
        numTasks++;
        util = util + (static_cast<double>(task.wceTime) / static_cast<double>(task.period));
        Boat << task.name + " (WCET: " + std::to_string(task.wceTime) + ", Period: " + std::to_string(task.period);
        if (numTasks < Ttasks.size())
        {
            Boat << "), "; // Print comma if it's not the last element
        }
        else
        {
            Boat << ") "; // If it's the last element, don't print comma
        }
    }

    // more printing
    std::stringstream utilStream;
    utilStream << std::fixed << std::setprecision(2) << util;
    Boat << "\nTask set utilization: " + utilStream.str();

    Boat << "\nHyperperiod: " + std::to_string(hyperPeriod) + "\n";
    Boat << "Rate Monotonic Algorithm execution for CPU" << iter << ":\n";

    // logic based on utilization and formula given in directions
    if (util > 1)
    {
        Boat << "The task set is not schedulable\n";
    }
    else if (util > calculateExpression(numTasks))
    {
        Boat << "Task set schedulability is unknown\n";
    }
    else // find the scheduling diagram
    {
        Boat << "Scheduling Diagram for CPU " << iter << ": "; // <------------ HERE IS ISSUE, how to send iteration?
        for (int i = 1; i <= hyperPeriod; i++)
        {
            node current = pq.top();

            // if there are still tasks to execute...
            if (current.execLeft > 0)
            {
                pq.pop();

                // adding to my output string which will later be formatted
                output += current.name;

                current.execLeft--;
                pq.push(current);

                std::priority_queue<node> temp;

                // checking if any of the nodes have crossed thier period, in which case...
                while (!pq.empty())
                {
                    node current = pq.top();
                    pq.pop();

                    if (i % current.period == 0 && i != 1)
                    {

                        current.execLeft += current.wceTime; // i need to add executions!
                    }

                    temp.push(current);
                }

                pq = temp;
            }
            else
            {

                output.append("I"); // will be formatted correctly later

                std::priority_queue<node> temp;

                // checking if any of the nodes have crossed thier period, in which case...
                while (!pq.empty())
                {
                    node current = pq.top();
                    pq.pop();

                    if ((i % current.period == 0 && i != 1))
                    {
                        current.execLeft += current.wceTime; // i need to add executions!
                    }

                    temp.push(current);
                }

                pq = temp;
            }
        }
    }
    Boat << convertToTaskSchedule(output);
    Boat << "\n\n";

    return Boat.str();
}

void fireman(int)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n, msgSize = 0;

    // Call our fireman function
    signal(SIGCHLD, fireman);

    if (argc < 2)
    {
        std::cerr << "ERROR, no port provided\n";
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "ERROR opening socket";
        exit(1);
    }

    // PART 5 Listen for info  --------------------------------------------------

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "ERROR on binding";
        exit(1);
    }
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    while (true)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);

        // PART 6 Make child processes  ---------------------------------------------

        if (fork() == 0)
        {
            if (newsockfd < 0)
            {
                std::cerr << "ERROR on accept";
                exit(1);
            }

            msgSize = 0;
            n = read(newsockfd, &msgSize, sizeof(int));
            if (n < 0)
            {
                std::cerr << "ERROR reading from socket";
                exit(1);
            }

            char *tempBuffer = new char[msgSize + 1];

            // initialize array as all 0s <avoid residual data>
            bzero(tempBuffer, msgSize + 1);

            // PART 7 Read From Socket  ------------------------------------------------

            n = read(newsockfd, tempBuffer, msgSize + 1);
            if (n < 0)
            {
                std::cerr << "Error reading from socket" << std::endl;
                exit(0);
            }

            // Buffer = Client Message
            std::string buffer = tempBuffer;
            delete[] tempBuffer;

            // PART 8 Calculate Problem  -----------------------------------------------

            std::string st = RMSA(buffer);
            msgSize = st.size();

            // PART 9 Send to Client through socket  -----> Client ---------------------

            n = write(newsockfd, &msgSize, sizeof(int));
            if (n < 0)
            {
                std::cerr << "ERROR writing to socket";
                exit(1);
            }

            n = write(newsockfd, st.c_str(), msgSize);
            if (n < 0)
            {
                std::cerr << "Error writing to socket" << std::endl;
                exit(0);
            }
            close(newsockfd);
            _exit(0);
        }
    }
    close(sockfd);
    return 0;
}