// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/GcodeParser.h>
#include <gtest/gtest.h>
#include <impl/emulator/Serial.h>

namespace Clef::Fw {
class GcodeParserTest : public testing::Test {
 public:
  GcodeParserTest()
      : serial_(std::make_shared<std::mutex>()), parser_(serial_) {}

  /**
   * Send a minimal valid G-Code command.
   */
  void doBasic() {
    serial_.inject("G1 X80\n");
    parser_.ingest();
    ASSERT_EQ(serial_.extract(), "ok\n");
  }

 protected:
  Clef::Impl::Emulator::Serial serial_;
  GcodeParser parser_;
};

TEST_F(GcodeParserTest, Basic) { doBasic(); }

TEST_F(GcodeParserTest, Comments) {
  serial_.inject(";this comment is a whole line\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "");
  serial_.inject("G1 X80 ;this is a comment\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), std::string(Str::OK) + "\n");
  doBasic();
  serial_.inject("G1 X80 this is not a comment\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(),
            std::string(Str::INVALID_CODE_LETTER_ERROR) + ": t\n");
  serial_.inject("G1 X80 ;another comment\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), std::string(Str::OK) + "\n");
  doBasic();
}

TEST_F(GcodeParserTest, BufferOverflow) {
  for (char c = 'A'; c < 'Z'; ++c) {
    serial_.inject(&c);
    serial_.inject("32689 ");
  }
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), std::string(Str::BUFFER_OVERFLOW_ERROR) + "\n");
  doBasic();
}

TEST_F(GcodeParserTest, DuplicateCodeLetter) {
  serial_.inject("G1 G1\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(),
            std::string(Str::DUPLICATE_CODE_LETTER_ERROR) + ": G\n");
  doBasic();
}

TEST_F(GcodeParserTest, MissingCommandCode) {
  serial_.inject("G\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(),
            std::string(Str::MISSING_COMMAND_CODE_ERROR) + "\n");
  doBasic();
}

TEST_F(GcodeParserTest, InvalidGCode) {
  serial_.inject("G888\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(),
            std::string(Str::INVALID_G_CODE_ERROR) + ": 888\n");
  doBasic();
}
}  // namespace Clef::Fw
