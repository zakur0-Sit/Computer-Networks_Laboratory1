#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <utmp.h>

int pipe_login[2], pipe_key[2], pipe_proc[2], pipe_logout_key[2], pipe_logout[2], pipe_quit[2];
int socket_login[2], socket_users[2], socket_proc[2], socket_logout[2], socket_quit[2];

void CharFit(char* line)
{
    // Sterg new line de la fiecare linie din while pentru a putea compara cu user
    // -----------------------------------------------------------

    int len = strlen(line);
    if(len != 0)
        line[len-1] = '\0';

    // -----------------------------------------------------------
}

void UserTest(char* user, bool *key)
{
    // Compar userul primit bu lista de useri din fiserul users.txt
    // ------------------------------------------------------------

    FILE  * fd;

    if((fd = fopen("users.txt", "r")) < 0)
    {
        perror("ERROR : open() \n");
        exit(1);
    }

    char *line;

    char answer1[50];
    char answer2[15] = "No user found!";
    char answer3[25] = "User already logged!";

    size_t len = 0;
    ssize_t read;

    sprintf(answer1, "Wellcome back %s \n", user);

    while((read = getline(&line, &len, fd)) != -1)
    {
        CharFit(line);
        if(*key == 1)
        {
            write(socket_login[1], answer3, sizeof(answer3));
            break;
        }

        if(strcmp(line, user) == 0)
        {
            write(socket_login[1], answer1, sizeof(answer1));
            *key = 1;
            break;
        }
    }

    if(strcmp(line, user) != 0)
        write(socket_login[1], answer2, sizeof(answer2));

    close(pipe_key[0]);
    write(pipe_key[1], key, sizeof(bool));
    close(pipe_key[1]);
    fclose(fd);

    // --------------------------------------------------
}

void LoggedUsers()
{
    // Folosesc utmp pentru a scoate toate informatiile necesare
    // --------------------------------------------------

    struct utmp *data;

    data = getutent();

    char allUserInfo[5000];
    char info[400];
    int offset = 0;

    while(data != 0)
    {
        if(data->ut_host[0] != '\0') {
            sprintf(info, "Username : %s \nHostname for remote loggin : %s \nTime entry was made : %d \n\n", data -> ut_user, data ->ut_host, data -> ut_tv.tv_sec);
            strcpy(allUserInfo + offset, info);
            offset += strlen(info);
        }

        data = getutent();
    }

    write(socket_users[1], allUserInfo, sizeof(allUserInfo));

    // ---------------------------------------------------
}

char* get_val_stat (int pid, const char* field_name)
{
    // Creez o functie char* pentru a scoate informatiile necesare din fisierul /proc/<pid>/status
    // ---------------------------------------------------

    FILE* fd_stat;

    char* value = NULL;

    char line[50];
    char path[20];

    sprintf(path, "/proc/%d/status", pid);

    if((fd_stat = fopen(path, "r")) == NULL)
    {
        perror("ERROR : fopen() \n");
        exit(1);
    }

    while(fgets(line, sizeof(line), fd_stat))
    {
        if (strncmp(line, field_name, strlen(field_name)) == 0)
        {
            char* val_start = line + strlen(field_name);

            while (*val_start == ' ' || *val_start == '\t')
            {
                val_start++; // Sărim peste spații și tab-uri
            }

            // Eliminăm tab-urile din valoare
            for (char* p = val_start; *p; p++)
                if (*p == '\t')
                    *p = ' ';

            value = strdup(val_start);
            break;
        }
    }

    return value;
    fclose(fd_stat);

    // ---------------------------------------------------
}

void get_proc_info (int pid)
{
    // Afisez toate informatiile dpimite din get_val_stat
    // ---------------------------------------------------

    char* name = get_val_stat(pid, "Name:");
    char* state = get_val_stat(pid, "State:");
    char* ppid = get_val_stat(pid, "PPid:");
    char* uid = get_val_stat(pid, "Uid:");
    char* vmsize = get_val_stat(pid, "VmSize:");

    char info[500];

    sprintf(info, "The process with PID: %d has the folowing data : \nName : %sState : %sPPid : %sUid : %sVmSize : %s", pid, name, state, ppid, uid, vmsize);

    write(socket_proc[1], info, sizeof(info));

    // ---------------------------------------------------
}

void Logout(bool *key)
{
    // Pentru logout se v-a reseta key = 0
    // ---------------------------------------------------

    char msg[20] = "Logout complete";

    *key = 0;

    close(pipe_logout_key[0]);
    write(pipe_logout_key[1], key, sizeof(bool));
    close(pipe_logout_key[1]);
    write(socket_logout[1], msg, sizeof(msg));

    // ---------------------------------------------------
}

int main()
{
    printf("Programm Server : \n\n");

    char* user;
    char* send;

    char buff[500];
    char user_child[100];
    char msg[100];
    char comm[50];

    bool key = 0;
    bool key_child;

    int pid_client;
    int child_pid_client;
    int fd;
    int fd_recive;
    int status;
    int memo;

    pid_t child_proc;
    pid_t child_logg_usr;
    pid_t child_quit;
    pid_t child_login;
    pid_t child_logout;


    if((fd = open("cl_sv_fifo", O_RDONLY)) < 0)
    {
        perror("ERROR : open(cl_sv_fifo) \n");
        exit(1);
    }

    if((fd_recive = open("recive_fifo", O_WRONLY)) < 0)
    {
        perror("ERROR : open(recive_fifo) \n");
        exit(1);
    }

    while(true)
    {
        socketpair(AF_UNIX, SOCK_STREAM, 0, socket_login);
        socketpair(AF_UNIX, SOCK_STREAM, 0, socket_users);
        socketpair(AF_UNIX, SOCK_STREAM, 0, socket_proc);
        socketpair(AF_UNIX, SOCK_STREAM, 0, socket_logout);
        socketpair(AF_UNIX, SOCK_STREAM, 0, socket_quit);

        if(read(fd, comm, sizeof(comm)) < 0)
        {
            perror("ERROR : read(fifo) \n");
            exit(1);
        }

        printf("Command : %s \n", comm);

        if(strcmp(comm, "quit") == 0)
        {
            switch(child_quit = fork())
            {
                case -1:
                    perror("ERROR : fork(child_quit)\n");
                    exit(1);

                case 0:
                    close(socket_quit[0]);
                    char result[50] = "Server is quitting. Goodbye!";
                    write(socket_quit[1], result, sizeof(result));
                    close(socket_quit[1]);
                    exit(status);

                default:
                    close(socket_quit[1]);
                    wait(&status);
                    read(socket_quit[0], buff, sizeof(buff));
                    memo = strlen(buff) + 1;
                    send = malloc(memo);

                    if(send == NULL)
                    {
                        perror("ERROR : malloc() \n");
                        exit(1);
                    }

                    printf("Memmory used : %d \n", memo);
                    strcpy(send, buff);
                    write(fd_recive, &memo, sizeof(memo));
                    write(fd_recive, send, memo);
                    free(send);
                    close(socket_quit[0]);
            }
            continue;
        }

        if (key == 0 && strstr(comm, "login:") == NULL)
        {
            sprintf(msg, "You are not logged in! \n");
            memo = strlen(msg) + 1;
            send = malloc(memo);

            if (send == NULL)
            {
                perror("ERROR : malloc() \n");
                exit(1);
            }

            strcpy(send, msg);
            write(fd_recive, &memo, sizeof(memo));
            write(fd_recive, send, memo);
            free(send);
            continue;
        }

        if(strstr(comm, "login:") != NULL)
        {
            if(pipe(pipe_login) || pipe(pipe_key) < 0)
            {
                perror("ERROR : pipe() \n");
                exit(1);
            }
            char* result = strstr(comm, "login:");
            user = result + strlen("login:");

            switch(child_login = fork())
            {
                case -1:
                    perror("ERROR : fork(child_login) \n");
                    exit(1);

                case 0:
                    close(socket_login[0]);
                    close(pipe_login[1]);
                    read(pipe_login[0], user_child, sizeof(user_child));
                    close(pipe_login[0]);
                    UserTest(user_child, &key);
                    close(socket_login[1]);
                    exit(status);

                default:
                    close(socket_login[1]);
                    close(pipe_login[0]);
                    write(pipe_login[1], user, strlen(user)+1);
                    close(pipe_login[1]);
                    wait(&status);
                    read(socket_login[0], buff, sizeof(buff));
                    close(socket_login[0]);
                    memo = strlen(buff) + 1;
                    send = malloc(memo);

                    if(send == NULL)
                    {
                        perror("ERROR : malloc() \n");
                        exit(1);
                    }

                    printf("Memmory used : %d \n", memo);
                    strcpy(send, buff);
                    write(fd_recive, &memo, sizeof(memo));
                    write(fd_recive, send, memo);
                    free(send);
            }

            close(pipe_key[1]);
            read(pipe_key[0], &key, sizeof(bool));
            close(pipe_key[0]);
        }
        else if(key == 1 && strcmp(comm, "get-logged-users") == 0)
        {
            switch (child_logg_usr = fork())
            {
                case -1:
                    perror("ERROR : fork(child_logg_usr) \n ");
                    exit(1);

                case 0:
                    close(socket_users[0]);
                    LoggedUsers();
                    close(socket_users[1]);
                    exit(status);

                default:
                    close(socket_users[1]);
                    wait(&status);
                    read(socket_users[0], buff, sizeof(buff));
                    memo = strlen(buff) + 1;
                    send = malloc(memo);

                    if(send == NULL)
                    {
                        perror("ERROR : malloc() \n");
                        exit(1);
                    }

                    printf("Memmory used : %d \n", memo);
                    strcpy(send, buff);
                    write(fd_recive, &memo, sizeof(memo));
                    write(fd_recive, send, memo);
                    free(send);
                    close(socket_users[0]);
            }
        }
        else if(key == 1 && strstr(comm, "get-proc-info:") != NULL)
        {
            char* result = strstr(comm, "get-proc-info:");
            if(pipe(pipe_proc) < 0)
            {
                perror("ERROR : pipe(users) \n");
                exit(1);
            }

            pid_client = atoi(result + strlen("get-proc-info:"));

            switch (child_proc = fork())
            {
                case -1:
                    perror("ERROR : fork() \n");
                    exit(1);

                case 0:
                    close(socket_proc[0]);
                    read(pipe_proc[0], &child_pid_client, sizeof(child_pid_client));
                    close(pipe_proc[0]);
                    get_proc_info(child_pid_client);
                    close(socket_proc[1]);
                    exit(status);

                default:
                    close(socket_proc[1]);
                    close(pipe_proc[0]);
                    write(pipe_proc[1], &pid_client, sizeof(pid_client));
                    close(pipe_proc[1]);
                    wait(&status);

                    read(socket_proc[0], buff, sizeof(buff));
                    memo = strlen(buff) + 1;
                    send = malloc(memo);

                    if(send == NULL)
                    {
                        perror("ERROR : malloc() \n");
                        exit(1);
                    }

                    printf("Memmory used : %d \n", memo);
                    strcpy(send, buff);
                    write(fd_recive, &memo, sizeof(memo));
                    write(fd_recive, send, memo);
                    free(send);
                    close(socket_proc[0]);
            }
        }
        else if(strcmp(comm,"logout") == 0)
        {
            if(pipe(pipe_logout_key) < 0)
            {
                perror("ERROR : pipe(pipe_logout_key) \n");
                exit(1);
            }

            switch(child_logout = fork())
            {
                case -1:
                    perror("ERROR : fork(child_logout)\n");
                    exit(1);

                case 0:
                    close(pipe_logout[1]);
                    read(pipe_logout[0], &key_child, sizeof(key_child));
                    close(socket_logout[0]);
                    Logout(&key_child);
                    close(socket_logout[1]);
                    exit(status);

                default:
                    close(pipe_logout[0]);
                    write(pipe_logout[1], &key, sizeof(key));
                    close(pipe_logout[1]);
                    wait(&status);
                    close(socket_logout[1]);
                    read(socket_logout[0], buff, sizeof(buff));
                    memo = strlen(buff) + 1;
                    send = malloc(memo);

                    if(send == NULL)
                    {
                        perror("ERROR : malloc() \n");
                        exit(1);
                    }

                    printf("Memmory used : %d \n", memo);
                    strcpy(send, buff);
                    write(fd_recive, &memo, sizeof(memo));
                    write(fd_recive, send, memo);
                    free(send);
                    close(socket_logout[0]);
            }
            close(pipe_logout_key[1]);
            read(pipe_logout_key[0], &key, sizeof(bool));
            close(pipe_logout_key[0]);
        }
        else
        {
            sprintf(msg, "Command ERROR \n");
            memo = strlen(msg) + 1;
            send = malloc(memo);

            if (send == NULL)
            {
                perror("ERROR : malloc() \n");
                exit(1);
            }

            strcpy(send, msg);
            write(fd_recive, &memo, sizeof(memo));
            write(fd_recive, send, memo);
            free(send);
        }
        printf("key : %b \n\n", key);
    }

    return 0;
}
