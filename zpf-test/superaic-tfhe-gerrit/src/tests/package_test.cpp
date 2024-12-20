#include <gtest/gtest.h>
#include <tfhe.h>
#include "tfhe_package.h"

#include "tfhe_superaic_server.h"
#include "test_internal.h"

using namespace std;

namespace {

    class PackageTest : public ::testing::Test {
    };

    TEST_F (PackageTest, strMsgTest) {
        {
            string msg("hello");
            shared_ptr<ProxyPayload_ACK_OK> payload = shared_ptr<ProxyPayload_ACK_OK>(new ProxyPayload_ACK_OK(msg));
            ASSERT_EQ(payload->payload_size,6);
            TFHE_Comm_Pack * pack = allocate_comm_pack(payload->payload_type,payload->payload_size);
            payload->serialize_payload(pack->payload,payload->payload_size);
            ASSERT_EQ(pack->payload_type,TFHE_PayloadType_ACK_OK);
            ASSERT_EQ(pack->payload_size,6);
            ASSERT_EQ(pack->payload[0],'h');
            ASSERT_EQ(pack->payload[1],'e');
            ASSERT_EQ(pack->payload[2],'l');
            ASSERT_EQ(pack->payload[3],'l');
            ASSERT_EQ(pack->payload[4],'o');
            ASSERT_EQ(pack->payload[5],'\0');

            shared_ptr<ProxyPayload_ACK_OK> payload_rsv = shared_ptr<ProxyPayload_ACK_OK>(new ProxyPayload_ACK_OK);
            payload_rsv->de_serialize_payload(pack->payload,pack->payload_size);
            ASSERT_EQ(payload_rsv->payload_size,6);
            pack->payload[0] = 'H';  // 确保使用了2块内存
            ASSERT_EQ(payload_rsv->msg,msg);
            release_comm_pack(pack);

        }
        {
            string msg("hello");
            shared_ptr<ProxyPayload_ACK_OK> payload = shared_ptr<ProxyPayload_ACK_OK>(new ProxyPayload_ACK_OK(msg));
            ASSERT_EQ(payload->payload_size,6);
            std::shared_ptr<TFHE_Comm_Pack>  pack = allocate_comm_pack_shared(payload->payload_type,payload->payload_size);
            payload->serialize_payload(pack->payload,payload->payload_size);
            ASSERT_EQ(pack->payload_type,TFHE_PayloadType_ACK_OK);
            ASSERT_EQ(pack->payload_size,6);
            ASSERT_EQ(pack->payload[0],'h');
            ASSERT_EQ(pack->payload[1],'e');
            ASSERT_EQ(pack->payload[2],'l');
            ASSERT_EQ(pack->payload[3],'l');
            ASSERT_EQ(pack->payload[4],'o');
            ASSERT_EQ(pack->payload[5],'\0');

            shared_ptr<ProxyPayload_ACK_OK> payload_rsv = shared_ptr<ProxyPayload_ACK_OK>(new ProxyPayload_ACK_OK);
            payload_rsv->de_serialize_payload(pack->payload,pack->payload_size);
            ASSERT_EQ(payload_rsv->payload_size,6);
            pack->payload[0] = 'H';  // 确保使用了2块内存
            ASSERT_EQ(payload_rsv->msg,msg);

        }
    }
}