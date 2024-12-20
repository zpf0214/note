#include "ChatroomClient.h"
#include "ui_ChatroomClient.h"

#include <iostream>
ChatroomClient::ChatroomClient(QWidget *parent): 
    QMainWindow(parent),
    ui(new Ui::ChatroomClient)
{
    ui->setupUi(this);
    resize(600, 800);
}

void ChatroomClient::resizeEvent(QResizeEvent *event){
    Q_UNUSED(event);
    // for(int i = 0; i < ui->listWidget->count(); i++) {
    //     QNChatMessage* messageW = (QNChatMessage*)ui->listWidget->itemWidget(ui->listWidget->item(i));
    //     QListWidgetItem* item = ui->listWidget->item(i);

    //     dealMessage(messageW, item, messageW->text(), messageW->time(), messageW->userType());
    // }
}

void ChatroomClient::test_tfhe_interface()
{
    // dzb TODO 
    // test lib fthe lib's interface

    // encrypt_str
    // decrypt_str
    // has_match

}