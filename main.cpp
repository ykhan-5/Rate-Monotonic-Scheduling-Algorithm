#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <queue>
#include <sstream>
#include <iomanip>
#include <cmath>

struct args
{
  std::string in; //input
  std::string out; //output
  int num; //which call it is
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

// this function takes the string i create throughout the program and outputs it formatted.
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

  // getting input and initializing structures
  struct args *Boat = (struct args *)x_void_ptr;
  std::priority_queue<node> pq;
  std::vector<node> Ttasks;
  std::istringstream iss(Boat->in);

  // initializing variables
  std::string name;
  std::string output = "";
  int wceTime, period;
  int numTasks = 0;
  double util = 0;
  int threadCount;

  // pushing into two queues, one to help me print, one to be priority
  while (iss >> name >> wceTime >> period)
  {
    pq.push(node(name, wceTime, period, wceTime));
    Ttasks.push_back(node(name, wceTime, period, wceTime));
  }

  // printing CPU #
  threadCount++;
  int hyperPeriod = calculateHyperPeriod(pq);
  // std::cout << "CPU " << threadCount << "\n";
  Boat->out += "Task scheduling information: ";

  // this for-loop gets the utilization number, as well as line one printing
  for (std::vector<node>::const_iterator it = Ttasks.begin(); it != Ttasks.end(); ++it)
  {
    const node &task = *it;
    numTasks++;
    util = util + (static_cast<double>(task.wceTime) / static_cast<double>(task.period));
    Boat->out += task.name + " (WCET: " + std::to_string(task.wceTime) + ", Period: " + std::to_string(task.period);
    if (numTasks < Ttasks.size())
    {
      Boat->out += "), "; // Print comma if it's not the last element
    }
    else
    {
      Boat->out += ") "; // If it's the last element, don't print comma
    }
  }

  // more printing
  std::stringstream utilStream;
  utilStream << std::fixed << std::setprecision(2) << util;
  Boat->out += "\nTask set utilization: " + utilStream.str();

  Boat->out += "\nHyperperiod: " + std::to_string(hyperPeriod) + "\n";
  Boat->out += "Rate Monotonic Algorithm execution for CPU " + std::to_string(Boat->num) + ":\n";

  // logic based on utilization and formula given in directions
  if (util > 1)
  {
    Boat->out += "The task set is not schedulable\n";
  }
  else if (util > calculateExpression(numTasks))
  {
    Boat->out += "Task set schedulability is unknown\n";
  }
  else // find the scheduling diagram
  {
    Boat->out += "Scheduling Diagram for CPU " + std::to_string(Boat->num) + ": ";
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
  Boat->out += convertToTaskSchedule(output);
  Boat->out += "\n\n";

  return NULL;
}

int main()
{
  std::string input = "";
  int count1 = 0; // allows me to set pthread_t tid[] number without using a for-loop
  int count2 = 0; // allows me to make threads and as i make them I increment (++)

  while (getline(std::cin, input))
  {
    count1++;
  }

  pthread_t tid[count1];
  struct args x[count1];

  std::cin.clear();                 // Clear the EOF flag set by getline
  std::cin.seekg(0, std::ios::beg); // Reset the stream to the beginning

  while (getline(std::cin, input))
  {
    x[count2].in = input; //sending the input
    x[count2].num = count2+1; //sending which CPU# it will handle
    
    if (pthread_create(&tid[count2], NULL, RMSA, (void *)&x[count2]))
    {
      std::cerr << "Error creating thread" << std::endl;
      return 1;
    }
    count2++;
    // pthread_join(tid[count2], NULL); 
  }

  for (int i = 0; i < count1; i++) //joining threads
    pthread_join(tid[i], NULL); 

  for (int j = 0; j < count1; j++) //cout of information
  {
    std::cout << "CPU " << j+1 << std::endl;
    std::cout << x[j].out << std::endl;
  }
  return 0;
}
