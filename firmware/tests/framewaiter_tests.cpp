#include "FrameWaiter.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace {
constexpr uint8_t kDelimiter = 0x00;
std::atomic<int> g_callback_counter{0};

void ResetCallbackCounter() {
    g_callback_counter.store(0, std::memory_order_relaxed);
}

void TestCallback() {
    g_callback_counter.fetch_add(1, std::memory_order_relaxed);
}
}

TEST(FrameWaiterTests, CallbackCalledOnDelimiter) {
    ResetCallbackCounter();
    FrameWaiter<16> fw{TestCallback};

    fw.feed(0x11);
    fw.feed(kDelimiter);
    fw.feed(0x22);
    fw.feed(kDelimiter);

    EXPECT_EQ(g_callback_counter.load(std::memory_order_relaxed), 2);
}

TEST(FrameWaiterTests, ReadFrameOk) {
    ResetCallbackCounter();
    FrameWaiter<16> fw{TestCallback};

    fw.feed(0x10);
    fw.feed(0x20);
    fw.feed(0x30);
    fw.feed(kDelimiter);

    std::array<uint8_t, 16> out{};
    size_t written = 0;
    EXPECT_EQ(fw.tryReadFrame(out.data(), out.size(), written), ReadFrameStatus::OK);
    EXPECT_EQ(written, 3u);
    EXPECT_EQ(out[0], 0x10);
    EXPECT_EQ(out[1], 0x20);
    EXPECT_EQ(out[2], 0x30);
}

TEST(FrameWaiterTests, IncompleteFrame) {
    ResetCallbackCounter();
    FrameWaiter<16> fw{TestCallback};

    fw.feed(0x10);
    fw.feed(0x20);

    std::array<uint8_t, 16> out{};
    size_t written = 123;
    EXPECT_EQ(fw.tryReadFrame(out.data(), out.size(), written), ReadFrameStatus::IncompleteFrame);
    EXPECT_EQ(written, 0u);
}

TEST(FrameWaiterTests, BufferNullptr) {
    ResetCallbackCounter();
    FrameWaiter<16> fw{TestCallback};

    size_t written = 55;
    EXPECT_EQ(fw.tryReadFrame(nullptr, 8, written), ReadFrameStatus::BufferNullptr);
    EXPECT_EQ(written, 0u);
}

TEST(FrameWaiterTests, BufferTooSmallDropsFrame) {
    ResetCallbackCounter();
    FrameWaiter<16> fw{TestCallback};

    fw.feed(0x01);
    fw.feed(0x02);
    fw.feed(0x03);
    fw.feed(0x04);
    fw.feed(kDelimiter);

    std::array<uint8_t, 3> out{};
    size_t written = 0;
    EXPECT_EQ(fw.tryReadFrame(out.data(), out.size(), written), ReadFrameStatus::BufferTooSmall);
    EXPECT_EQ(written, 0u);

    fw.feed(0x09);
    fw.feed(kDelimiter);

    std::array<uint8_t, 4> out2{};
    EXPECT_EQ(fw.tryReadFrame(out2.data(), out2.size(), written), ReadFrameStatus::OK);
    EXPECT_EQ(written, 1u);
    EXPECT_EQ(out2[0], 0x09);
}

TEST(FrameWaiterTests, OverflowResetsAfterDelimiter) {
    ResetCallbackCounter();
    FrameWaiter<8> fw{TestCallback};

    for (int i = 0; i < 9; ++i) {
        fw.feed(0xAA);
    }

    std::array<uint8_t, 8> out{};
    size_t written = 0;
    EXPECT_EQ(fw.tryReadFrame(out.data(), out.size(), written), ReadFrameStatus::Overflow);
    EXPECT_EQ(written, 0u);

    fw.feed(0x01);
    fw.feed(kDelimiter);
    fw.feed(0x55);
    fw.feed(kDelimiter);

    EXPECT_EQ(fw.tryReadFrame(out.data(), out.size(), written), ReadFrameStatus::OK);
    EXPECT_EQ(written, 1u);
    EXPECT_EQ(out[0], 0x55);
}

TEST(FrameWaiterTests, ProducerConsumerTwoThreads) {
    ResetCallbackCounter();
    FrameWaiter<64> fw{TestCallback};

    std::vector<std::vector<uint8_t>> frames = {
        {0x01, 0x02, 0x03},
        {0x10, 0x20},
        {0xAA, 0xBB, 0xCC, 0xDD}
    };

    std::atomic<bool> producer_done{false};

    std::thread producer([&]() {
        for (const auto& frame : frames) {
            for (uint8_t byte : frame) {
                fw.feed(byte);
            }
            fw.feed(kDelimiter);
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::vector<std::vector<uint8_t>> received;
    std::array<uint8_t, 64> out{};
    size_t written = 0;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while ((received.size() < frames.size() || !producer_done.load(std::memory_order_acquire)) &&
           std::chrono::steady_clock::now() < deadline) {
        while (true) {
            ReadFrameStatus status = fw.tryReadFrame(out.data(), out.size(), written);
            if (status == ReadFrameStatus::OK) {
                received.emplace_back(out.begin(), out.begin() + static_cast<std::ptrdiff_t>(written));
                continue;
            }
            if (status != ReadFrameStatus::IncompleteFrame) {
                ADD_FAILURE() << "Unexpected status: " << static_cast<int>(status);
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    producer.join();

    ASSERT_EQ(received.size(), frames.size());
    EXPECT_EQ(received, frames);
    EXPECT_EQ(g_callback_counter.load(std::memory_order_relaxed), static_cast<int>(frames.size()));
}
