#include <vector>
#include <fstream>
#include <filesystem>
#include "IFlashDevice.h"

class FileFlashDevice : public IFlashDevice {
  public:
    FileFlashDevice(const std::filesystem::path& path, size_t size) : path_(path), size_(size) {
    }

    error open(const std::filesystem::path& path = {}) override {
        if (!path.empty()) {
            path_ = path;
        }
        if (std::filesystem::exists(path_)) {
            file_.open(path_, std::ios::binary | std::ios::in | std::ios::out);
        } else {
            file_.open(path_, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        }

        assert(file_.is_open());
        if (!file_) {
            return error::open_error;
        }
        file_.seekp(0, std::ios::end);
        if (file_.tellp() != size_) {
            file_.seekp(0);
            std::vector<uint8_t> buffer(size_, 0xff);
            file_.write(reinterpret_cast<const char*>(buffer.data()), size_);
        }
        return error::none;
    }

    error close() override {
        file_.close();
        return error::none;
    }

    error read(std::vector<uint8_t>& buffer, size_t offset, size_t size) override {
        if (!file_.is_open()) {
            return error::read_error;
        }
        if (offset + size > size_) {
            return error::out_of_bounds;
        }
        buffer.resize(size);
        file_.seekg(offset);
        file_.read(reinterpret_cast<char*>(buffer.data()), size);
        if (file_.gcount() != size) {
            return error::read_error;
        }
        return error::none;
    }

    error write(const std::vector<uint8_t>& buffer, size_t offset) override {
        if (!file_.is_open()) {
            return error::write_error;
        }
        if (offset + buffer.size() > size_) {
            return error::out_of_bounds;
        }
        file_.seekp(offset);
        file_.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        return error::none;
    }

    error erase(size_t offset, size_t size) override {
        if (!file_.is_open()) {
            return error::erase_error;
        }
        if (offset + size > size_) {
            return error::out_of_bounds;
        }
        std::vector<uint8_t> buffer(size, 0xff);
        return write(buffer, offset);
    }

    std::size_t size() const override {
        return size_;
    }

  private:
    std::filesystem::path path_;
    size_t size_;
    std::fstream file_;
};
