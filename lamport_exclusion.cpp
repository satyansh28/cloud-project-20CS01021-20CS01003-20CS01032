#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <stack>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <queue>
#include <unistd.h>
#include <vector>
using namespace std;
int listening_port;
vector<int> peer_ports;
int replies = 0;
// atomic<stack<vector<int>>> timestampStack;
priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
class Peer
{
public:
  Peer(int port, int num_peers, int current_process_indexx)
  {

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    current_process_index = current_process_indexx;
    for (int i = 0; i < num_peers; ++i)
    {
      timestamp = 0;
    }
    // timestamp(num_peers,make_shared<atomic<int>>(0));
    if (sockfd < 0)
    {
      cerr << "ERROR opening socket\n";
      exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      //   cerr << "ERROR on binding\n";
      perror("ERROR on binding \n");
      exit(1);
    }
  }
  bool canGoCritical()
  {
    if (replies == peer_ports.size() - 1 && pq.top().second == listening_port)
      return true;
    return false;
  }
  void static executeCS()
  {
    cout << "GOT INSIDE CRITICAL SECTION!" << endl;
    usleep(8000000); // sleep for 8 seconds
    for (int port : peer_ports)
    {
      if (port == listening_port)
        continue;
      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)
      {
        cerr << "Error opening socket.\n";
        return;
      }
      struct sockaddr_in serv_addr;
      bzero((char *)&serv_addr, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(port);
      serv_addr.sin_addr.s_addr = INADDR_ANY;

      // Connect to the server
      if (connect(sockfd, (struct sockaddr *)&serv_addr,
                  sizeof(serv_addr)) < 0)
      {
        cerr << "Error connecting to the server.\n";
        return;
      }

      string timestamp_str = "o\n" + to_string(0) + "\n" + to_string(listening_port) + "\n";

      send(sockfd, timestamp_str.c_str(), timestamp_str.size(), 0);
      // Close the socket
      cout << "Release message sent to " << port << endl;
      close(sockfd);
    }
  }
  void startAccept(int current_process_index)
  {

    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
    {
      cerr << "ERROR on accept\n";
      exit(1);
    }

    char buffer[1024];
    bzero(buffer, 1024);
    int n = recv(newsockfd, buffer, 1024, 0);
    if (n < 0)
    {
      cerr << "ERROR reading from socket\n";
      exit(1);
    }
    // emulate delay
    usleep(500000);

    int received_timestamp = -1;
    stringstream ss(buffer);
    string temp;
    int port;
    bool isReply = false, isReleased = false;
    while (getline(ss, temp, '\n'))
    {
      if (!temp.empty() && temp[0] == 'r') // reply
        isReply = true;
      else if (!temp.empty() && temp[0] == 'o') // open
        isReleased = true;
      else if (!temp.empty() && temp[0] == 'q') // request
        continue;
      else if (!temp.empty() && received_timestamp == -1)
        received_timestamp = (stoi(temp));
      else if (!temp.empty())
        port = stoi(temp);
    }

    // Compare the received timestamps with the current timestamps
    if (isReply)
    {
      if (port == listening_port)
      {
        replies += 1;
        cout << "Got a Reply!" << endl;
        if (canGoCritical())
        {
          replies=0;
          pq.pop();
          thread cs(executeCS);
          cs.detach();
        }
      }
    }
    else if (isReleased)
    {
      pq.pop();
      cout << "Got a Release message!" << endl;
      if (canGoCritical())
      {
        replies=0;
        pq.pop();
        thread cs(executeCS);
        cs.detach();
      }
    }
    else
    {
      int current_timestamp = timestamp;
      timestamp = max(current_timestamp, received_timestamp) + 1;
      // Create an event by incrementing the timestamp at the current index

      cout << "Got a Request from " << port << endl;
      pq.push(pair<int, int>(received_timestamp, port));
      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)
      {
        cerr << "Error opening socket.\n";
        return;
      }
      struct sockaddr_in serv_addr;
      bzero((char *)&serv_addr, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(port);
      serv_addr.sin_addr.s_addr = INADDR_ANY;

      // Connect to the server
      if (connect(sockfd, (struct sockaddr *)&serv_addr,
                  sizeof(serv_addr)) < 0)
      {
        cerr << "Error connecting to the server.\n";
        return;
      }

      string timestamp_str = "r\n" + to_string(timestamp) + "\n" + to_string(port) + "\n";

      send(sockfd, timestamp_str.c_str(), timestamp_str.size(), 0);
      // Close the socket
      cout << " Reply sent to " << port << endl;
      close(sockfd);
    }
    close(newsockfd);
  }
  void startListening()
  {

    int index = current_process_index;
    listen(sockfd, 5);
    threads.push_back(thread([this, index]()
                             {
      
      while (true) {
        startAccept(index);
      } }));
  }

  void startEventGeneration()
  {
    int index = current_process_index;
    threads.push_back(thread([this, index]()
                             {
      while (true) {
        int option;
        cout << "Enter 1 to request Critical Section, or \n2 to print current queue or \n3 to exit the code.\n";

        cin >> option;
        if (option == 1) {
          timestamp++;
          pq.push(pair<int,int>(timestamp,listening_port));
          
          for(int port:peer_ports)
          {
            if(port==listening_port)
              continue;
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
              cerr << "Error opening socket.\n";
              return;
            }
            struct sockaddr_in serv_addr;
            bzero((char *)&serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(port);
            serv_addr.sin_addr.s_addr = INADDR_ANY;

            // Connect to the server
            if (connect(sockfd, (struct sockaddr *)&serv_addr,
                        sizeof(serv_addr)) < 0) {
              cerr << "Error connecting to the server.\n";
            return;
            }

            string timestamp_str="q\n"+to_string(timestamp)+"\n"+to_string(listening_port)+"\n";
            
            send(sockfd, timestamp_str.c_str(), timestamp_str.size(), 0);
            // Close the socket
            cout  << "Request sent to "<<port<<endl;
            close(sockfd);
            
          }
          
          
        } else if (option == 2) {
          vector<pair<int,int>>temp;
          while(!pq.empty())
          {
            temp.push_back(pq.top());
            cout<<pq.top().first<<":"<<pq.top().second<<endl;
            pq.pop();
          }
          for(int i=0;i<temp.size();i++)
            pq.push(temp[i]);
        } else if (option == 3) {
          
          exit(0);
        }
      } }));
  }
  void waitForThreads()
  {
    for (auto &thread : threads)
    {
      if (thread.joinable())
      {
        thread.join();
      }
    }
  }

private:
  int sockfd, newsockfd;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  atomic<int> timestamp;
  int current_process_index;
  vector<thread> threads;
};

int main()
{

  cout << "Enter the listening port for this process: ";
  cin >> listening_port;
  int num_peers;
  cout << "Enter the number of peers(excluding this): ";
  cin >> num_peers;

  if (num_peers < 1)
  {
    cerr << "Number of peers must be at least 1.\n";
    return 1;
  }

  for (int i = 0; i < num_peers; ++i)
  {
    cout << "Enter the port for peer " << i + 1 << ": ";
    int t;
    cin >> t;
    peer_ports.push_back(t);
  }

  // Add the listening port to the vector
  peer_ports.push_back(listening_port);

  // Sort the vector
  sort(peer_ports.begin(), peer_ports.end());

  // Find the index of the listening port
  int current_process_index =
      find(peer_ports.begin(), peer_ports.end(), listening_port) -
      peer_ports.begin();

  Peer p(listening_port, num_peers, current_process_index);
  p.startListening();
  p.startEventGeneration();
  p.waitForThreads();
  // Now you have a vector of peer ports that you can use

  return 0;
}