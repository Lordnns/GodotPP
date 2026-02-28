#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

namespace qns_core {

  class QNSSerializer {
    std::vector<uint8_t> buffer;
  public:
    template<typename T>
    void write(T val) {
      const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&val);
      buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
    }

    void write_bytes(const uint8_t* data, size_t len) {
      buffer.insert(buffer.end(), data, data + len);
    }

    void write_string(const std::string& str) {
      write<uint32_t>(str.size());
      write_bytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }

    const std::vector<uint8_t>& get_buffer() const { return buffer; }
  };

  class QNSDeserializer {
    const uint8_t* data;
    size_t cursor = 0;
    size_t max_size;
  public:
    QNSDeserializer(const uint8_t* d, size_t size) : data(d), max_size(size) {}
        
    template<typename T>
    T read() {
      T val;
      std::memcpy(&val, data + cursor, sizeof(T));
      cursor += sizeof(T);
      return val;
    }

    const uint8_t* read_bytes(size_t len) {
      const uint8_t* ptr = data + cursor;
      cursor += len;
      return ptr;
    }

    std::string read_string() {
      uint32_t len = read<uint32_t>();
      std::string str(reinterpret_cast<const char*>(data + cursor), len);
      cursor += len;
      return str;
    }
  };
}