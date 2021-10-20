// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "IntegrationFixture.h"

namespace Clef::Fw {
class GcodeParserTest : public IntegrationFixture {
 public:
  GcodeParserTest() : IntegrationFixture() {}

  /**
   * Send a minimal valid G-Code command.
   */
  void doBasic() {
    XYZEPosition startPosition = actionQueue_.getEndPosition();
    serial_.inject("G1 X80\n");
    parser_.ingest(context_);
    checkBasic(startPosition);
  }

  /**
   * Perform checks and cleanup for doBasic().
   */
  void checkBasic(XYZEPosition origStartPosition) {
    ASSERT_EQ(serial_.extract(), "ok\n");
    ASSERT_EQ(actionQueue_.size(), 1);
    ASSERT_TRUE(actionQueue_.checkConservation());
    ActionQueue::Iterator it = actionQueue_.first();
    ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XY);
    XYZEPosition endPosition = (*it)->getEndPosition();
    ASSERT_EQ(*endPosition.x, 80);
    ASSERT_EQ(endPosition.y, origStartPosition.y);
    ASSERT_EQ(endPosition.z, origStartPosition.z);
    ASSERT_EQ(endPosition.e, origStartPosition.e);
    actionQueue_.pop(context_);
    ASSERT_TRUE(actionQueue_.checkConservation());
  }
};

TEST_F(GcodeParserTest, Basic) { doBasic(); }

TEST_F(GcodeParserTest, Comments) {
  XYZEPosition startPosition = actionQueue_.getEndPosition();
  serial_.inject(";this comment is a whole line\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "");
  serial_.inject("G1 X80 ;this is a comment\n");
  parser_.ingest(context_);
  checkBasic(startPosition);
  doBasic();
  serial_.inject("G1 X80 this is not a comment\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(),
            std::string(Str::INVALID_CODE_LETTER_ERROR) + ": t\n");
  serial_.inject("G1 X80 ;another comment\n");
  parser_.ingest(context_);
  checkBasic(startPosition);
  doBasic();
}

TEST_F(GcodeParserTest, BufferOverflow) {
  for (char c = 'A'; c < 'Z'; ++c) {
    serial_.inject(&c);
    serial_.inject("32689 ");
  }
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), std::string(Str::BUFFER_OVERFLOW_ERROR) + "\n");
  doBasic();
}

TEST_F(GcodeParserTest, DuplicateCodeLetter) {
  serial_.inject("G1 G1\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(),
            std::string(Str::DUPLICATE_CODE_LETTER_ERROR) + ": G\n");
  doBasic();
}

TEST_F(GcodeParserTest, MissingCommandCode) {
  serial_.inject("G\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(),
            std::string(Str::MISSING_COMMAND_CODE_ERROR) + "\n");
  doBasic();
}

TEST_F(GcodeParserTest, InvalidGCode) {
  serial_.inject("G888\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(),
            std::string(Str::INVALID_G_CODE_ERROR) + ": 888\n");
  doBasic();
}

TEST_F(GcodeParserTest, ParseFloat) {
  serial_.inject("G1 X80.5\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XY);
  XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_FLOAT_EQ(*endPosition.x, 80.5);
  actionQueue_.pop(context_);
}

TEST_F(GcodeParserTest, G1_XY) {
  serial_.inject("G1 X40 Y30\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XY);
  XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, 40);
  ASSERT_EQ(*endPosition.y, 30);
  actionQueue_.pop(context_);
}

TEST_F(GcodeParserTest, G1_XYE) {
  // Send a first XYE point
  serial_.inject("G1 X40 E2\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ASSERT_EQ(actionQueue_.size(), 1);
  ActionQueue::Iterator it = actionQueue_.last();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XYE);
  XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, 40);
  ASSERT_EQ(*endPosition.y, 0);
  ASSERT_EQ(*endPosition.e, 2);
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it)->getNumPointsPushed(), 1);
  ASSERT_EQ(context_.xyePositionQueue.size(), 1);
  XYEPosition xyePosition1 = *context_.xyePositionQueue.last();
  ASSERT_EQ(*xyePosition1.x, 40);
  ASSERT_EQ(*xyePosition1.y, 0);
  ASSERT_EQ(*xyePosition1.e, 2);

  // Send a second XYE point (the E coordinates are very close together)
  serial_.inject("G1 X80 Y60 E2.00002\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ASSERT_EQ(actionQueue_.size(), 1);
  endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, 80);
  ASSERT_EQ(*endPosition.y, 60);
  ASSERT_FLOAT_EQ(*endPosition.e, 2.00002);
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it)->getNumPointsPushed(), 2);
  ASSERT_EQ(context_.xyePositionQueue.size(), 2);
  XYEPosition xyePosition2 = *context_.xyePositionQueue.last();
  ASSERT_EQ(*xyePosition2.x, 80);
  ASSERT_EQ(*xyePosition2.y, 60);
  ASSERT_FLOAT_EQ(*xyePosition2.e, 2.00002);

  // Send a non-XYE point
  serial_.inject("G1 X33 Y44\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ASSERT_EQ(actionQueue_.size(), 2);
  it = actionQueue_.last();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XY);
  endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, 33);
  ASSERT_EQ(*endPosition.y, 44);
  ASSERT_FLOAT_EQ(*endPosition.e, 2.00002);
  ASSERT_EQ(context_.xyePositionQueue.size(), 2);

  // Send a third XYE point in a different segment
  serial_.inject("G1 X30 Y30 E6\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  it = actionQueue_.last();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XYE);
  ASSERT_EQ(actionQueue_.size(), 3);
  endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, 30);
  ASSERT_EQ(*endPosition.y, 30);
  ASSERT_EQ(*endPosition.e, 6);
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it)->getNumPointsPushed(), 1);
  ASSERT_EQ(context_.xyePositionQueue.size(), 3);
  XYEPosition xyePosition3 = *context_.xyePositionQueue.last();
  ASSERT_EQ(*xyePosition3.x, 30);
  ASSERT_EQ(*xyePosition3.y, 30);
  ASSERT_EQ(*xyePosition3.e, 6);

  // Clean up
  actionQueue_.pop(context_);
  ASSERT_EQ(*actionQueue_.getStartPosition().x, 80);
  ASSERT_EQ(*actionQueue_.getStartPosition().y, 60);
  actionQueue_.pop(context_);
  ASSERT_EQ(*actionQueue_.getStartPosition().x, 33);
  ASSERT_EQ(*actionQueue_.getStartPosition().y, 44);
  actionQueue_.pop(context_);
  ASSERT_EQ(*actionQueue_.getStartPosition().x, 30);
  ASSERT_EQ(*actionQueue_.getStartPosition().y, 30);
  context_.xyePositionQueue.pop();
  context_.xyePositionQueue.pop();
  context_.xyePositionQueue.pop();
}

TEST_F(GcodeParserTest, G1_XYE_Aliasing) {
  serial_.inject("G1 X40 Y30 E2\n");
  serial_.inject("G1 X40 Y30 E2\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\nok\n");
  ASSERT_EQ(actionQueue_.size(), 1);
  ActionQueue::Iterator it = actionQueue_.last();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XYE);
  XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, 40);
  ASSERT_EQ(*endPosition.y, 30);
  ASSERT_EQ(*endPosition.e, 2);
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it)->getNumPointsPushed(), 1);
  ASSERT_EQ(context_.xyePositionQueue.size(), 1);
  XYEPosition xyePosition1 = *context_.xyePositionQueue.last();
  ASSERT_EQ(*xyePosition1.x, 40);
  ASSERT_EQ(*xyePosition1.y, 30);
  ASSERT_EQ(*xyePosition1.e, 2);
}

TEST_F(GcodeParserTest, G1_XYE_Direction) {
  serial_.inject("G1 X40 Y30 E2\n");
  serial_.inject("G1 X40 Y30 E3\n");
  serial_.inject("G1 X40 Y30 E2\n");
  serial_.inject("G1 X40 Y30 E3\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\nok\nok\nok\n");
  ASSERT_EQ(actionQueue_.size(), 3);
  ActionQueue::Iterator it1 = actionQueue_.first();
  ASSERT_EQ((*it1)->getType(), Action::Type::MOVE_XYE);
  XYZEPosition endPosition = (*it1)->getEndPosition();
  ASSERT_EQ(*endPosition.x, 40);
  ASSERT_EQ(*endPosition.y, 30);
  ASSERT_EQ(*endPosition.e, 3);
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it1)->getNumPointsPushed(), 2);
  ActionQueue::Iterator it2 = actionQueue_.first().next();
  ASSERT_EQ((*it2)->getType(), Action::Type::MOVE_XYE);
  endPosition = (*it2)->getEndPosition();
  ASSERT_EQ(*endPosition.x, 40);
  ASSERT_EQ(*endPosition.y, 30);
  ASSERT_EQ(*endPosition.e, 2);
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it2)->getNumPointsPushed(), 1);
  ActionQueue::Iterator it3 = actionQueue_.first().next().next();
  ASSERT_EQ((*it3)->getType(), Action::Type::MOVE_XYE);
  ASSERT_EQ((*it3)->getEndPosition(), (*it1)->getEndPosition());
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it3)->getNumPointsPushed(), 1);
  ASSERT_EQ(context_.xyePositionQueue.size(), 4);
}

TEST_F(GcodeParserTest, G1_E) {
  serial_.inject("G1 E5\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_E);
  XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.e, 5);
  actionQueue_.pop(context_);
}

TEST_F(GcodeParserTest, G1_Z) {
  serial_.inject("G1 Z-10\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_Z);
  XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.z, -10);
  actionQueue_.pop(context_);
}

TEST_F(GcodeParserTest, G1_F) {
  serial_.inject("G1 X80 F3000\n");
  parser_.ingest(context_);
  ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Action::Type::SET_FEEDRATE);
  ASSERT_EQ((*it)->getEndPosition(), (XYZEPosition{0, 0, 0, 0}));
  actionQueue_.pop(context_);
  checkBasic(XYZEPosition{0, 0, 0, 0});
}

TEST_F(GcodeParserTest, UndefinedCodeLetter) {
  serial_.inject("G1 X\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(),
            std::string(Str::UNDEFINED_CODE_LETTER_ERROR) + ": X\n");
  doBasic();
}

TEST_F(GcodeParserTest, InsufficientQueueCapacity) {
  // Test limits of action buffer
  bool broken = false;
  for (int i = 0; i < 1000; ++i) {
    serial_.inject("G1 X80\n");
    parser_.ingest(context_);
    std::string result = serial_.extract();
    if (result != "ok\n") {
      ASSERT_EQ(result,
                std::string(Str::INSUFFICIENT_QUEUE_CAPACITY_ERROR) + "\n");
      broken = true;
      break;
    }
  }
  while (actionQueue_.size() > 0) {
    actionQueue_.pop(context_);
  }
  ASSERT_TRUE(broken);

  // Test limits of xyePosition buffer
  broken = false;
  for (int i = 0; i < 1000; ++i) {
    serial_.inject("G1 X80 E" + std::to_string(i) + "\n");
    parser_.ingest(context_);
    std::string result = serial_.extract();
    if (result != "ok\n") {
      ASSERT_EQ(result,
                std::string(Str::INSUFFICIENT_QUEUE_CAPACITY_ERROR) + "\n");
      broken = true;
      break;
    }
  }
  while (context_.xyePositionQueue.size() > 0) {
    context_.xyePositionQueue.pop();
  }
  actionQueue_.pop(context_);
  ASSERT_TRUE(broken);
}
}  // namespace Clef::Fw
