#pragma once

#include <vector>
#include <filesystem>
#include <ostream>

class IFlashDevice {
  public:
    enum class error {
        none,
        open_error,
        close_error,
        read_error,
        write_error,
        erase_error,
        out_of_bounds,
    };

    virtual error open(const std::filesystem::path& path = {}) = 0;
    virtual error close() = 0;
    virtual error read(std::vector<uint8_t>& data, size_t offset, size_t size) = 0;
    virtual error write(const std::vector<uint8_t>& data, size_t offset) = 0;
    virtual error erase(size_t offset, size_t size) = 0;
    virtual std::size_t size() const = 0;

    virtual ~IFlashDevice() = default;
};

std::ostream& operator<<(std::ostream& os, IFlashDevice::error error) {
    switch (error) {
    case IFlashDevice::error::none:
        os << "none";
        break;
    case IFlashDevice::error::open_error:
        os << "open_error";
        break;
    case IFlashDevice::error::close_error:
        os << "close_error";
        break;
    case IFlashDevice::error::read_error:
        os << "read_error";
        break;
    case IFlashDevice::error::write_error:
        os << "write_error";
        break;
    case IFlashDevice::error::erase_error:
        os << "erase_error";
        break;
    case IFlashDevice::error::out_of_bounds:
        os << "out_of_bounds";
        break;
    default:
        os << "unknown";
        break;
    }
    return os;
}