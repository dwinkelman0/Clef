// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/GcodeParser.h>
#include <gtest/gtest.h>
#include <impl/emulator/Clock.h>
#include <impl/emulator/PwmTimer.h>
#include <impl/emulator/Serial.h>
#include <impl/emulator/Stepper.h>

namespace Clef::Fw {
class GcodeParserTest : public testing::Test {
 public:
  GcodeParserTest()
      : globalMutex_(std::make_shared<std::mutex>()),
        clock_(),
        serial_(globalMutex_),
        actionQueue_(),
        parser_(),
        xAxisTimer_(globalMutex_),
        yAxisTimer_(globalMutex_),
        zAxisTimer_(globalMutex_),
        eAxisTimer_(globalMutex_),
        xAxis_(Clef::Impl::Emulator::xAxisStepper, xAxisTimer_),
        yAxis_(Clef::Impl::Emulator::yAxisStepper, yAxisTimer_),
        zAxis_(Clef::Impl::Emulator::zAxisStepper, zAxisTimer_),
        eAxis_(Clef::Impl::Emulator::eAxisStepper, eAxisTimer_),
        axes_(xAxis_, yAxis_, zAxis_, eAxis_),
        context_({axes_, parser_, clock_, serial_, actionQueue_}) {
    clock_.init();
    serial_.init();
  }

  /**
   * Send a minimal valid G-Code command.
   */
  void doBasic() {
    Axes::XYZEPosition startPosition = actionQueue_.getEndPosition();
    serial_.inject("G1 X80\n");
    parser_.ingest(context_);
    checkBasic(startPosition);
  }

  /**
   * Perform checks and cleanup for doBasic().
   */
  void checkBasic(Axes::XYZEPosition origStartPosition) {
    ASSERT_EQ(serial_.extract(), "ok\n");
    ASSERT_EQ(actionQueue_.size(), 1);
    ASSERT_TRUE(actionQueue_.checkConservation());
    ActionQueue::Iterator it = actionQueue_.first();
    ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XY);
    Axes::XYZEPosition endPosition = (*it)->getEndPosition();
    ASSERT_EQ(*endPosition.x, Axes::XAxis::UstepsPerMm * 80);
    ASSERT_EQ(endPosition.y, origStartPosition.y);
    ASSERT_EQ(endPosition.z, origStartPosition.z);
    ASSERT_EQ(endPosition.e, origStartPosition.e);
    actionQueue_.pop(context_);
    ASSERT_TRUE(actionQueue_.checkConservation());
  }

 protected:
  std::shared_ptr<std::mutex> globalMutex_;
  Clef::Impl::Emulator::Clock clock_;
  Clef::Impl::Emulator::Serial serial_;
  ActionQueue actionQueue_;
  GcodeParser parser_;
  Clef::Impl::Emulator::GenericTimer xAxisTimer_;
  Clef::Impl::Emulator::GenericTimer yAxisTimer_;
  Clef::Impl::Emulator::GenericTimer zAxisTimer_;
  Clef::Impl::Emulator::GenericTimer eAxisTimer_;
  Axes::XAxis xAxis_;
  Axes::YAxis yAxis_;
  Axes::ZAxis zAxis_;
  Axes::EAxis eAxis_;
  Axes axes_;
  Context context_;
};

TEST_F(GcodeParserTest, Basic) { doBasic(); }

TEST_F(GcodeParserTest, Comments) {
  Axes::XYZEPosition startPosition = actionQueue_.getEndPosition();
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
  Axes::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_FLOAT_EQ(*endPosition.x, Axes::XAxis::UstepsPerMm * 80.5);
  actionQueue_.pop(context_);
}

TEST_F(GcodeParserTest, G1_XY) {
  serial_.inject("G1 X40 Y30\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XY);
  Axes::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, Axes::XAxis::UstepsPerMm * 40);
  ASSERT_EQ(*endPosition.y, Axes::YAxis::UstepsPerMm * 30);
  actionQueue_.pop(context_);
}

TEST_F(GcodeParserTest, G1_XYE) {
  // Send a first XYE point
  serial_.inject("G1 X40 Y30 E2\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ASSERT_EQ(actionQueue_.size(), 1);
  ActionQueue::Iterator it = actionQueue_.last();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XYE);
  Axes::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, Axes::XAxis::UstepsPerMm * 40);
  ASSERT_EQ(*endPosition.y, Axes::YAxis::UstepsPerMm * 30);
  ASSERT_EQ(*endPosition.e, Axes::EAxis::UstepsPerMm * 2);
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it)->getNumPoints(), 1);
  ASSERT_EQ(context_.xyePositionQueue.size(), 1);
  Axes::XYEPosition xyePosition1 = *context_.xyePositionQueue.last();
  ASSERT_EQ(*xyePosition1.x, Axes::XAxis::UstepsPerMm * 40);
  ASSERT_EQ(*xyePosition1.y, Axes::YAxis::UstepsPerMm * 30);
  ASSERT_EQ(*xyePosition1.e, Axes::EAxis::UstepsPerMm * 2);

  // Send a second XYE point
  serial_.inject("G1 X80 Y60 E4\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ASSERT_EQ(actionQueue_.size(), 1);
  endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, Axes::XAxis::UstepsPerMm * 80);
  ASSERT_EQ(*endPosition.y, Axes::YAxis::UstepsPerMm * 60);
  ASSERT_EQ(*endPosition.e, Axes::EAxis::UstepsPerMm * 4);
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it)->getNumPoints(), 2);
  ASSERT_EQ(context_.xyePositionQueue.size(), 2);
  Axes::XYEPosition xyePosition2 = *context_.xyePositionQueue.last();
  ASSERT_EQ(*xyePosition2.x, Axes::XAxis::UstepsPerMm * 80);
  ASSERT_EQ(*xyePosition2.y, Axes::YAxis::UstepsPerMm * 60);
  ASSERT_EQ(*xyePosition2.e, Axes::EAxis::UstepsPerMm * 4);

  // Send a non-XYE point
  serial_.inject("G1 X0 Y0\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ASSERT_EQ(actionQueue_.size(), 2);
  it = actionQueue_.last();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XY);
  endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, 0);
  ASSERT_EQ(*endPosition.y, 0);
  ASSERT_EQ(*endPosition.e, Axes::EAxis::UstepsPerMm * 4);
  ASSERT_EQ(context_.xyePositionQueue.size(), 2);

  // Send a third XYE point in a different segment
  serial_.inject("G1 X30 Y30 E6\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  it = actionQueue_.last();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_XYE);
  ASSERT_EQ(actionQueue_.size(), 3);
  endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, Axes::XAxis::UstepsPerMm * 30);
  ASSERT_EQ(*endPosition.y, Axes::YAxis::UstepsPerMm * 30);
  ASSERT_EQ(*endPosition.e, Axes::EAxis::UstepsPerMm * 6);
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it)->getNumPoints(), 1);
  ASSERT_EQ(context_.xyePositionQueue.size(), 3);
  Axes::XYEPosition xyePosition3 = *context_.xyePositionQueue.last();
  ASSERT_EQ(*xyePosition3.x, Axes::XAxis::UstepsPerMm * 30);
  ASSERT_EQ(*xyePosition3.y, Axes::YAxis::UstepsPerMm * 30);
  ASSERT_EQ(*xyePosition3.e, Axes::EAxis::UstepsPerMm * 6);

  // Clean up
  actionQueue_.pop(context_);
  actionQueue_.pop(context_);
  actionQueue_.pop(context_);
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
  Axes::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.x, Axes::XAxis::UstepsPerMm * 40);
  ASSERT_EQ(*endPosition.y, Axes::YAxis::UstepsPerMm * 30);
  ASSERT_EQ(*endPosition.e, Axes::EAxis::UstepsPerMm * 2);
  ASSERT_EQ(dynamic_cast<Action::MoveXYE *>(*it)->getNumPoints(), 1);
  ASSERT_EQ(context_.xyePositionQueue.size(), 1);
  Axes::XYEPosition xyePosition1 = *context_.xyePositionQueue.last();
  ASSERT_EQ(*xyePosition1.x, Axes::XAxis::UstepsPerMm * 40);
  ASSERT_EQ(*xyePosition1.y, Axes::YAxis::UstepsPerMm * 30);
  ASSERT_EQ(*xyePosition1.e, Axes::EAxis::UstepsPerMm * 2);
}

TEST_F(GcodeParserTest, G1_E) {
  serial_.inject("G1 E5\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_E);
  Axes::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.e, Axes::EAxis::UstepsPerMm * 5);
  actionQueue_.pop(context_);
}

TEST_F(GcodeParserTest, G1_Z) {
  serial_.inject("G1 Z-10\n");
  parser_.ingest(context_);
  ASSERT_EQ(serial_.extract(), "ok\n");
  ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Action::Type::MOVE_Z);
  Axes::XYZEPosition endPosition = (*it)->getEndPosition();
  ASSERT_EQ(*endPosition.z,
            static_cast<int32_t>(Axes::ZAxis::UstepsPerMm) * -10);
  actionQueue_.pop(context_);
}

TEST_F(GcodeParserTest, G1_F) {
  serial_.inject("G1 X80 F3000\n");
  parser_.ingest(context_);
  ActionQueue::Iterator it = actionQueue_.first();
  ASSERT_EQ((*it)->getType(), Action::Type::SET_FEEDRATE);
  ASSERT_EQ((*it)->getEndPosition(), (Axes::XYZEPosition{0, 0, 0, 0}));
  actionQueue_.pop(context_);
  checkBasic(Axes::XYZEPosition{0, 0, 0, 0});
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
