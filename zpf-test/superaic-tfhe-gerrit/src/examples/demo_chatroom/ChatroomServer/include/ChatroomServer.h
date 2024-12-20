#include <set>

#include "muduo/net/TcpServer.h"
#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"

using namespace muduo;
using namespace muduo::net;

class ChatroomServer {
public:
    ChatroomServer(EventLoop* loop, const InetAddress& listenAddr);

    void start() {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& conn);

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime);

    TcpServer server_;
    std::set<TcpConnectionPtr> connections_;
};
