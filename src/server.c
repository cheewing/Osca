#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void accept_request(void *);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);

void accept_request(void *from_client)
{
    int client = *(int *)from_client;
    char buf[1024];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    struct stat st;
    int cgi = 0;

    char *query_string = NULL;

    numchars = getline(client, buf, sizeof(buf));

    i = 0; j = 0;

    //对于HTTP报文来说，第一行的内容即为报文的起始行，格式为<method> <request-URL> <version>，
    //每个字段用空白字符相连
    while (!Isspace(buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[j];
        i++; j++;
    }
    method[i] = '\0';

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);
        return NULL;
    }

    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    i = 0;

    // 将method后面的空白字符略过
    while (ISspace(buf[j]) && (j < sizeof(buf)))
        j++;

    // 继续读取request-URL
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
    {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';

    // 如果是GET请求，url可能会带有?,有查询参数
    if (strcasecmp(method, "GET") == 0)
    {
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if (*query_string == '?')
        {
            // 如果带有查询参数，需要执行cgi，解析参数，设置标志位1
            cgi = 1;
            // 将解析参数截取下来
            *query_string = '\0';
            query_string++;
        }
    }

    sprintf(path, "htdocs%s", url);

    // 如果path只是一个目录，默认设置为首页index.html
    if (path[strlen(path) - 1] == '/')
        strcat(pth, "index.html");

    //函数定义:    int stat(const char *file_name, struct stat *buf); 
    //函数说明:    通过文件名filename获取文件信息，并保存在buf所指的结构体stat中 
    //返回值:     执行成功则返回0，失败返回-1，错误代码存于errno（需要include <errno.h>）
    if (stat(path, &st) == -1)
    {
        // 假如访问的网页不存在，则不断地读取剩下的请求头信息，并丢弃即可
        while ((numchars > 0) && strcmp("\n", buf))
            numchars = get_line(client, buf, sizeof(buf));
        
        not_found(client);
    }
    else
    {
        // 如果访问的网页存在则进行处理
        if ((st.st_mode & S_IFMT) == S_IFDIR) // S_IFDIR代表目录
            strcat(path, "/index.html");
        if ((st.st_mode & S_IXUSR) ||
            (st.st_mode & S_IXGRP) ||
            (st.st_mode & S_IXOTH))
            // S_IXUSR: 文件所有者具有可执行权限
            // S_IXGRP: 用户组具有可执行权限
            // S_IXOTH: 其他用户具有可执行权限
            cgi = 1;
        
        if (!cgi)
            // 将静态文件返回
            serve_file(client, path);
        else
            execute_cgi(client, path, method, query_string);
    }

    close(client);
    return NULL;
}

void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your bwowser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

void cat(int client, FILE *resource)
{
    char buf[1024];
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf, resource));
    }
}

void cannot_execute(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

void execute_cgi(int client, const char *path,
                 const char *method, const char *query_string)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A'; buf[1] = '\0';
    if (strcasecmp(method, "GET") == 0)
        // 如果是GET请求，读取并且丢弃头信息
        while ((numchar > 0) && strcmp("\n", buf))
            numchars = get_line(client, buf, sizeof(buf));
    else
    {
        numchars = get_line(client, buf, sizeof(buf));
        while ((numchars > 0) && strcmp("\n", buf))
        {
            // 循环读取头信息找到Content-Length字段的值
            buf[15] = '\0';

            if (strcasecmp(buf, "Content-Length:") == 0)
                // Content-Length: 15
                content_length = atoi(&(buf[16]));
            numchars = get_line(client, buf, sizeof(buf));
        }

        if (content_length == -1)
        {
            bad_request(client);
            return;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);

    if (pipe(cgi_output) < 0) 
    {
        cannot_execute(client);
        return ;
    }
    
    if (pipe(cgi_input) < 0)
    {
        cannot_execute(client);
        return ;
    }

    if ((pid = fork()) < 0)
    {
        cannot_execute(client);
        return ;
    }

    // fork出子进程运行cgi脚本
    if (pid == 0) 
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1], 1);
        dup2(cgi_input[0], 0); // 将系统标准输入重定向委cgi_input[0], cgi程序中用的是标准输入输出进行交互

        close(cgi_output[0]);
        close(cgi_input[1]);

        // CGI标准需要将请求的方法存储在环境变量中，然后和cgi脚本进行交互
        // 存储REQUEST_METHOD
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0)
        {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else 
        {
            sprintf(length_env, "CONTENT_LENGTH=%d", conteng_length);
            putenv(length_env);
        }

        // 表头文件#include<unistd.h>
        // 定义函数
        // int execl(const char * path,const char * arg,....);
        // 函数说明
        // execl()用来执行参数path字符串所代表的文件路径，接下来的参数代表执行该文件时传递过去的argv(0)、argv[1]……，最后一个参数必须用空指针(NULL)作结束。
        // 返回值
        // 如果执行成功则函数不会返回，执行失败则直接返回-1，失败原因存于errno中。
        execl(path, path, NULL); // 执行CGI脚本
        exit(0);
    }
    else
    {
        close(cgi_output[1]);
        close(cgi_input[0]);

        if (strcasecmp(method, "POST") == 0)
            for (i = 0; i < content_length; i++)
            {
                // 开始读区POST中的内容
                recv(client, &c, 1, 0);
                // 将数据发送给cgi脚本
                write(cgi_input[1], &c, 1);
            }
        
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);

        close(cgi_output[0]);
        close(cgi_input[1]);

        //定义函数：pid_t waitpid(pid_t pid, int * status, int options);
        //函数说明：waitpid()会暂时停止目前进程的执行, 直到有信号来到或子进程结束. 
        //如果在调用wait()时子进程已经结束, 则wait()会立即返回子进程结束状态值. 子进程的结束状态值会由参数status 返回, 
        //而子进程的进程识别码也会一快返回. 
        //如果不在意结束状态值, 则参数status 可以设成NULL. 参数pid 为欲等待的子进程识别码, 其他数值意义如下：
        //1、pid<-1 等待进程组识别码为pid 绝对值的任何子进程.
        //2、pid=-1 等待任何子进程, 相当于wait().
        //3、pid=0 等待进程组识别码与目前进程相同的任何子进程.
        //4、pid>0 等待任何子进程识别码为pid 的子进程.
        waitpid(pid, &status, 0);
    }
}

void error_die(const char *sc)
{
    perror(sc);
    exit(1);
}

int get_line(int sock, char *buf, int size)
{
    int i = 0; 
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        if (n > 0)
        {
            if (c == '\r') // 换行符
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n')) // 如果是\r\n
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return i;
}

void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */
    //发送HTTP头
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

void not_found(int client)
{
    char buf[1024];
    //返回404
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];
    buf[0] = 'A'; buf[1] = '\0';

    while ((numchars > 0) && strcmp("\n", buf))
        numchars = get_line(client, buf, sizeof(buf));
    
    resource = fopen(filename, "r");
    if (resource == NULL)
        not_found(client);
    else
    {
        headers(client, filename);
        cat(client, resource);
    }

    fclose(resource);
}

int startup(ushort *port)
{
    int httpd = 0;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");
    
    if (*port == 0)
    {
        socklen_t namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }

    if (listen(httpd, 5) < 0)
        error_die("listen");
    
    return httpd;
}

void unimplemented(int client)
{
 char buf[1024];
//发送501说明相应方法没有实现
 sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</TITLE></HEAD>\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</BODY></HTML>\r\n");
 send(client, buf, strlen(buf), 0);
}

int main(int argc, char *argv[])
{
    int server_sock = -1;
    u_short port = 0;
    int client_sock = -1;
    struct sockaddr_in client_name;
    socklen_t client_name_len = sizeof(client_name);
    pthread_t newthread;

    server_sock = startup(&port);

    while (1)
    {
        client_sock = accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);
        if (client_sock == -1)
        error_die("accept");

        if (pthread_create(&newthread, NULL, accept_request, (void *)&client_sock) != 0)
            perror("pthread_create");
    }

    close(server_sock);

    return 0;
}