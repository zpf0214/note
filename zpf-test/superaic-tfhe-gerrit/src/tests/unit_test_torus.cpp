#include <stdio.h>

#include "gtest/gtest.h"

#include "tfhe_superaic_torus.h"

TEST(ExampleTest, doubleToTorus32) 
{
    EXPECT_EQ(0x80000000,dtot32(0.5));
    EXPECT_EQ(-0.5,t32tod(0x80000000));
    EXPECT_EQ(0.25,t32tod(0x40000000));
}

TEST(ExampleTest, approxPhase) 
{
    printf("%d\n",approxPhase(17,32));
}


TEST(ExampleTest, AddsTwoNumbers) 
{
    EXPECT_EQ(1 + 2, 3);
}

TEST(ExampleTest, SubtractsTwoNumbers) 
{
    EXPECT_EQ(2 - 1, 1);
}

// int main(){
//     printf("unit_test_torus start\n");

//     return 0;
// }