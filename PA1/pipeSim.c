#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main(int argc, char* argv[])
{
    printf("I'm SHELL process, with PID:%d - Main command is: man ping | grep -extraOptions \"-f\" command.\n", (int) getpid());

    // Initializing a pipe here.
    int pip[2];
    (void) pipe(pip);

    // Let's take the parameters here.
    // Forking Happens Here
    int rc = fork();

    // Safety measure.
    if (rc < 0)
    {exit(1);}

    // Main child process branch
    // Main child branch runs the "man" command whereas
    // "grep" is run as a granchild sub-branch.
    if (rc == 0)
    {
        // Grandchild Process
        // This is the process that will run the "grep" command.
        if (fork() == 0)
        {
            printf("I'm GREP process, with PID:%d - My command is: grep -extraOptions \"-f\"\n", (int) getpid());

            // Pipe is ready when the child process "man" is terminated.
            // Direct the output to the text file.
            freopen("output.txt", "a+", stdout);

            close(STDIN_FILENO);
            close(pip[1]);
            dup(pip[0]);
            close(pip[0]);

            // This will take over the grandchild process's runtime stack.
            char *myargs[8];
            myargs[0] = strdup("grep");
            myargs[1] = strdup("-m");
            myargs[2] = strdup("1");
            myargs[3] = strdup("-A");
            myargs[4] = strdup("6");
            myargs[5] = strdup("--");
            myargs[6] = strdup("-f");
            myargs[7] = NULL;
            execvp(myargs[0], myargs);
        }

            // Child Process.
            // This runs the "man ping" command.
        else
        {
            printf("I'm MAN process, with PID:%d - My command is: man ping\n", (int) getpid());

            // Redirect the output of "man" into the "pipe"
            close(STDOUT_FILENO);
            close(pip[0]);
            dup(pip[1]);
            close(pip[1]);

            // This process takes over the address stack.
            char *myargs[3];
            myargs[0] = strdup("man");
            myargs[1] = strdup("ping");
            myargs[2] = NULL;
            execvp(myargs[0], myargs);
        }

    }

    else
    {
        int wc = wait(NULL);
        printf("I'm SHELL process, with PID:%d - execution is completed, you can find the results in output.txt\n", (int) getpid());
    }

    return 0;
}