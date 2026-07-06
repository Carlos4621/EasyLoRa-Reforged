#include "SerialTransport.hpp"

#include "FakeSerialStream.hpp"

#include <gtest/gtest.h>

#include <boost/asio.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace {
using TestTransport = BasicSerialTransport<FakeSerialStream>;

void drain(boost::asio::io_context& context) {
    context.run();
    context.restart();

    while (context.poll() != 0U) {
        context.restart();
    }

    context.restart();
}

std::vector<uint8_t> frame(std::initializer_list<uint8_t> bytes) {
    auto result = std::vector<uint8_t>(bytes);
    result.push_back(Frame::Frame_Delimiter);
    return result;
}
} // namespace

TEST(SerialTransportTests, OpenConfiguresStreamAndStartsReading) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };

    transport.open("/dev/test-lora");
    drain(context);

    EXPECT_TRUE(state->openCalled);
    EXPECT_EQ(state->openedPath, "/dev/test-lora");
    EXPECT_EQ(state->setOptionCount, 5U);
    EXPECT_NE(state->pendingRead, nullptr);
}

TEST(SerialTransportTests, OpenAppliesCustomSerialConfigValues) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    const SerialConfig config{
        .baudRate = 115200,
        .flowControl = boost::asio::serial_port_base::flow_control::type::hardware,
        .parityByte = boost::asio::serial_port_base::parity::type::odd,
        .stopBits = boost::asio::serial_port_base::stop_bits::type::two,
        .characterSize = 7,
    };

    transport.open("/dev/custom-lora", config);
    drain(context);

    EXPECT_EQ(state->openedPath, "/dev/custom-lora");
    ASSERT_TRUE(state->baudRate.has_value());
    ASSERT_TRUE(state->characterSize.has_value());
    ASSERT_TRUE(state->flowControl.has_value());
    ASSERT_TRUE(state->parity.has_value());
    ASSERT_TRUE(state->stopBits.has_value());
    EXPECT_EQ(*state->baudRate, config.baudRate);
    EXPECT_EQ(*state->characterSize, config.characterSize);
    EXPECT_EQ(*state->flowControl, config.flowControl);
    EXPECT_EQ(*state->parity, config.parityByte);
    EXPECT_EQ(*state->stopBits, config.stopBits);
}

TEST(SerialTransportTests, OpenPropagatesStreamOpenErrors) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };

    state->failNextOpen(boost::asio::error::access_denied);
    transport.open("/dev/denied-lora");

    EXPECT_THROW(drain(context), boost::system::system_error);
    EXPECT_TRUE(state->openCalled);
    EXPECT_FALSE(state->pendingRead);
}

TEST(SerialTransportTests, OpenPropagatesSetOptionErrors) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };

    state->failNextSetOption(boost::asio::error::invalid_argument);
    transport.open("/dev/bad-config-lora");

    EXPECT_THROW(drain(context), boost::system::system_error);
    EXPECT_TRUE(state->openCalled);
    EXPECT_EQ(state->setOptionCount, 1U);
    EXPECT_FALSE(state->pendingRead);
}

TEST(SerialTransportTests, ReceivesDelimitedFrame) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<std::vector<uint8_t>> receivedFrames;

    transport.setFrameHandler([&receivedFrames](std::vector<uint8_t> package) {
        receivedFrames.emplace_back(std::move(package));
    });

    transport.open("/dev/test-lora");
    drain(context);

    const auto expected = frame({ 0x10, 0x20, 0x30 });
    state->feedIncoming(expected);
    drain(context);

    ASSERT_EQ(receivedFrames.size(), 1U);
    EXPECT_EQ(receivedFrames.front(), expected);
    EXPECT_NE(state->pendingRead, nullptr);
}

TEST(SerialTransportTests, ReceivesEmptyFrame) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<std::vector<uint8_t>> receivedFrames;

    transport.setFrameHandler([&receivedFrames](std::vector<uint8_t> package) {
        receivedFrames.emplace_back(std::move(package));
    });

    transport.open("/dev/test-lora");
    drain(context);

    state->feedIncoming({ Frame::Frame_Delimiter });
    drain(context);

    ASSERT_EQ(receivedFrames.size(), 1U);
    EXPECT_EQ(receivedFrames.front(), (std::vector<uint8_t>{ Frame::Frame_Delimiter }));
}

TEST(SerialTransportTests, AllowsMissingFrameHandler) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };

    transport.open("/dev/test-lora");
    drain(context);

    state->feedIncoming(frame({ 0x42 }));

    EXPECT_NO_THROW(drain(context));
    EXPECT_NE(state->pendingRead, nullptr);
}

TEST(SerialTransportTests, ReceivesMultipleFramesAcrossReads) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<std::vector<uint8_t>> receivedFrames;

    transport.setFrameHandler([&receivedFrames](std::vector<uint8_t> package) {
        receivedFrames.emplace_back(std::move(package));
    });

    transport.open("/dev/test-lora");
    drain(context);

    const auto first = frame({ 0x01, 0x02 });
    state->feedIncoming(first);
    drain(context);

    const auto second = frame({ 0xAA, 0xBB, 0xCC });
    state->feedIncoming(second);
    drain(context);

    ASSERT_EQ(receivedFrames.size(), 2U);
    EXPECT_EQ(receivedFrames[0], first);
    EXPECT_EQ(receivedFrames[1], second);
}

TEST(SerialTransportTests, ReceivesFrameSplitAcrossReads) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<std::vector<uint8_t>> receivedFrames;

    transport.setFrameHandler([&receivedFrames](std::vector<uint8_t> package) {
        receivedFrames.emplace_back(std::move(package));
    });

    transport.open("/dev/test-lora");
    drain(context);

    state->feedIncoming({ 0xCA, 0xFE });
    drain(context);
    EXPECT_TRUE(receivedFrames.empty());

    const auto expected = frame({ 0xCA, 0xFE, 0xBA, 0xBE });
    state->feedIncoming({ 0xBA, 0xBE, Frame::Frame_Delimiter });
    drain(context);

    ASSERT_EQ(receivedFrames.size(), 1U);
    EXPECT_EQ(receivedFrames.front(), expected);
}

TEST(SerialTransportTests, ReceivesMultipleFramesFromOneRead) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<std::vector<uint8_t>> receivedFrames;

    transport.setFrameHandler([&receivedFrames](std::vector<uint8_t> package) {
        receivedFrames.emplace_back(std::move(package));
    });

    transport.open("/dev/test-lora");
    drain(context);

    const auto first = frame({ 0x11 });
    const auto second = frame({ 0x22, 0x33 });
    state->feedIncoming({ 0x11, Frame::Frame_Delimiter, 0x22, 0x33, Frame::Frame_Delimiter });
    drain(context);

    ASSERT_EQ(receivedFrames.size(), 2U);
    EXPECT_EQ(receivedFrames[0], first);
    EXPECT_EQ(receivedFrames[1], second);
}

TEST(SerialTransportTests, QueuesWritesInOrder) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };

    transport.open("/dev/test-lora");
    drain(context);

    EXPECT_EQ(transport.asyncWrite({ 0x01, 0x02 }), WriteStatus::Scheduled);
    EXPECT_EQ(transport.asyncWrite({ 0xA0, 0xB0, 0xC0 }), WriteStatus::Scheduled);
    drain(context);

    ASSERT_EQ(state->writes.size(), 2U);
    EXPECT_EQ(state->writes[0], (std::vector<uint8_t>{ 0x01, 0x02 }));
    EXPECT_EQ(state->writes[1], (std::vector<uint8_t>{ 0xA0, 0xB0, 0xC0 }));
}

TEST(SerialTransportTests, RejectsWritesBeforeOpenCompletes) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };

    EXPECT_EQ(transport.asyncWrite({ 0x01 }), WriteStatus::Closed);

    transport.open("/dev/test-lora");
    EXPECT_EQ(transport.asyncWrite({ 0x02 }), WriteStatus::Closed);
    drain(context);

    EXPECT_EQ(transport.asyncWrite({ 0x03 }), WriteStatus::Scheduled);
    drain(context);

    ASSERT_EQ(state->writes.size(), 1U);
    EXPECT_EQ(state->writes.front(), (std::vector<uint8_t>{ 0x03 }));
}

TEST(SerialTransportTests, RejectsWritesLargerThanConfiguredLimit) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    const BufferConfig bufferConfig{
        .rxBufferMaxSize = Default_Buffer_Config.rxBufferMaxSize,
        .txMaxElementsInQueue = Default_Buffer_Config.txMaxElementsInQueue,
        .txMaxSizePerElement = 2,
    };

    transport.open("/dev/test-lora", Default_Serial_Config, bufferConfig);
    drain(context);

    EXPECT_EQ(transport.asyncWrite({ 0x01, 0x02, 0x03 }), WriteStatus::MessageTooLong);
    drain(context);

    EXPECT_TRUE(state->writes.empty());
}

TEST(SerialTransportTests, RejectsWritesWhenQueueIsFull) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    state->autoCompleteWrites = false;
    TestTransport transport{ context };
    const BufferConfig bufferConfig{
        .rxBufferMaxSize = Default_Buffer_Config.rxBufferMaxSize,
        .txMaxElementsInQueue = 1,
        .txMaxSizePerElement = Default_Buffer_Config.txMaxSizePerElement,
    };

    transport.open("/dev/test-lora", Default_Serial_Config, bufferConfig);
    drain(context);

    EXPECT_EQ(transport.asyncWrite({ 0x01 }), WriteStatus::Scheduled);
    EXPECT_EQ(transport.asyncWrite({ 0x02 }), WriteStatus::QueueFull);
    drain(context);

    ASSERT_EQ(state->writes.size(), 1U);
    EXPECT_EQ(state->writes.front(), (std::vector<uint8_t>{ 0x01 }));

    state->completePendingWrite();
    drain(context);

    EXPECT_EQ(transport.asyncWrite({ 0x03 }), WriteStatus::Scheduled);
    drain(context);

    ASSERT_EQ(state->writes.size(), 2U);
    EXPECT_EQ(state->writes.back(), (std::vector<uint8_t>{ 0x03 }));
}

TEST(SerialTransportTests, DoesNotStartQueuedWriteUntilCurrentWriteCompletes) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    state->autoCompleteWrites = false;
    TestTransport transport{ context };

    transport.open("/dev/test-lora");
    drain(context);

    EXPECT_EQ(transport.asyncWrite({ 0x01 }), WriteStatus::Scheduled);
    EXPECT_EQ(transport.asyncWrite({ 0x02, 0x03 }), WriteStatus::Scheduled);
    drain(context);

    ASSERT_EQ(state->writes.size(), 1U);
    EXPECT_EQ(state->writes[0], (std::vector<uint8_t>{ 0x01 }));
    ASSERT_NE(state->pendingWrite, nullptr);

    state->completePendingWrite();
    drain(context);

    ASSERT_EQ(state->writes.size(), 2U);
    EXPECT_EQ(state->writes[1], (std::vector<uint8_t>{ 0x02, 0x03 }));
}

TEST(SerialTransportTests, CloseDoesNotStartQueuedWritesWhileCurrentWriteIsPending) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    state->autoCompleteWrites = false;
    TestTransport transport{ context };

    transport.open("/dev/test-lora");
    drain(context);

    EXPECT_EQ(transport.asyncWrite({ 0x01 }), WriteStatus::Scheduled);
    EXPECT_EQ(transport.asyncWrite({ 0x02 }), WriteStatus::Scheduled);
    drain(context);
    ASSERT_EQ(state->writes.size(), 1U);

    transport.close();
    drain(context);

    EXPECT_TRUE(state->cancelCalled);
    EXPECT_TRUE(state->closeCalled);
    EXPECT_EQ(state->writes.size(), 1U);

    state->completePendingWrite(boost::asio::error::operation_aborted);
    drain(context);

    EXPECT_EQ(transport.asyncWrite({ 0x03 }), WriteStatus::Closed);
    drain(context);

    EXPECT_EQ(state->writes.size(), 1U);
}

TEST(SerialTransportTests, CloseCancelsReadAndPendingWriteTogether) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    state->autoCompleteWrites = false;
    TestTransport transport{ context };
    std::vector<FatalErrors> fatalErrors;
    std::vector<RecoverableErrors> recoverableErrors;

    transport.setFatalErrorHandler([&fatalErrors](FatalErrors error) {
        fatalErrors.push_back(error);
    });
    transport.setRecuperableErrorHandler([&recoverableErrors](RecoverableErrors error) {
        recoverableErrors.push_back(error);
    });
    transport.open("/dev/test-lora");
    drain(context);

    EXPECT_EQ(transport.asyncWrite({ 0x01 }), WriteStatus::Scheduled);
    EXPECT_EQ(transport.asyncWrite({ 0x02 }), WriteStatus::Scheduled);
    drain(context);

    ASSERT_NE(state->pendingRead, nullptr);
    ASSERT_NE(state->pendingWrite, nullptr);
    ASSERT_EQ(state->writes.size(), 1U);

    transport.close();
    drain(context);

    EXPECT_TRUE(state->cancelCalled);
    EXPECT_TRUE(state->closeCalled);
    EXPECT_TRUE(fatalErrors.empty());
    EXPECT_TRUE(recoverableErrors.empty());

    state->completePendingWrite(boost::asio::error::operation_aborted);
    drain(context);

    EXPECT_EQ(state->writes.size(), 1U);
}

TEST(SerialTransportTests, FatalReadErrorsCallFatalErrorHandler) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<FatalErrors> errors;

    transport.setFatalErrorHandler([&errors](FatalErrors error) {
        errors.push_back(error);
    });

    transport.open("/dev/test-lora");
    drain(context);

    state->failNextRead(boost::asio::error::eof);
    drain(context);

    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front(), FatalErrors::UnpluggedDevice);
}

TEST(SerialTransportTests, FatalReadErrorsAreClarifiedBeforeNotification) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<FatalErrors> errors;

    transport.setFatalErrorHandler([&errors](FatalErrors error) {
        errors.push_back(error);
    });

    transport.open("/dev/test-lora");
    drain(context);
    state->failNextRead(std::make_error_code(std::errc::permission_denied));
    drain(context);

    transport.open("/dev/test-lora");
    drain(context);
    state->failNextRead(boost::asio::error::bad_descriptor);
    drain(context);

    ASSERT_EQ(errors.size(), 2U);
    EXPECT_EQ(errors[0], FatalErrors::PermissionDenied);
    EXPECT_EQ(errors[1], FatalErrors::ExternallyClosedPort);
}

TEST(SerialTransportTests, RecoverableReadErrorsCallRecoverableErrorHandlerAndContinueReading) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<RecoverableErrors> errors;
    std::vector<std::vector<uint8_t>> receivedFrames;

    transport.setRecuperableErrorHandler([&errors](RecoverableErrors error) {
        errors.push_back(error);
    });
    transport.setFrameHandler([&receivedFrames](std::vector<uint8_t> package) {
        receivedFrames.emplace_back(std::move(package));
    });

    transport.open("/dev/test-lora");
    drain(context);

    state->feedIncoming({ 0xCA, 0xFE });
    drain(context);
    state->failNextRead(boost::asio::error::not_found);
    drain(context);
    state->feedIncoming(frame({ 0x42 }));
    drain(context);

    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front(), RecoverableErrors::RxBufferFull);
    ASSERT_EQ(receivedFrames.size(), 1U);
    EXPECT_EQ(receivedFrames.front(), frame({ 0x42 }));
    EXPECT_FALSE(state->closeCalled);
    EXPECT_NE(state->pendingRead, nullptr);
}

TEST(SerialTransportTests, ReadErrorStopsTransportAndClearsPartialFrame) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<std::vector<uint8_t>> receivedFrames;
    std::vector<FatalErrors> errors;

    transport.setFrameHandler([&receivedFrames](std::vector<uint8_t> package) {
        receivedFrames.emplace_back(std::move(package));
    });
    transport.setFatalErrorHandler([&errors](FatalErrors error) {
        errors.push_back(error);
    });
    transport.open("/dev/test-lora");
    drain(context);

    state->feedIncoming({ 0xCA, 0xFE });
    drain(context);
    state->failNextRead(boost::asio::error::eof, { 0xBA, 0xBE });
    drain(context);
    state->feedIncoming({ Frame::Frame_Delimiter });
    drain(context);

    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front(), FatalErrors::UnpluggedDevice);
    EXPECT_TRUE(receivedFrames.empty());
    EXPECT_TRUE(state->cancelCalled);
    EXPECT_TRUE(state->closeCalled);
    EXPECT_EQ(state->pendingRead, nullptr);
}

TEST(SerialTransportTests, OpenRestartsTransportAfterReadError) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<std::vector<uint8_t>> receivedFrames;

    transport.setFrameHandler([&receivedFrames](std::vector<uint8_t> package) {
        receivedFrames.emplace_back(std::move(package));
    });
    transport.open("/dev/test-lora");
    drain(context);

    state->failNextRead(boost::asio::error::eof);
    drain(context);

    EXPECT_TRUE(state->closeCalled);
    EXPECT_EQ(state->pendingRead, nullptr);

    transport.open("/dev/test-lora");
    drain(context);
    state->feedIncoming(frame({ 0x42 }));
    drain(context);

    ASSERT_EQ(receivedFrames.size(), 1U);
    EXPECT_EQ(receivedFrames.front(), frame({ 0x42 }));
    EXPECT_NE(state->pendingRead, nullptr);
}

TEST(SerialTransportTests, ReadErrorDiscardsQueuedWrites) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    state->autoCompleteWrites = false;
    TestTransport transport{ context };

    transport.open("/dev/test-lora");
    drain(context);

    EXPECT_EQ(transport.asyncWrite({ 0x01 }), WriteStatus::Scheduled);
    EXPECT_EQ(transport.asyncWrite({ 0x02 }), WriteStatus::Scheduled);
    drain(context);

    ASSERT_EQ(state->writes.size(), 1U);
    ASSERT_NE(state->pendingWrite, nullptr);

    state->failNextRead(boost::asio::error::eof);
    drain(context);
    state->completePendingWrite(boost::asio::error::operation_aborted);
    drain(context);

    EXPECT_TRUE(state->closeCalled);
    EXPECT_EQ(state->writes.size(), 1U);
}

TEST(SerialTransportTests, AllowsMissingFatalErrorHandler) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };

    transport.open("/dev/test-lora");
    drain(context);

    state->failNextRead(boost::asio::error::eof);

    EXPECT_NO_THROW(drain(context));
}

TEST(SerialTransportTests, WriteErrorsCallFatalErrorHandler) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<FatalErrors> errors;

    transport.setFatalErrorHandler([&errors](FatalErrors error) {
        errors.push_back(error);
    });

    transport.open("/dev/test-lora");
    drain(context);

    state->failNextWrite(boost::asio::error::eof);
    EXPECT_EQ(transport.asyncWrite({ 0x77 }), WriteStatus::Scheduled);
    drain(context);

    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front(), FatalErrors::UnpluggedDevice);
    EXPECT_TRUE(state->writes.empty());
    EXPECT_TRUE(state->cancelCalled);
    EXPECT_TRUE(state->closeCalled);
}

TEST(SerialTransportTests, AllowsMissingFatalErrorHandlerOnWriteError) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };

    transport.open("/dev/test-lora");
    drain(context);

    state->failNextWrite(boost::asio::error::eof);
    EXPECT_EQ(transport.asyncWrite({ 0x77 }), WriteStatus::Scheduled);

    EXPECT_NO_THROW(drain(context));
    EXPECT_TRUE(state->writes.empty());
}

TEST(SerialTransportTests, WriteErrorStopsTransportAndDiscardsQueuedWrites) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<FatalErrors> errors;

    transport.setFatalErrorHandler([&errors](FatalErrors error) {
        errors.push_back(error);
    });
    transport.open("/dev/test-lora");
    drain(context);

    state->failNextWrite(boost::asio::error::eof);
    EXPECT_EQ(transport.asyncWrite({ 0x01 }), WriteStatus::Scheduled);
    EXPECT_EQ(transport.asyncWrite({ 0x02, 0x03 }), WriteStatus::Scheduled);
    drain(context);

    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front(), FatalErrors::UnpluggedDevice);
    EXPECT_TRUE(state->writes.empty());
    EXPECT_TRUE(state->closeCalled);
}

TEST(SerialTransportTests, OpenRestartsTransportAfterWriteError) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<std::vector<uint8_t>> receivedFrames;

    transport.setFrameHandler([&receivedFrames](std::vector<uint8_t> package) {
        receivedFrames.emplace_back(std::move(package));
    });
    transport.open("/dev/test-lora");
    drain(context);

    state->failNextWrite(boost::asio::error::eof);
    EXPECT_EQ(transport.asyncWrite({ 0x01 }), WriteStatus::Scheduled);
    drain(context);

    EXPECT_TRUE(state->closeCalled);

    transport.open("/dev/test-lora");
    drain(context);
    EXPECT_EQ(transport.asyncWrite({ 0x02 }), WriteStatus::Scheduled);
    state->feedIncoming(frame({ 0x42 }));
    drain(context);

    ASSERT_EQ(state->writes.size(), 1U);
    EXPECT_EQ(state->writes.front(), (std::vector<uint8_t>{ 0x02 }));
    ASSERT_EQ(receivedFrames.size(), 1U);
    EXPECT_EQ(receivedFrames.front(), frame({ 0x42 }));
}

TEST(SerialTransportTests, CloseCancelsAndClosesStream) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<FatalErrors> fatalErrors;
    std::vector<RecoverableErrors> recoverableErrors;

    transport.setFatalErrorHandler([&fatalErrors](FatalErrors error) {
        fatalErrors.push_back(error);
    });
    transport.setRecuperableErrorHandler([&recoverableErrors](RecoverableErrors error) {
        recoverableErrors.push_back(error);
    });

    transport.open("/dev/test-lora");
    drain(context);

    transport.close();
    drain(context);

    EXPECT_TRUE(state->cancelCalled);
    EXPECT_TRUE(state->closeCalled);
    EXPECT_TRUE(fatalErrors.empty());
    EXPECT_TRUE(recoverableErrors.empty());
}

TEST(SerialTransportTests, CloseIsIdempotent) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };

    transport.open("/dev/test-lora");
    drain(context);

    transport.close();
    transport.close();
    transport.close();
    drain(context);

    EXPECT_EQ(state->cancelCount, 1U);
    EXPECT_EQ(state->closeCount, 1U);
}

TEST(SerialTransportTests, ClosePreventsLaterOpenAndWrite) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };

    transport.close();
    drain(context);

    transport.open("/dev/test-lora");
    EXPECT_EQ(transport.asyncWrite({ 0x01 }), WriteStatus::Closed);
    drain(context);

    EXPECT_FALSE(state->openCalled);
    EXPECT_TRUE(state->cancelCalled);
    EXPECT_TRUE(state->closeCalled);
    EXPECT_TRUE(state->writes.empty());
}

TEST(SerialTransportTests, CloseIgnoresLaterHandlerUpdates) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    TestTransport transport{ context };
    std::vector<std::vector<uint8_t>> receivedFrames;
    std::vector<FatalErrors> fatalErrors;
    std::vector<RecoverableErrors> recoverableErrors;

    transport.close();
    transport.setFrameHandler([&receivedFrames](std::vector<uint8_t> package) {
        receivedFrames.emplace_back(std::move(package));
    });
    transport.setFatalErrorHandler([&fatalErrors](FatalErrors error) {
        fatalErrors.push_back(error);
    });
    transport.setRecuperableErrorHandler([&recoverableErrors](RecoverableErrors error) {
        recoverableErrors.push_back(error);
    });
    drain(context);

    state->failNextRead(boost::asio::error::operation_aborted);
    state->feedIncoming(frame({ 0x42 }));
    drain(context);

    EXPECT_TRUE(receivedFrames.empty());
    EXPECT_TRUE(fatalErrors.empty());
    EXPECT_TRUE(recoverableErrors.empty());
}

TEST(SerialTransportTests, DestructorCancelsAndClosesPendingReadWithoutReportingError) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();
    std::vector<FatalErrors> fatalErrors;
    std::vector<RecoverableErrors> recoverableErrors;

    {
        TestTransport transport{ context };
        transport.setFatalErrorHandler([&fatalErrors](FatalErrors error) {
            fatalErrors.push_back(error);
        });
        transport.setRecuperableErrorHandler([&recoverableErrors](RecoverableErrors error) {
            recoverableErrors.push_back(error);
        });
        transport.open("/dev/test-lora");
        drain(context);
        ASSERT_NE(state->pendingRead, nullptr);
    }

    drain(context);

    EXPECT_TRUE(state->cancelCalled);
    EXPECT_TRUE(state->closeCalled);
    EXPECT_TRUE(fatalErrors.empty());
    EXPECT_TRUE(recoverableErrors.empty());
}

TEST(SerialTransportTests, DestructorRequestsCloseButNeedsContextToRunIt) {
    boost::asio::io_context context;
    const auto state = FakeSerialStream::prepareNextInstance();

    {
        TestTransport transport{ context };
        transport.open("/dev/test-lora");
        drain(context);
        ASSERT_NE(state->pendingRead, nullptr);
    }

    EXPECT_FALSE(state->cancelCalled);
    EXPECT_FALSE(state->closeCalled);

    drain(context);

    EXPECT_TRUE(state->cancelCalled);
    EXPECT_TRUE(state->closeCalled);
}
