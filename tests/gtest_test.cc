#pragma once


#include "gtest/gtest.h"


int positive(int n) {
	if (n > 0)
		return 0;
	return -1;
}


TEST(positive, HandlesZeroInput) {
	EXPECT_EQ(positive(0), -1);
}


TEST(positive, HandlesPosInput) {
	EXPECT_EQ(positive(2), 0);
}


TEST(positive, HandlesNegInput) {
	EXPECT_EQ(positive(-2), -1);
}