#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

int main()
{
    int fd;
    int fd_recive;

    printf("Programm Client : \n");

    if(mkfifo("cl_sv_fifo", 0777) < 0)
    {
        if(errno != EEXIST)
        {
            perror("ERROR : mkfifo() \n");
            exit(1);
        }
    }

    if(mkfifo("recive_fifo", 0777) < 0)
    {
        if(errno != EEXIST)
        {
            perror("ERROR : mkfifo() \n");
            exit(1);
        }
    }

    if((fd = open("cl_sv_fifo", O_WRONLY)) < 0)
    {
        perror("ERROR : open(fifo) \n");
        exit(1);
    }

    if((fd_recive = open("recive_fifo", O_RDONLY)) < 0)
    {
        perror("ERROR : open(fifo) \n");
        exit(1);
    }

    while(true)
    {
        char comm[50];

        int memo;

        printf("\nInsert the command : \n");
        scanf("%s", comm);
        printf("Command : %s \n\n", comm);

        if(write(fd, comm, sizeof(comm)) < 0)
        {
            perror("ERROR : write(fifo) \n");
            exit(1);
        }

        if(read(fd_recive, &memo, sizeof(memo)) < 0)
        {
            perror("ERROR : read(fifo) \n");
            exit(1);
        }

        char msg[memo];

        if(read(fd_recive, msg, memo) < 0)
        {
            perror("ERROR : read(fifo) \n");
            exit(1);
        }

        printf("%s \n", msg);

        if(strcmp(comm, "quit") == 0)
            break;
    }

    close(fd);
    close(fd_recive);

    return 0;
}
