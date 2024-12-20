#include "ChatroomServer.h"

#include <iostream>
int main() {
    LOG_INFO << "ChatroomServer started";
    EventLoop loop;
    InetAddress listenAddr(8888);
    ChatroomServer server(&loop, listenAddr);
    server.start();
    loop.loop();
    return 0;
}