#ifndef TFHE_PACKAGE_H
#define TFHE_PACKAGE_H
#include <memory>

#define HEAD_MATIC 0x1234
using namespace std;
typedef enum TFHE_PayloadType_{
    TFHE_PayloadType_Greeting = 0,
    TFHE_PayloadType_ACK_OK = 1,
    TFHE_PayloadType_ACK_NG = 2 ,
    TFHE_PayloadType_SEND_BT_KEY_STRING = 3,  
    TFHE_PayloadType_QUERY_BT_KEY_ID = 4,     // 发送本地bootstrap key的hash code，如果服务器有相同的hash code的boot strap key，就返回对应的 sesstion ID，否则返回 ACK_NG
    TFHE_PayloadType_SESSION_ID = 5 ,         // 发送bootstrap key 对应的session ID，在客户端每次向服务器发起请求的时候，都要提供sesstion ID 让服务器知道使用哪个bootstrap key
}TFHE_PayloadType_e;


typedef struct __attribute__ ((packed)) TFHE_Comm_Pack{
    uint32_t head;
    uint16_t ver;
    uint32_t payload_size;
    uint16_t payload_type;
    uint8_t  payload[];
}TFHE_Comm_Pack_t;


inline size_t get_pack_head_size(void){return sizeof(TFHE_Comm_Pack);}
inline size_t get_pack_size(TFHE_Comm_Pack &pack){return pack.payload_size + get_pack_head_size();}
inline size_t cal_pack_size(size_t payload_size){return payload_size + get_pack_head_size();};


TFHE_Comm_Pack_t * allocate_comm_pack(TFHE_PayloadType_e type,uint32_t payload_size);
void release_comm_pack(TFHE_Comm_Pack_t * pack);
bool send_proxy_pack(TFHE_Comm_Pack_t &pack, int fd);



class TFHE_Payload 
{
public:
    explicit TFHE_Payload(TFHE_PayloadType_e payload_type);
    TFHE_Payload(const TFHE_Payload&) = delete; //forbidden
    TFHE_Payload* operator=(const TFHE_Payload&) = delete; //forbidden

    const TFHE_PayloadType_e payload_type;
    uint32_t payload_size;

    /**
     * @brief get_buffer_size 用于计算需要多少字节的缓冲区。缓冲区的大小为包头+palyload_size
     * @return 缓冲区大小
     */
    size_t  get_buffer_size(void) ;

    /**
     * @brief get_payload_size 用于计算payload需要多少字节的缓冲区。
     * @return 缓冲区大小
     */
    size_t get_payload_size(void) ;

    /**
     * @brief serialize_payload payload序列化到缓冲区
     * @param buffer 缓冲区
     * @param buffer_size 缓冲区大小
     * @return 写入缓冲区的字节数
     */
    virtual size_t serialize_payload(unsigned char * buffer, size_t buffer_size) = 0;

    /**
     * @brief de_serialize_payload 反序列化。默认的行为是直接丢掉数据。
     * @param buffer
     * @param buffer_size
     * @return
     */
    virtual size_t de_serialize_payload(unsigned char * buffer, size_t buffer_size);

};



class ProxyPayload_ACK_OK: public TFHE_Payload {
public:
    explicit ProxyPayload_ACK_OK();
    explicit ProxyPayload_ACK_OK(string &msg);
    string msg;

    virtual size_t serialize_payload(unsigned char * buffer, size_t buffer_size);
    virtual size_t de_serialize_payload(unsigned char * buffer, size_t buffer_size);

};







#endif //TFHE_PACKAGE_H
