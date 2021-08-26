// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/GcodeParser.h>
#include <gtest/gtest.h>
#include <impl/emulator/Serial.h>

namespace Clef::Fw {
class GcodeParserTest : public testing::Test {
 public:
  GcodeParserTest()
      : serial_(std::make_shared<std::mutex>()),
        parser_(serial_, actionQueue_, xyePositionQueue_) {}

  /**
   * Send a minimal valid G-Code command.
   */
  void doBasic() {
    Clef::Fw::XYZEPosition startPosition = actionQueue_.getEndPosition();
    serial_.inject("G1 X80\n");
    parser_.ingest();
    checkBasic(startPosition);
  }

  /**
   * Perform checks and cleanup for doBasic().
   */
  void checkBasic(Clef::Fw::XYZEPosition origStartPosition) {
    ASSERT_EQ(serial_.extract(), "ok\n");
    ASSERT_EQ(actionQueue_.size(), 1);
    Clef::Fw::ActionQueue::Iterator it = actionQueue_.first();
    ASSERT_EQ((*it)->getType(), Clef::Fw::Action::Type::MOVE_XY);
    Clef::Fw::XYZEPosition endPosition = (*it)->getEndPosition();
    ASSERT_EQ(*endPosition.x, Clef::If::XAxis::UstepsPerMm * 80);
    ASSERT_EQ(endPosition.y, origStartPosition.y);
    ASSERT_EQ(endPosition.z, origStartPosition.z);
    ASSERT_EQ(endPosition.e, origStartPosition.e);
    actionQueue_.pop();
  }

 protected:
  Clef::Impl::Emulator::Serial serial_;
  Clef::Fw::ActionQueue actionQueue_;
  Clef::Fw::XYEPositionQueue xyePositionQueue_;
  GcodeParser parser_;
};

TEST_F(GcodeParserTest, Basic) { doBasic(); }

TEST_F(GcodeParserTest, Comments) {
  Clef::Fw::XYZEPosition startPosition = actionQueue_.getEndPosition();
  serial_.inject(";this comment is a whole line\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "");
  serial_.inject("G1 X80 ;this is a comment\n");
  parser_.ingest();
  checkBasic(startPosition);
  doBasic();
  serial_.inject("G1 X80 this is not a comment\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(),
            std::string(Str::INVALID_CODE_LETTER_ERROR) + ": t\n");
  serial_.inject("G1 X80 ;another comment\n");
  parser_.ingest();
  checkBasic(startPosition);
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

TEST_F(GcodeParserTest, ParseFloat) {
  serial_.inject("G1 X80.5\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "ok\n");
  Clef::Fw::ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Clef::Fw::Action::Type::MOVE_XY);
  Clef::Fw::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_FLOAT_EQ(*endPosition.x, Clef::If::XAxis::UstepsPerMm * 80.5);
  actionQueue_.pop();
}

TEST_F(GcodeParserTest, G1_XY) {
  serial_.inject("G1 X40 Y30\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "ok\n");
  Clef::Fw::ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Clef::Fw::Action::Type::MOVE_XY);
  Clef::Fw::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, Clef::If::XAxis::UstepsPerMm * 40);
  ASSERT_EQ(*endPosition.y, Clef::If::YAxis::UstepsPerMm * 30);
  actionQueue_.pop();
}

TEST_F(GcodeParserTest, G1_XYE) {
  // Send a first XYE point
  serial_.inject("G1 X40 Y30 E2\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "ok\n");
  ASSERT_EQ(actionQueue_.size(), 1);
  Clef::Fw::ActionQueue::Iterator it = actionQueue_.last();
  ASSERT_EQ((*it)->getType(), Clef::Fw::Action::Type::MOVE_XYE);
  Clef::Fw::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, Clef::If::XAxis::UstepsPerMm * 40);
  ASSERT_EQ(*endPosition.y, Clef::If::YAxis::UstepsPerMm * 30);
  ASSERT_EQ(*endPosition.e, Clef::If::EAxis::UstepsPerMm * 2);
  ASSERT_EQ(it->getVariant().moveXye.getNumPoints(), 1);
  ASSERT_EQ(xyePositionQueue_.size(), 1);
  Clef::Fw::XYEPosition xyePosition1 = *xyePositionQueue_.last();
  ASSERT_EQ(*xyePosition1.x, Clef::If::XAxis::UstepsPerMm * 40);
  ASSERT_EQ(*xyePosition1.y, Clef::If::YAxis::UstepsPerMm * 30);
  ASSERT_EQ(*xyePosition1.e, Clef::If::EAxis::UstepsPerMm * 2);

  // Send a second XYE point
  serial_.inject("G1 X80 Y60 E4\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "ok\n");
  ASSERT_EQ(actionQueue_.size(), 1);
  endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, Clef::If::XAxis::UstepsPerMm * 80);
  ASSERT_EQ(*endPosition.y, Clef::If::YAxis::UstepsPerMm * 60);
  ASSERT_EQ(*endPosition.e, Clef::If::EAxis::UstepsPerMm * 4);
  ASSERT_EQ(it->getVariant().moveXye.getNumPoints(), 2);
  ASSERT_EQ(xyePositionQueue_.size(), 2);
  Clef::Fw::XYEPosition xyePosition2 = *xyePositionQueue_.last();
  ASSERT_EQ(*xyePosition2.x, Clef::If::XAxis::UstepsPerMm * 80);
  ASSERT_EQ(*xyePosition2.y, Clef::If::YAxis::UstepsPerMm * 60);
  ASSERT_EQ(*xyePosition2.e, Clef::If::EAxis::UstepsPerMm * 4);

  // Send a non-XYE point
  serial_.inject("G1 X0 Y0\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "ok\n");
  ASSERT_EQ(actionQueue_.size(), 2);
  it = actionQueue_.last();
  ASSERT_EQ((*it)->getType(), Clef::Fw::Action::Type::MOVE_XY);
  endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, 0);
  ASSERT_EQ(*endPosition.y, 0);
  ASSERT_EQ(*endPosition.e, Clef::If::EAxis::UstepsPerMm * 4);
  ASSERT_EQ(xyePositionQueue_.size(), 2);

  // Send a third XYE point in a different segment
  serial_.inject("G1 X30 Y30 E6\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "ok\n");
  it = actionQueue_.last();
  ASSERT_EQ((*it)->getType(), Clef::Fw::Action::Type::MOVE_XYE);
  ASSERT_EQ(actionQueue_.size(), 3);
  endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, Clef::If::XAxis::UstepsPerMm * 30);
  ASSERT_EQ(*endPosition.y, Clef::If::YAxis::UstepsPerMm * 30);
  ASSERT_EQ(*endPosition.e, Clef::If::EAxis::UstepsPerMm * 6);
  ASSERT_EQ(it->getVariant().moveXye.getNumPoints(), 1);
  ASSERT_EQ(xyePositionQueue_.size(), 3);
  Clef::Fw::XYEPosition xyePosition3 = *xyePositionQueue_.last();
  ASSERT_EQ(*xyePosition3.x, Clef::If::XAxis::UstepsPerMm * 30);
  ASSERT_EQ(*xyePosition3.y, Clef::If::YAxis::UstepsPerMm * 30);
  ASSERT_EQ(*xyePosition3.e, Clef::If::EAxis::UstepsPerMm * 6);

  // Clean up
  actionQueue_.pop();
  actionQueue_.pop();
  actionQueue_.pop();
  xyePositionQueue_.pop();
  xyePositionQueue_.pop();
  xyePositionQueue_.pop();
}

TEST_F(GcodeParserTest, G1_E) {
  serial_.inject("G1 E5\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "ok\n");
  Clef::Fw::ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Clef::Fw::Action::Type::MOVE_E);
  Clef::Fw::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.e, Clef::If::EAxis::UstepsPerMm * 5);
  actionQueue_.pop();
}

TEST_F(GcodeParserTest, G1_Z) {
  serial_.inject("G1 Z-10\n");
  parser_.ingest();
  ASSERT_EQ(serial_.extract(), "ok\n");
  Clef::Fw::ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Clef::Fw::Action::Type::MOVE_Z);
  Clef::Fw::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.z,
            static_cast<int32_t>(Clef::If::ZAxis::UstepsPerMm) * -10);
  actionQueue_.pop();
}

TEST_F(GcodeParserTest, G1_F) {
  serial_.inject("G1 X80 F3000\n");
  parser_.ingest();
  Clef::Fw::ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Clef::Fw::Action::Type::SET_FEEDRATE);
  ASSERT_EQ((*it)->getEndPosition(), Clef::Fw::originXyze);
  actionQueue_.pop();
  checkBasic(Clef::Fw::originXyze);
}
}  // namespace Clef::Fw
