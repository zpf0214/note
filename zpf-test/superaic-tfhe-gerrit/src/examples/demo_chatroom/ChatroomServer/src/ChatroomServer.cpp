#include "ChatroomServer.h"

#include <iostream>

using namespace muduo;
using namespace muduo::net;

ChatroomServer::ChatroomServer(EventLoop* loop, const InetAddress& listenAddr)
    : server_(loop, listenAddr, "ChatroomServer") {
    server_.setConnectionCallback(std::bind(&ChatroomServer::onConnection, this, _1));
    server_.setMessageCallback(std::bind(&ChatroomServer::onMessage, this, _1, _2, _3));
    server_.setThreadNum(4);
}

void ChatroomServer::onConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << "ChatroomServer - " << conn->peerAddress().toIpPort() << " -> "
                << conn->localAddress().toIpPort() << " is "
                << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected()) {
        connections_.insert(conn);
    } else {
        connections_.erase(conn);
    }
}

void ChatroomServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime) {
    std::string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " recv " << msg.size() << " bytes at " << receiveTime.toFormattedString();
    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
        if (*it != conn) {
            (*it)->send(msg);
        }
    }
}
