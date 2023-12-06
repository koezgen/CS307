//
// Created by koezgen on 11/26/23.
//

#include <sstream>
#include <string>
#include <thread>
#include <iostream>
#include <mutex>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <vector>
#include <cstring>

using namespace std;

// Locking and unlocking purposes.
mutex thread_mutex;

// Struct to hold the items required for execvp.
struct line {
    bool isRedirected = false;
    string redirectFile;

    string command;
    string inputs = "";
    string options = "";
    string redirection;
    string background_job = "n";
};

// Listener thread for non io processes.
// Opening a pipe is not in the Critical Section of the thread function.
void listener(int pipe_out)
{
    std::thread::id this_id = std::this_thread::get_id();

    const size_t bufferSize = 1024;
    char buffer[bufferSize];
    setvbuf(stdout, buffer, _IOFBF, 1024);

    ssize_t bytesRead;

    std::stringstream dataStream;

    bytesRead = read(pipe_out, buffer, bufferSize - 1);
    buffer[bytesRead] = '\0'; // Null-terminate the string
    dataStream << buffer; // Accumulate the data

    // CRITICAL SECTION OF THE THREAD.
    thread_mutex.lock();
    cout << "----" << this_id << endl;
    std::cout << dataStream.str();
    cout << "----" << this_id << endl;
    fflush(stdout);
    thread_mutex.unlock();
}

// Returns the line object.
line parser(string line)
{
    struct line obj;

    if (line.find('<') != string::npos || line.find('>') != string::npos)
    {
        // DIVIDE AND CONQUER
        if (line.find('<') != string::npos)
        {
            int pos = line.find("<");
            obj.redirection = "<";
            obj.isRedirected = true;

            string part1 = line.substr(0, pos);
            istringstream isspt1(part1);
            string part2 = line.substr(pos, string::npos);
            istringstream isspt2(part2);
            string word;

            while (isspt1 >> word)
            {
                if (word[0] == '-')
                {
                    obj.options += word;
                }

                // Check whether a command was issued or not.
                else if (!obj.command.empty())
                {
                    obj.inputs = word;
                }

                // Now we have a command.
                else
                {
                    obj.command = word;
                }
            }

            while (isspt2 >> word)
            {
                if (word == "&")
                {
                    obj.background_job = "y";
                }

                else
                {
                    obj.redirectFile = word;
                }
            }

        }

        if (line.find('>') != string::npos)
        {
            int pos = line.find(">");
            obj.redirection = ">";
            obj.isRedirected = true;

            string part1 = line.substr(0, pos);
            istringstream isspt1(part1);
            string part2 = line.substr(pos, string::npos);
            istringstream isspt2(part2);
            string word;

            while (isspt1 >> word)
            {
                if (word[0] == '-')
                {
                    obj.options += word;
                }

                    // Check whether a command was issued or not.
                else if (!obj.command.empty())
                {
                    obj.inputs = word;
                }

                    // Now we have a command.
                else
                {
                    obj.command = word;
                }
            }

            while (isspt2 >> word)
            {
                if (word == "&")
                {
                    obj.background_job = "y";
                }

                else
                {
                    obj.redirectFile = word;
                }
            }
        }
    }

    else
    {
        obj.redirection = "-";
        istringstream iss(line);
        string word;

        while (iss >> word)
        {
            if (word[0] == '-')
            {
                obj.options += word;
            }

            else if (word == "&")
            {
                obj.background_job = "y";
            }

            // Check whether a command was issued or not.
            else if (!obj.command.empty())
            {
                obj.inputs = word;
            }

            // Now we have a command.
            else
            {
                obj.command = word;
            }
        }
    }

    return obj;
}

void Wait(std::vector<pid_t>& pids)
{
    while (!pids.empty())
    {
        pid_t process_id = pids.back();
        pids.pop_back();

        int status;
        if (waitpid(process_id, &status, 0) == -1)
        {
            std::cerr << "Error waiting for process " << process_id << std::endl;
        }
    }
}

int main()
{
    // I need these numbers.
    int thr_count = 0;
    pid_t rc = getpid();
    int flag = 0;

    ofstream parse_txt("parse.txt");

    std::ifstream file("commands.txt");
    if (!file.is_open()) {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    std::string line;
    int lineCount = 0;
    int greaterThanCount = 0;
    int lessThanCount = 0;

    while (std::getline(file, line)) {
        lineCount++;

        for (char ch : line) {
            if (ch == '>') {
                greaterThanCount++;
            } else if (ch == '<') {
                lessThanCount++;
            }
        }
    }

    file.close();

    ifstream commandFile;
    commandFile.open("commands.txt");

    // In the shell process, we hold the PID's
    vector<pid_t> pidArr;

    // This is where the commands are fetched.
    while ((rc != 0) && getline(commandFile, line))
    {
        if (line == "wait")
        {
            Wait(pidArr);
        }

        else
        {
            struct line command = parser(line);

            parse_txt << "---------" << endl;
            parse_txt << "Command: " << command.command << endl;
            parse_txt << "Inputs: " << command.inputs << endl;
            parse_txt << "Options: " << command.options << endl;
            parse_txt << "Redirection: " << command.redirection << endl;
            parse_txt << "Background Job: " << command.background_job << endl;
            parse_txt << "----------" << endl;

            if (!command.isRedirected)
            {
                int pneu[2];
                pipe(pneu);

                rc = fork();

                if (rc == 0)
                {
                    close(STDOUT_FILENO);
                    close(pneu[0]);
                    dup(pneu[1]);
                    close(pneu[1]);

                    vector<string> argv;

                    argv.push_back(command.command);

                    if (!command.inputs.empty()) {
                        argv.push_back(command.inputs);
                    }

                    if (!command.options.empty()){
                        argv.push_back(command.options);
                    }

                    char** args = new char*[argv.size() + 1];

                    for (size_t i = 0; i < argv.size(); ++i) {
                        args[i] = strdup(argv[i].c_str());
                    }

                    args[argv.size()] = nullptr;

                    execvp(args[0], args);
                }

                else
                {
                    (new thread(&listener, pneu[0]))->detach();

                    if (command.background_job == "n")
                    {
                        int status;
                        waitpid(rc, &status, 0);
                    }

                    else
                    {
                        pidArr.push_back(rc);
                    }
                }
            }

            // Process interleaving is left to the OS
            else
            {
                // Checking whether I am in child process or not.
                if (command.redirection == "<") {
                    int pneu[2];
                    pipe(pneu);

                    int rc = fork();
                    if (rc == 0)
                    {
                        int fd = open(command.redirectFile.c_str(), O_RDONLY);
                        if (fd < 0) {
                            perror("open");
                            exit(EXIT_FAILURE);
                        }

                        // Redirect standard input to the file
                        dup2(fd, STDIN_FILENO);
                        close(fd);

                        close(STDOUT_FILENO);
                        close(pneu[0]);
                        dup(pneu[1]);
                        close(pneu[1]);

                        vector<string> argv;

                        argv.push_back(command.command);

                        if (!command.inputs.empty()) {
                            argv.push_back(command.inputs);
                        }

                        if (!command.options.empty()){
                            argv.push_back(command.options);
                        }

                        char **args = new char *[argv.size() + 1];

                        for (size_t i = 0; i < argv.size(); ++i) {
                            args[i] = strdup(argv[i].c_str());
                        }

                        args[argv.size()] = nullptr;

                        // Execute the command
                        execvp(args[0], args);

                        // If execvp returns, there was an error
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }

                    else
                    {
                        (new thread(&listener, pneu[0]))->detach();

                        if (command.background_job == "n")
                        {
                            int status;
                            waitpid(rc, &status, 0);
                        }

                        else
                        {
                            pidArr.push_back(rc);
                        }
                    }
                }

                else
                {
                    int rc = fork();

                    if (rc == 0)
                    {
                        int fd = open(command.redirectFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0) {
                            perror("open");
                            exit(EXIT_FAILURE);
                        }

                        // Redirect standard output to the file
                        dup2(fd, STDOUT_FILENO);
                        close(fd);

                        vector<string> argv;

                        argv.push_back(command.command);

                        if (!command.inputs.empty()) {
                            argv.push_back(command.inputs);
                        }

                        if (!command.options.empty()){
                            argv.push_back(command.options);
                        }

                        char** args = new char*[argv.size() + 1];

                        for (size_t i = 0; i < argv.size(); ++i) {
                            args[i] = strdup(argv[i].c_str());
                        }

                        args[argv.size()] = nullptr;

                        // Execute the command
                        execvp(args[0], args);

                        // If execvp returns, there was an error
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }

                    else
                    {
                        if (command.background_job == "n")
                        {
                            int status;
                            waitpid(rc, &status, 0);
                        }

                        else
                        {
                            pidArr.push_back(rc);
                        }
                    }
                }
            }
        }
    }

    commandFile.close();
    parse_txt.close();
    return 0;
}