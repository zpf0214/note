#include <iostream>
#include <QApplication>

#include "clienteventthread.h"
#include "ChatroomClient.h"

int main(int argc, char *argv[]) {

    QApplication a(argc, argv);

    LOG_INFO << "ChatroomClient started";

    // 客户端收发线程
    ClientEventThread client_thread;
    client_thread.Start();
    
    // 客户端UI
    ChatroomClient client;
    client.show();


    return a.exec();
}