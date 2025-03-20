# 端口、地址重用机制

# 原理
> 设置socket为SO_REUSEPORT以及SO_REUSEADDR，实现多进程或者多线程下对同一地址端口监听，提供相同服务，并且能够在服务器崩溃后快速重启，避免等待TIME_WAIT状态结束。可以基于此实现多线程、多进程同时在相同地址端口提供相同服务，还能实现不停服升级。

# 多进程端口重用
> 多进程可以各自创建监听socket，或者多进程共享相同的监听socket，均能提供相同服务，内核自动提供负载均衡，当有连接到来，内核选择唤醒某个进程（无惊群效应），不同点在于共享监听socket，多进程就共享同一连接队列，并且通常只唤醒第一个accept的进程，子进程过多会导致竞争，各自创建则各自有一个连接队列，有连接时根据负载均衡选择一个进程的连接队列放进去。

```cpp
// 多进程各自创建监听端口
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <cstring>
#include <csignal>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_EVENTS 100
#define PORT 8090

volatile sig_atomic_t stop = 0;

void handle_signal(int signum) {
    stop = 1;
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void worker_process() {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 10)) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    set_nonblocking(sockfd);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev)) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];
    while (!stop) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == sockfd) {
                int client_fd = accept(sockfd, NULL, NULL);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }
                set_nonblocking(client_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev)) {
                    perror("epoll_ctl");
                    close(client_fd);
                }
            } else {
                char buffer[1024];
                ssize_t len = read(events[i].data.fd, buffer, sizeof(buffer));
                if (len <= 0) {
                    close(events[i].data.fd);
                } else {
                    write(events[i].data.fd, buffer, len);
                }
            }
        }
    }

    close(sockfd);
    close(epoll_fd);
    exit(EXIT_SUCCESS);
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    const int num_workers = 4;
    pid_t pids[num_workers];

    for (int i = 0; i < num_workers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            worker_process();
        } else {
            pids[i] = pid;
        }
    }

    while (!stop) {
        sleep(1);
    }

    for (int i = 0; i < num_workers; i++) {
        kill(pids[i], SIGTERM);
        waitpid(pids[i], nullptr, 0);
    }

    std::cout << "Server stopped gracefully." << std::endl;
    return 0;
}
```

# 多线程共同监听
> 多线程共享epoll fd，epoll在多线程中是安全的，不能在多进程中共享。

```cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <cstring>
#include <thread>
#include <vector>
#include <fcntl.h>

#define MAX_EVENTS 100
#define PORT 8090

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void worker_thread(int epoll_fd, int sockfd) {
    struct epoll_event events[MAX_EVENTS];
    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == sockfd) {
                    // 处理新连接
                    int client_fd = accept(events[i].data.fd, NULL, NULL);
                    if (client_fd == -1) {
                        perror("accept");
                        continue;
                    }
                    set_nonblocking(client_fd);
                    struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev)) {
                        perror("epoll_ctl");
                        close(client_fd);
                    }
                } else {
                    // 处理客户端数据
                    char buffer[1024];
                    ssize_t len = read(events[i].data.fd, buffer, sizeof(buffer));
                    if (len <= 0) {
                        close(events[i].data.fd);
                    } else {
                        write(events[i].data.fd, buffer, len);
                    }
                }
            }
        }
    }
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr))) {
        perror("bind");
        return 1;
    }

    if (listen(sockfd, 10)) {
        perror("listen");
        return 1;
    }

    set_nonblocking(sockfd);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return 1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev)) {
        perror("epoll_ctl");
        return 1;
    }

    const int num_threads = 4;
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(worker_thread, epoll_fd, sockfd);
    }

    for (auto& t : threads) {
        t.join();
    }

    close(sockfd);
    close(epoll_fd);
    return 0;
}
```

# 总结
> epoll fd只能在多线程共享，socket fd可以在多线程多进程下共享，在地址端口重用的情况下，两者都可以在多线程多进程下不共享。在epoll fd共享的情况下，内核可能总是唤醒第一个调epoll_wait的线程，在socket fd共享的情况下，多进程多线程共享相同的连接队列，内核可能总是唤醒第一个调accept的线程或进程。

[返回主页](../../README.md)
