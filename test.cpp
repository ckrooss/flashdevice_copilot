#include <gtest/gtest.h>
#include "IFlashDevice.h"
#include "StringFlashDevice.h"
#include "FileFlashDevice.h"
#include "SparseDataDevice.h"

class IFlashDeviceTest : public ::testing::TestWithParam<IFlashDevice*> {
  protected:
    IFlashDevice* flash_;

    void SetUp() override {
        flash_ = GetParam();
        ASSERT_EQ(flash_->open(), IFlashDevice::error::none);
    }

    void TearDown() override {
        flash_->close();
    }
};

TEST_P(IFlashDeviceTest, WriteReadData) {
    std::vector<uint8_t> write_data = {0x01, 0x02, 0x03};
    EXPECT_EQ(flash_->write(write_data, 0), IFlashDevice::error::none);

    std::vector<uint8_t> read_data;
    EXPECT_EQ(flash_->read(read_data, 0, write_data.size()), IFlashDevice::error::none);
    EXPECT_EQ(read_data, write_data);
}

TEST_P(IFlashDeviceTest, EraseData) {
    std::vector<uint8_t> write_data = {0x01, 0x02, 0x03};
    EXPECT_EQ(flash_->write(write_data, 0), IFlashDevice::error::none);

    EXPECT_EQ(flash_->erase(0, write_data.size()), IFlashDevice::error::none);

    std::vector<uint8_t> buffer;
    EXPECT_EQ(flash_->read(buffer, 0, write_data.size()), IFlashDevice::error::none);
    EXPECT_EQ(buffer.size(), write_data.size());
    for (auto& b : buffer) {
        EXPECT_EQ(b, 0xff);
    }
}

TEST_P(IFlashDeviceTest, UninitializedData) {
    std::vector<uint8_t> buffer;
    EXPECT_EQ(flash_->read(buffer, 0, 3), IFlashDevice::error::none);
    EXPECT_EQ(buffer.size(), 3u);
    for (auto& b : buffer) {
        EXPECT_EQ(b, 0xff);
    }
}

TEST_P(IFlashDeviceTest, OutOfBoundsRead) {
    std::vector<uint8_t> buffer;
    EXPECT_EQ(flash_->read(buffer, flash_->size(), 1), IFlashDevice::error::out_of_bounds);
}

TEST_P(IFlashDeviceTest, OutOfBoundsWrite) {
    std::vector<uint8_t> buffer = {0x01, 0x02, 0x03};
    EXPECT_EQ(flash_->write(buffer, flash_->size() - 2), IFlashDevice::error::out_of_bounds);
}

TEST_P(IFlashDeviceTest, OutOfBoundsErase) {
    EXPECT_EQ(flash_->erase(flash_->size() - 1, 2), IFlashDevice::error::out_of_bounds);
}

INSTANTIATE_TEST_SUITE_P(IFlashDeviceImplementations, IFlashDeviceTest,
                         ::testing::Values(new StringFlashDevice(1024), new FileFlashDevice("./flash.bin", 1024), new SparseDataDevice(1024)));

TEST(SparseDataDeviceTest, WriteOverlappingBlocks) {
    SparseDataDevice flash(1024);

    std::vector<uint8_t> write_data = {0x01, 0x02, 0x03};
    EXPECT_EQ(flash.write(write_data, 0), IFlashDevice::error::none);

    std::vector<uint8_t> read_data;
    EXPECT_EQ(flash.read(read_data, 0, 3), IFlashDevice::error::none);
    EXPECT_EQ(read_data, write_data);

    EXPECT_EQ(flash.write(write_data, 1), IFlashDevice::error::none);

    read_data.clear();
    std::vector<uint8_t> expected_data = {0x01, 0x01, 0x02, 0x03};
    EXPECT_EQ(flash.read(read_data, 0, 4), IFlashDevice::error::none);
    EXPECT_EQ(read_data, expected_data);
    EXPECT_EQ(flash.used_chunks(), 1);

    flash.close();
}

TEST(SparseDataDeviceTest, WriteOverlapFromLeft) {
    SparseDataDevice device(100);
    std::vector<uint8_t> data1{0x01, 0x02, 0x03};
    std::vector<uint8_t> data2{0x04, 0x05, 0x06};
    std::vector<uint8_t> result{0x04, 0x05, 0x01, 0x02};
    device.write(data1, 2);
    device.write(data2, 0);
    std::vector<uint8_t> read_result(4);
    device.read(read_result, 0, 4);
    EXPECT_EQ(read_result, result);
}

TEST(SparseDataDeviceTest, WriteOverlapFromRight) {
    SparseDataDevice device(100);
    std::vector<uint8_t> data1{0x01, 0x02, 0x03};
    std::vector<uint8_t> data2{0x04, 0x05, 0x06};
    std::vector<uint8_t> result{0x01, 0x02, 0x04, 0x05, 0x06};
    device.write(data1, 0);
    device.write(data2, 2);
    std::vector<uint8_t> read_result(5);
    device.read(read_result, 0, 5);
    EXPECT_EQ(read_result, result);
}

TEST(SparseDataDeviceTest, WriteOverlapComplete) {
    SparseDataDevice device(100);
    std::vector<uint8_t> data1{0x01, 0x02, 0x03};
    std::vector<uint8_t> data2{0x04, 0x05, 0x06};
    std::vector<uint8_t> result{0x04, 0x05, 0x06};
    device.write(data1, 0);
    device.write(data2, 0);
    std::vector<uint8_t> read_result(3);
    device.read(read_result, 0, 3);
    EXPECT_EQ(read_result, result);
}

TEST(SparseDataDeviceTest, ReadOverlap) {
    SparseDataDevice device(100);

    // Test when reading within bounds but overlapping with write data
    std::vector<uint8_t> write_data = {0x01, 0x02, 0x03};
    EXPECT_EQ(device.write(write_data, 20), IFlashDevice::error::none);
    std::vector<uint8_t> read_data;
    EXPECT_EQ(device.read(read_data, 15, 10), IFlashDevice::error::none);
    std::vector<uint8_t> expected_data = {0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x02, 0x03, 0xff, 0xff};
    EXPECT_EQ(read_data, expected_data);
}

TEST(SparseDataDeviceTest, ReadOverlapMultipleChunks) {
    SparseDataDevice device(100);

    // Test when reading within bounds but overlapping with write data
    std::vector<uint8_t> write_data = {0x01, 0x02, 0x03};
    EXPECT_EQ(device.write(write_data, 3), IFlashDevice::error::none);
    EXPECT_EQ(device.write(write_data, 7), IFlashDevice::error::none);

    // Should now be ff ff ff 01 02 03 ff 01 02 03 

    std::vector<uint8_t> read_data;
    EXPECT_EQ(device.read(read_data, 0, 10), IFlashDevice::error::none);
    std::vector<uint8_t> expected_data = {0xff, 0xff, 0xff, 0x01, 0x02, 0x03, 0xff, 0x01, 0x02, 0x03};
    EXPECT_EQ(read_data, expected_data);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
