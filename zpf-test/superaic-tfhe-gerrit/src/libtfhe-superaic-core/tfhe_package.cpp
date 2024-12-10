#include <iostream>
#include <cassert>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "tfhe.h"
#include "tfhe_package.h"



#define COM_VER  1;

TFHE_Comm_Pack_t * allocate_comm_pack(TFHE_PayloadType_e type,uint32_t payload_size){

    size_t head_size = sizeof(TFHE_Comm_Pack_t);
    size_t package_size = head_size + payload_size;
    TFHE_Comm_Pack_t * pack = (TFHE_Comm_Pack_t *)malloc(package_size);
    if(!pack){
        return NULL;
    }
    memset(pack,0,package_size);
    pack->head = HEAD_MATIC;
    pack->ver = COM_VER;
    pack->payload_type = type;
    pack->payload_size = payload_size;
    return pack;
}
void release_comm_pack(TFHE_Comm_Pack_t * pack){
    free(pack);
}


bool send_proxy_pack(TFHE_Comm_Pack_t &pack, int fd){
#if 0    
    if (!tcpSocket){
        return false;
    }

    size_t buffer_size = get_pack_size(pack);
    size_t send_out = 0;
    size_t index = 0;
    size_t left = buffer_size;
    char * buffer = (char *)&pack;


    while(index < buffer_size){
        char *p = buffer+ index;
        send_out = tcpSocket->write(p,left);
        if(send_out < 0){
            return false;
        }
        index += send_out;
        left -= send_out;
    }
#endif    
    return true;
}


bool receive_until(int fd, char * buffer, size_t total_size){
#if 0    
    if (!tcpSocket){
        return false;
    }

    size_t read_in = 0;
    size_t index = 0;
    size_t left = total_size - index;
    int zero_cnt = 0;

    while(left > 0){
        read_in = tcpSocket->read(buffer+index,left);
        if(read_in == 0){
            zero_cnt ++;
            if(zero_cnt > 10){
                qDebug() << "Get 0 bytes for " <<  zero_cnt << " times, stop!";
                return false;
            }
        }
        if(read_in < 0){
            return false;
        }
        if(read_in != left){
            if(left - read_in > 4 && read_in){
                qDebug() << (int)(*(buffer+index)) << "," << (int)(*(buffer+index + 1)) << "," << (int)(*(buffer+index + 2)) << "," << (int)(*(buffer+index + 3));
            }
            qDebug()  << "read_in is " << read_in << " expect " << left;
        }
        index += read_in;
        left -= read_in;
    }
#endif
    return true;
}


TFHE_Comm_Pack_t * _receive_proxy_pack(int fd){

    TFHE_Comm_Pack_t head;
#if 0
    if (!tcpSocket){
        qWarning() << "tcpSocket is NULL";
        return NULL;
    }
    if(!receive_until(tcpSocket,(char *)&head,sizeof(head))){
        qWarning() << "receive head failed";
        return NULL;
    }
    if(head.head != HEAD_MATIC){
        // TODO: drop dirty data in buffer
        qWarning() << "head.head mismatch with HEAD_MATIC :" << head.head;
        return NULL;
    }

    size_t pack_size = get_pack_size(head);
    qDebug() << "receiving pack size is " << pack_size;

    size_t index = sizeof(head);
    size_t left = pack_size - index;
    ProxyCOMMPACK *pack_buffer = (ProxyCOMMPACK *)malloc(pack_size);
    if(!pack_buffer){
        qWarning() << "malloc pack_buffer failed. pack_size = " << pack_size;
        return NULL;
    }
    memcpy((void *)pack_buffer, (void *) &head,index);

    if(!(receive_until(tcpSocket,((char *)pack_buffer) + index, left ))){
        free(pack_buffer);
        qWarning() << "receive payload failed. payload size = " << left;
        return NULL;
    }
    return pack_buffer;
#endif
    return NULL;    
}



size_t cal_string_payload_size(string &msg){
    return msg.length() + 1; // 预留0的位置
}


/**
 * @brief string_to_buffer 把string类型的数据填入缓冲区
 * @param msg 消息
 * @param buffer 缓冲区
 * @param buffer_size 缓冲区大小
 * @return 使用了多少缓冲区
 */
size_t string_to_buffer(string &msg, unsigned char *buffer, size_t buffer_size){
    strncpy((char *)buffer, msg.c_str(),buffer_size);
    return strlen((char *)buffer) + 1;
}


TFHE_Payload::TFHE_Payload(TFHE_PayloadType_e payload_type)
    : payload_type(payload_type),payload_size(0)
{}



size_t TFHE_Payload::get_buffer_size()
{
    return get_pack_head_size() + payload_size;
}

size_t TFHE_Payload::get_payload_size()
{
    return payload_size;
}

size_t TFHE_Payload::de_serialize_payload(unsigned char *buffer, size_t buffer_size)
{
    return buffer_size;
}

/////////////////////////////////////////////////////////////////////////////


bool send_payload(shared_ptr<TFHE_Payload> payload,int fd){
    if(!payload) return false;

    size_t payload_size = payload->get_payload_size();
    TFHE_Comm_Pack * pack = allocate_comm_pack(payload->payload_type,payload_size);
    payload->serialize_payload(pack->payload,payload_size);
    bool ret = send_proxy_pack(*pack,fd);
    release_comm_pack(pack);
    return ret;

}



shared_ptr<TFHE_Payload> receive_payload(int fd){
    TFHE_Comm_Pack * pack = _receive_proxy_pack(fd);
    if(!pack){
        cerr << "Recieve proxy pack failed ." << endl;
        return NULL;
    }

    shared_ptr<TFHE_Payload> payload = NULL;
    switch(pack->payload_type){
    case TFHE_PayloadType_ACK_OK:{
        payload = shared_ptr<ProxyPayload_ACK_OK>(new ProxyPayload_ACK_OK);
        payload->de_serialize_payload(pack->payload,pack->payload_size);
    }
    break;
#if 0    
    case ProxyPayloadType_ACK_NG:{
        payload = QSharedPointer<ProxyPayload_ACK_NG>(new ProxyPayload_ACK_NG);
        payload->de_serialize_payload(pack->payload,pack->payloadSize);

    }
    break;
    case ProxyPayloadType_QUERY_REPLY:{
        payload = QSharedPointer<ProxyPayload_Query_Reply>(new ProxyPayload_Query_Reply);
        payload->de_serialize_payload(pack->payload,pack->payloadSize);

    }
    break;
    case ProxyPayloadType_Chatroom_Msg:{
        payload = QSharedPointer<ProxyPayload_Chatroom_Msg>(new ProxyPayload_Chatroom_Msg(QString(""),QString("")));
        payload->de_serialize_payload(pack->payload,pack->payloadSize);

    }
    break;
#endif
    default:
        cerr << "receive_payload: Unknow paylod type: " << pack->payload_type << endl;
        break;
    }
    release_comm_pack(pack);

    return payload;
}


/////////////////////////////////////////////////////////////////////////////
ProxyPayload_ACK_OK::ProxyPayload_ACK_OK(string &msg):
    TFHE_Payload(TFHE_PayloadType_ACK_OK),msg(msg)
{
    payload_size = cal_string_payload_size(msg);
}

ProxyPayload_ACK_OK::ProxyPayload_ACK_OK():
    TFHE_Payload(TFHE_PayloadType_ACK_OK)
{
    payload_size = 0;
}


size_t ProxyPayload_ACK_OK::serialize_payload(unsigned char *buffer, size_t buffer_size)
{
    if(!buffer || buffer_size < payload_size) return 0;
    size_t offset = string_to_buffer(msg,buffer, buffer_size);

    return offset;
}

size_t ProxyPayload_ACK_OK::de_serialize_payload(unsigned char *buffer, size_t buffer_size)
{
    msg = (char *)buffer;
    payload_size = cal_string_payload_size(msg);

    return buffer_size;

}
