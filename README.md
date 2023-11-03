# Computer-Networks_Laboratory1
First laboratory project 

Develop two applications (named "client" and "server") that communicate with each other based on a protocol that has the following specifications:

- communication is done by executing commands read from the keyboard in the client and executed in the child processes created by the server;
- commands are character strings delimited by new line;
- the answers are strings of bytes prefixed by the length of the answer;
- the result obtained after the execution of any order will be displayed by the client;
- child processes in the server do not communicate directly with the client, but only with the parent process;
- the minimum protocol includes the commands:
      - "login: username" - whose existence is validated by using a configuration file, which contains all the users who have access to the functionalities. The execution of the command will be carried out in a child process from the server;
      - "get-logged-users" - displays information (username, hostname for remote login, time entry was made) about users logged into the operating system (see "man 5 utmp" and "man 3 getutent"). This command will not be able to be executed if the user is not authenticated in the application. The execution of the command will be carried out in a child process from the server;
      - "get-proc-info : pid" - displays information (name, state, ppid, uid, vmsize) about the indicated process (information source: file /proc/<pid>/status). This command will not be able to be executed if the user is not authenticated in the application. The execution of the command will be carried out in a child process from the server;
      - "logout";
      - "quit".
- in the implementation of the commands, no function from the "exec()" family (or another similar one, e.g. popen(), system()...) will be used in order to execute some system commands that offer the respective functionalities;
- communication between processes will be done using at least once each of the following mechanisms that allow communication: pipes, fifos and socketpair.
