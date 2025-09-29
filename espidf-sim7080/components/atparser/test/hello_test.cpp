#include <gtest/gtest.h>
#include "../include/atparser.hh"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

class NullUart:public iUartHal{
  public:
   void writeChars(const char* data, size_t data_len_bytes) override{
    return;
   }
};

class AtParserTest : public ::testing::Test
{
protected:
    AtParserTest()
    {
        printf("TestFixture constructor\n");
        
    }
    ~AtParserTest()
    {
        printf("TestFixture destructor\n");
    }
     

    void SetUp() override
    {
        printf("Test method setup\n");
        p = new atparser::Parser();
        p->Init();
        uart = new NullUart();
    }
    void TearDown() override
    {
        printf("Test method teardown\n");
        delete p;
    }

    atparser::Parser* p;
    iUartHal* uart;
};

void sendLine(atparser::Parser* p, char* line){
  std::this_thread::sleep_for(200ms);
  p->Parse(line);
}

TEST_F(AtParserTest, InitialTest){
  std::vector<atparser::cAbstactParam *> params;
  auto error=p->WriteCommandAndWaitResult("AT", params, uart);
  EXPECT_EQ(error, atparser::ErrorCode::DTE_NOT_READY);
  char sendback1[] ="RDY";
  p->Parse(sendback1);
  char sendback2[] ="OK";
  std::thread sender (sendLine, p, sendback2);
  error=p->WriteCommandAndWaitResult("AT", params, uart);
  sender.join();
  EXPECT_EQ(error, atparser::ErrorCode::OK);
}

TEST(TestTest, TestTest01){
  EXPECT_EQ(7*6, 42);
}

TEST(UtilsTest, StartsWithTest_Positive1){
  char * next{nullptr};
  char testee[] ="klara und klaus";
  bool result = atparser::starts_with(testee, "klara u", &next);
  EXPECT_TRUE(result);
  EXPECT_STREQ(next, "nd klaus");
}

TEST(UtilsTest, StartsWithTest_Positive2){
  char * next{nullptr};
  char testee[] ="HundKatze";
  bool result = atparser::starts_with(testee, "Hund", &next);
  EXPECT_TRUE(result);
  EXPECT_STREQ(next, "Katze");
}

TEST(UtilsTest, StartsWithTest_Neg1){
  char * next{nullptr};
  char testee[] ="xklara und klaus";
  bool result = atparser::starts_with(testee, "klara u", &next);
  EXPECT_FALSE(result);
  EXPECT_EQ(next, nullptr);
}

TEST(UtilsTest, StartsWithTest_Neg2){
  char * next{nullptr};
  char testee[] ="AB";
  bool result = atparser::starts_with(testee, "ABC", &next);
  EXPECT_FALSE(result);
}