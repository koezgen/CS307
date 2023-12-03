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
    thread_mutex.lock();
    std::thread::id this_id = std::this_thread::get_id();

    const size_t bufferSize = 256;
    char buffer[bufferSize];
    ssize_t bytesRead;

    cout << "----" << this_id;

    while ((bytesRead = read(pipe_out, buffer, bufferSize - 1)) > 0)
    {
        buffer[bytesRead] = '\0'; // Null-terminate the string
        std::cout << buffer << std::endl;
    }

    cout << "----" << this_id;
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
        flag = 0;

        // This is a very special situation. We have to wait for all the active processes to finish.
        if (line == "wait")
        {
            Wait(pidArr);
        }

        else
        {
            // This is the place where we handle everything.
            // A lot of things will happen here.

            struct line command = parser(line);

            parse_txt << "---------" << endl;
            parse_txt << "Command: " << command.command << endl;
            parse_txt << "Inputs: " << command.inputs << endl;
            parse_txt << "Options: " << command.options << endl;
            parse_txt << "Redirection: " << command.redirection << endl;
            parse_txt << "Background Job: " << command.background_job << endl;
            parse_txt << "----------" << endl;

            // The threads should be lifted from the
            // Spin condition.
            if (!command.isRedirected)
            {
                // J'ai eu mon attestation du français recemment.
                int pneu[2];
                pipe(pneu);

                rc = fork();

                // Checking whether I am in child process or not.
                if (rc == 0)
                {
                    close(STDOUT_FILENO);
                    close(pneu[0]);
                    dup(pneu[1]);
                    close(pneu[1]);

                    vector<string> argv;

                    argv.push_back(command.command);
                    argv.push_back(command.options);

                    if (!command.inputs.empty()) {
                        argv.push_back(command.inputs);
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
                    thread thr(&listener, pneu[0]);

                    if (command.background_job == "n")
                    {
                        wait(nullptr);
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
                // J'ai eu mon attestation du français recemment.
                int pneu[2];
                pipe(pneu);

                int rc = fork();

                // Checking whether I am in child process or not.
                if (rc == 0)
                {

                    if (command.redirection == "<") {
                        vector<string> argv;

                        argv.push_back(command.command);
                        argv.push_back(command.options);

                        if (!command.inputs.empty()) {
                            argv.push_back(command.inputs);
                        }

                        char** args = new char*[argv.size() + 1];

                        for (size_t i = 0; i < argv.size(); ++i) {
                            args[i] = strdup(argv[i].c_str());
                        }

                        args[argv.size()] = nullptr;

                        pid_t pid = fork();

                        if (pid == 0) { // Child process
                            // Open the file for input redirection
                            int fd = open(command.redirectFile.c_str(), O_RDONLY);
                            if (fd < 0) {
                                perror("open");
                                exit(EXIT_FAILURE);
                            }

                            // Redirect standard input to the file
                            dup2(fd, STDIN_FILENO);
                            close(fd);

                            // Execute the command
                            execvp(args[0], args);

                            // If execvp returns, there was an error
                            perror("execvp");
                            exit(EXIT_FAILURE);
                        } else if (pid > 0) { // Parent process
                            // Optionally wait for the child process to finish
                            waitpid(pid, NULL, 0);
                        } else {
                            perror("fork");
                        }

                        // Free the dynamically allocated memory
                        for (size_t i = 0; i < argv.size(); ++i) {
                            free(args[i]);
                        }
                        delete[] args;
                    }

                    else
                    {
                        vector<string> argv;

                        argv.push_back(command.command);
                        argv.push_back(command.options);

                        if (!command.inputs.empty()) {
                            argv.push_back(command.inputs);
                        }

                        char** args = new char*[argv.size() + 1];

                        for (size_t i = 0; i < argv.size(); ++i) {
                            args[i] = strdup(argv[i].c_str());
                        }

                        args[argv.size()] = nullptr;

                        pid_t pid = fork();

                        if (pid == 0) { // Child process
                            // Open the file for output redirection
                            int fd = open(command.redirectFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                            if (fd < 0) {
                                perror("open");
                                exit(EXIT_FAILURE);
                            }

                            // Redirect standard output to the file
                            dup2(fd, STDOUT_FILENO);
                            close(fd);

                            // Execute the command
                            execvp(args[0], args);

                            // If execvp returns, there was an error
                            perror("execvp");
                            exit(EXIT_FAILURE);
                        } else if (pid > 0) { // Parent process
                            // Optionally wait for the child process to finish
                            waitpid(pid, NULL, 0);
                        } else {
                            perror("fork");
                        }

                        // Free the dynamically allocated memory
                        for (size_t i = 0; i < argv.size(); ++i) {
                            free(args[i]);
                        }
                        delete[] args;
                    }
                }

                else if (command.background_job == "n")
                {
                    wait(nullptr);
                }
            }
        }
    }

    commandFile.close();
    parse_txt.close();
    return 0;
}