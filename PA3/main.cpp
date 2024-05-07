#include <pthread.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <queue>
#include <sstream>
#include <iomanip>
#include <cmath>

struct args
{
    std::string in; // input
    // std::string out;                   // output
    int num;                           // which call it is
    int *next;                         // for calling in next thread
    pthread_mutex_t *input_copy_mutex; // for shared data
    pthread_mutex_t *print_mutex;      // for printing
    pthread_cond_t *condition;         // The condition variable.
};

// node will be the main struct used for each task
struct node
{
    std::string name; // stores the task name
    int wceTime;      // stores the task worst case execution time
    int period;       // stores the task period
    int execLeft;     // stores how many executions this task has left in the period

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

// here is my function used in multi-threading
void *RMSA(void *x_void_ptr) // RMSA --> Rate Monotonic Scheduling Algorithm
{
    args Boat = *(args *)x_void_ptr;             // Deinitilization
    int localNum = Boat.num;                     // turning shared resource into a local resource
    std::string localString = Boat.in;           // turning shared resource into a local resource
    pthread_mutex_unlock(Boat.input_copy_mutex); // unlock copying semaphore now that we have it all local

    std::priority_queue<node> pq;
    std::vector<node> Ttasks;
    std::istringstream iss(localString);

    // initializing variables
    std::string name;
    std::string output = "";
    int wceTime, period;
    int numTasks = 0;
    double util = 0;
    std::string out;

    // pushing into two queues, one to help me print, one to be priority
    while (iss >> name >> wceTime >> period)
    {
        pq.push(node(name, wceTime, period, wceTime));
        Ttasks.push_back(node(name, wceTime, period, wceTime));
    }

    // printing CPU #
    int hyperPeriod = calculateHyperPeriod(pq);

    out += "CPU " + std::to_string(localNum) + "\n";
    out += "Task scheduling information: ";

    // this for-loop gets the utilization number, as well as line one printing
    for (std::vector<node>::const_iterator it = Ttasks.begin(); it != Ttasks.end(); ++it)
    {
        const node &task = *it;
        numTasks++;
        util = util + (static_cast<double>(task.wceTime) / static_cast<double>(task.period));
        out += task.name + " (WCET: " + std::to_string(task.wceTime) + ", Period: " + std::to_string(task.period);
        if (numTasks < Ttasks.size())
        {
            out += "), "; // Print comma if it's not the last element
        }
        else
        {
            out += ") "; // If it's the last element, don't print comma
        }
    }

    // more printing
    std::stringstream utilStream;
    utilStream << std::fixed << std::setprecision(2) << util;
    out += "\nTask set utilization: " + utilStream.str();

    out += "\nHyperperiod: " + std::to_string(hyperPeriod) + "\n";
    out += "Rate Monotonic Algorithm execution for CPU " + std::to_string(localNum) + ":\n";

    // logic based on utilization and formula given in directions
    if (util > 1)
    {
        out += "The task set is not schedulable\n";
    }
    else if (util > calculateExpression(numTasks))
    {
        out += "Task set schedulability is unknown\n";
    }
    else // find the scheduling diagram
    {
        out += "Scheduling Diagram for CPU " + std::to_string(localNum) + ": ";
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
    out += convertToTaskSchedule(output);
    out += "\n\n";

    pthread_mutex_lock(Boat.print_mutex); // second critical section --> check if num = localNum

    while ((*(Boat.next)) != localNum)
    {
        pthread_cond_wait(Boat.condition, Boat.print_mutex); // if not set it to wait until it does
    }

    pthread_mutex_unlock(Boat.print_mutex); // unlock crit section

    std::cout << out; // print accumulated output

    pthread_mutex_lock(Boat.print_mutex); // lock so we can increment the shared resource

    (*Boat.next)++;

    pthread_cond_broadcast(Boat.condition); // wake up other threads since now we are done

    pthread_mutex_unlock(Boat.print_mutex); // unlock semaphore

    return NULL;
}

int main()
{

    struct args x;
    std::vector<std::string> store;
    pthread_mutex_t input_copy_mutex;
    pthread_mutex_init(&input_copy_mutex, NULL); // semaphore for copying shared resources

    pthread_mutex_t print_mutex;
    pthread_mutex_init(&print_mutex, NULL); // semaphore for printing

    static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
    static int next = 1;
    x.input_copy_mutex = &input_copy_mutex;
    x.print_mutex = &print_mutex;
    x.condition = &condition;
    x.next = &next;

    std::string input = "";
    int count1 = 0; // allows me to set pthread_t tid[]
    int count2 = 0; // allows me to make threads increment (++) each time

    while (getline(std::cin, input))
    {
        count1++;
        store.push_back(input);
    }

    pthread_t tid[count1];

    for (int i = 0; i < count1; i++)
    {
        pthread_mutex_lock(&input_copy_mutex); // Enter first critical section
        x.in = store[i];                       // sending the input
        x.num = count2 + 1;                    // sending which CPU# it will handle

        if (pthread_create(&tid[count2], NULL, RMSA, &x)) // only using one memory address
        {
            std::cerr << "Error creating thread" << std::endl;
            return 1;
        }
        count2++;
    }

    for (int i = 0; i < count1; i++) // joining threads
        pthread_join(tid[i], NULL);

    return 0;
}
