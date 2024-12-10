#include <gtest/gtest.h>

#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"

using namespace muduo;
using namespace muduo::net;

namespace {

    class MuduoTest : public ::testing::Test {
    };

    TEST_F (MuduoTest, compileTest) {
        EventLoop loop;
    }
}