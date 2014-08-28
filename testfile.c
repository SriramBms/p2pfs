#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    fd_set rfds;
    struct timeval tv;
    int retval, len;
    char buff[255] = {0};

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    /* Wait up to five seconds. */
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    retval = select(1, &rfds, NULL, NULL, &tv);

    if (retval == -1){
        perror("select()");
        exit(EXIT_FAILURE);
    }
    else if (retval){
        /* FD_ISSET(0, &rfds) is true so input is available now. */

        /* Read data from stdin using fgets. */
        fgets(buff, sizeof(buff), stdin);

        /* Remove trailing newline character from the input buffer if needed. */
        len = strlen(buff) - 1;
        if (buff[len] == '\n')
            buff[len] = '\0';

        printf("'%s' was read from stdin.\n", buff);
    }
    else
        printf("No data within five seconds.\n");            

    exit(EXIT_SUCCESS);
}