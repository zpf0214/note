#ifndef CHATROOM_CLIENT_H
#define CHATROOM_CLIENT_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class ChatroomClient; }
QT_END_NAMESPACE

class ChatroomClient : public QMainWindow{
    Q_OBJECT

public:
    ChatroomClient(QWidget *parent = 0);
    ~ChatroomClient(){
        delete ui;
    }

signals:
    void send(std::string text);

protected:
    void resizeEvent(QResizeEvent *event);

private:
    void test_tfhe_interface();

    void on_pushButton_clicked();

    Ui::ChatroomClient *ui;
};

#endif //CHATROOM_CLIENT_H