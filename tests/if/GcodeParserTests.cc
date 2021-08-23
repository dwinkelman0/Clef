// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <gtest/gtest.h>
#include <if/GcodeParser.h>
#include <impl/emulator/Serial.h>

namespace Clef::If {
class GcodeParserTest : public testing::Test {
 public:
  GcodeParserTest()
      : serial_(std::make_shared<std::mutex>()), parser_(serial_) {}

 protected:
  Clef::Impl::Emulator::Serial serial_;
  GcodeParser parser_;
};

TEST_F(GcodeParserTest, Basic) {
  serial_.inject("G1 X80\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "ok\n");
}

TEST_F(GcodeParserTest, Comment) {
  serial_.inject("G1 X80 ;this is a comment\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "ok\n");
}
}  // namespace Clef::If
