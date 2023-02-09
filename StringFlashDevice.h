#pragma once

#include "IFlashDevice.h"

#include <vector>
#include <string>
#include <filesystem>
#include "IFlashDevice.h"

class StringFlashDevice : public IFlashDevice {
  public:
    StringFlashDevice(size_t size) : data_(size, 0xff) {
    }

    error open(const std::filesystem::path& /*path*/ = {}) override {
        return error::none;
    }

    error close() override {
        return error::none;
    }

    error read(std::vector<uint8_t>& buffer, size_t offset, size_t size) override {
        if (offset + size > data_.size()) {
            return error::out_of_bounds;
        }
        buffer.resize(size);
        std::copy(data_.begin() + offset, data_.begin() + offset + size, buffer.begin());
        return error::none;
    }

    error write(const std::vector<uint8_t>& buffer, size_t offset) override {
        if (offset + buffer.size() > data_.size()) {
            return error::out_of_bounds;
        }
        std::copy(buffer.begin(), buffer.end(), data_.begin() + offset);
        return error::none;
    }

    error erase(size_t offset, size_t size) override {
        if (offset + size > data_.size()) {
            return error::out_of_bounds;
        }
        std::fill(data_.begin() + offset, data_.begin() + offset + size, 0xff);
        return error::none;
    }

    std::size_t size() const override {
        return data_.size();
    }

  private:
    std::string data_;
};
