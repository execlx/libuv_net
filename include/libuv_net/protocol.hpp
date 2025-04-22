#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include "libuv_net/message.hpp"

namespace libuv_net {

// 协议版本
constexpr uint8_t PROTOCOL_VERSION = 1;

// 协议头部大小
constexpr size_t PROTOCOL_HEADER_SIZE = 6; // 1字节版本 + 5字节消息头部

// 协议头部
struct ProtocolHeader {
    uint8_t version;
    MessageType type;
    uint32_t length;
};

// 协议消息
struct ProtocolMessage {
    ProtocolHeader header;
    std::vector<uint8_t> payload;
};

// 序列化协议头部
std::vector<uint8_t> serialize_header(const ProtocolHeader& header);

// 反序列化协议头部
ProtocolHeader deserialize_header(const std::vector<uint8_t>& buffer);

// 序列化协议消息
std::vector<uint8_t> serialize_message(const ProtocolMessage& message);

// 反序列化协议消息
ProtocolMessage deserialize_message(const std::vector<uint8_t>& buffer);

} // namespace libuv_net 