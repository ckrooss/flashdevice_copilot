#include "IFlashDevice.h" // IFlashDevice
#include <map>            // std::map
#include <vector>         // std::vector

/**
 * @brief A flash device that stores data in a sparse map
 * 
 */
class SparseDataDevice : public IFlashDevice {
  public:
    /**
   * @brief Construct a new Sparse Data Device object
   * 
   * @param size - size of the device in bytes
   */
    SparseDataDevice(std::size_t size) : size_(size) {
    }

    /**
     * @brief Open the device
     * 
     * @param path - path to the device
     * @return error - error code
     */
    virtual error open(const std::filesystem::path& path) override {
        return error::none;
    }

    /**
     * @brief Close the device
     * 
     * @return error - error code
     */
    virtual error close() override {
        return error::none;
    }

    /**
     * @brief Read data from the device
     * 
     * @param data - output vector
     * @param offset - offset in the device 
     * @param size - size of the data to read
     * @return error - error code
     */
    virtual error read(std::vector<uint8_t>& data, size_t offset, size_t size) override {
        if (offset + size > size_) {
            return error::out_of_bounds;
        }

        data.resize(size);
        // Fill the output vector with 0xff
        std::fill(data.begin(), data.end(), 0xff);

        for (const auto& [coffset, cdata] : chunks_) {
            // If the chunk is after the offset, break, we are to far
            if (coffset >= offset + size) {
                break;
            }

            // If the chunk is before the offset, skip it
            if (coffset + cdata.size() <= offset) {
                continue;
            }

            // Write the data
            size_t write_start = std::max(coffset, offset);
            size_t write_end = std::min(coffset + cdata.size(), offset + size);
            size_t copy_size = write_end - write_start;
            std::copy(cdata.begin() + (write_start - coffset), cdata.begin() + (write_start - coffset) + copy_size,
                      data.begin() + (write_start - offset));
        }

        return error::none;
    }

    /**
     * @brief Write data to the device
     * 
     * @param data - data to write
     * @param offset - offset in the device
     * @return error - error code
     */
    virtual error write(const std::vector<uint8_t>& data, size_t offset) override {
        // Check if the write is out of bounds
        if (offset + data.size() > size_) {
            return error::out_of_bounds;
        }

        const auto end = offset + data.size();

        for (auto& [coffset, cdata] : chunks_) {
            const auto cend = coffset + cdata.size();

            // If the start of the cached write is after the new offset, break, we are to far
            if (coffset >= end) {
                break;
            }

            // If the start of the write is after the offset, skip it
            if (cend <= offset) {
                continue;
            }

            // If same data, overwrite directly
            if (coffset == offset && cdata.size() == data.size()) {
                cdata = data;
                return error::none;
            }

            // If overlap completely, remove the old data
            if (offset <= coffset && end >= cend + (coffset - offset)) {
                chunks_.erase(coffset);
                chunks_[offset] = data;
                return error::none;
            }

            // If overlap partially and the old data is "below", overwrite the old data, append new data
            if (coffset <= offset && cend >= offset) {
                cdata.resize(cdata.size() + end - cend);
                std::copy(data.begin(), data.end(), cdata.end() - data.size());
                return error::none;
            }

            // If overlap partially and the old data is "above", create a new chunk and append the old data
            if (coffset <= end && cend >= end) {
                auto additional = cend - end;
                chunks_[offset] = data;
                chunks_[offset].resize(data.size() + additional);
                std::copy(cdata.begin() + (cdata.size() - additional), cdata.end(), chunks_[offset].end() - additional);
                return error::none;
            }
        }

        // If we get here, we can just add the new data
        chunks_[offset] = data;
        return error::none;
    }

    /**
     * @brief Erase data from the device
     * 
     * @param offset - offset in the device
     * @param size - size of the data to erase
     * @return error - error code
     */
    virtual error erase(size_t offset, size_t size) override {
        if (offset + size > size_) {
            return error::out_of_bounds;
        }

        // Remove all chunks that are in the range
        auto it = chunks_.lower_bound(offset);
        while (it != chunks_.end() && it->first < offset + size) {
            it = chunks_.erase(it);
        }

        return error::none;
    }

    /**
     * @brief Get the size of the device
     * 
     * @return size_t - size of the device in bytes
     */
    virtual size_t size() const override {
        return size_;
    }

    /**
     * @brief Get the number of used chunks
     * 
     * @return size_t - number of used chunks
     */
    size_t used_chunks() const {
        return chunks_.size();
    }

    /**
     * @brief Get the data
     * 
     * @return * const std::map<size_t, std::vector<uint8_t>>& - data
     */
    const std::map<size_t, std::vector<uint8_t>>& data() const {
        return chunks_;
    }

  private:
    /// @brief Size of the device
    std::size_t size_;

    /// @brief map of chunks that are not 0xff
    std::map<size_t, std::vector<uint8_t>> chunks_;
};