// client.cpp
#include "clienteventthread.h"

#include <iostream>
ClientEventThread::ClientEventThread()
{
}

void ClientEventThread::onConnection(const TcpConnectionPtr& conn)
{
    cout << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN") << endl;
    if (!conn->connected())
        loop_->quit();
}

void ClientEventThread::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp receiveTime)
{
    string msg(buf->retrieveAllAsString());
    std::cout << "clent receive: " << msg  << endl;
    emit cppSigRecv(QString::fromStdString(msg));
}

void ClientEventThread::run()
{
    loop_ = std::make_shared<EventLoop>();
    InetAddress serverAddr(ip, port);
    client_ = std::make_shared<TcpClient>(loop_.get(), serverAddr, "ClientEventThread");
    client_->setConnectionCallback(std::bind(&ClientEventThread::onConnection, this, _1));
    client_->setMessageCallback(std::bind(&ClientEventThread::onMessage, this, _1, _2, _3));
    client_->connect();
    loop_->loop();
    qDebug() << "exit";
}

void ClientEventThread::Start(string ip, int port)
{
    this->ip = ip;
    this->port = port;
    start();
}

void ClientEventThread::Stop()
{
    if(loop_)
        loop_->quit();
}

void ClientEventThread::Send(string data)
{
    client_->connection()->send(data);
}

void ClientEventThread::send_to_server(std::string text)
{
    //dzb TODO
    /* 加密处理 */
    /* 封装 */
    
    Send(text);
}