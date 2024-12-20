// client.h
#ifndef CLIENT_H
#define CLIENT_H

#include <QThread>
#include <QDebug>
#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpClient.h"

using namespace muduo;
using namespace muduo::net;
using namespace std;

class ClientEventThread : public QThread
{
    Q_OBJECT
public:
    ClientEventThread();
    void connect()
    {
        client_->connect();
    }
    void Start(string ip="127.0.0.1", int port=8888);
    void Stop();
    void Send(string data);

public slots:
    void send_to_server(std::string text);

signals:
    void cppSigRecv(QString data);



private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime);
    void run() override;
    
    std::shared_ptr<EventLoop> loop_;
    std::shared_ptr<TcpClient> client_;

    //EventLoop* loop_ = nullptr;
    //TcpClient *client_ = nullptr;
    string ip;
    int port;

    //TcpClient client_;
    //TcpConnectionPtr connection_;
};

#endif // CLIENT_H