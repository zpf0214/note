#include "gtest/gtest.h"

#include "tfhe_superaic_torus.h"
#include "test_internal.h"
TEST(ArithmeticTest, doubleToTorus32) 
{
    EXPECT_EQ(0x80000000,dtot32(0.5));
    EXPECT_EQ(-0.5,t32tod(0x80000000));
    EXPECT_EQ(0.25,t32tod(0x40000000));
    EXPECT_EQ(-0.25,t32tod(-0x40000000));
}


TEST(ArithmeticTest, modeSwitchPos) 
{
    {
        Torus32 source = 0x40000000;
        EXPECT_EQ(0.25,t32tod(source));
        double d = t32tod(source);
        int32_t mSize = 16;
        int32_t result = modSwitchFromTorus32(source,mSize);
        EXPECT_EQ(result,mSize/4);
    }

    {
        Torus32 source = 4;
        int32_t mSize = 16;
        int32_t result = modSwitchToTorus32(source,mSize);
        EXPECT_EQ(result,0x40000000);
    }

}


TEST(ArithmeticTest, modeSwitchNeg) 
{
        int32_t mask = -1;
        mask = mask & 0xfffffff8;

    {
        Torus32 source = dtot32(-0.125);
        printf("source = 0x%x\n",source);
        int32_t mSize = 16;
        int32_t result = modSwitchFromTorus32(source,mSize);
        result |= mask;
        EXPECT_EQ(result,-mSize/8);
    }

    {
        Torus32 source = dtot32(-0.25);
        printf("source = 0x%x\n",source);
        int32_t mSize = 16;
        int32_t result = modSwitchFromTorus32(source,mSize);
        result |= mask;
        printf("result = %d\n",result);
        EXPECT_EQ(result,-mSize / 4);
    }

    {
        Torus32 source = -4;
        int32_t mSize = 16;
        int32_t result = modSwitchToTorus32(source,mSize);
        double fresult = t32tod(result);
        ASSERT_LE(abs(-0.25 - fresult) , 1e-40);
    }

}

