#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace libuv_net {

// 消息类型枚举
enum class MessageType : uint8_t {
    DATA = 0x01,    // 数据消息
    CONTROL = 0x02, // 控制消息
    HEARTBEAT = 0x03 // 心跳消息
};

// 协议消息结构
// | 长度(4字节) | 类型(1字节) | 负载(N字节) |
struct Message {
    uint32_t length; // 消息长度
    MessageType type; // 消息类型
    std::vector<uint8_t> payload; // 消息负载

    // 消息头大小
    static const size_t HEADER_SIZE = sizeof(uint32_t) + sizeof(MessageType);

    // 构造函数
    Message() : length(0), type(MessageType::DATA) {}
    Message(MessageType type, const std::vector<uint8_t>& data)
        : length(data.size()), type(type), payload(data) {}

    // 序列化消息到缓冲区
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer(HEADER_SIZE + length);
        // 写入长度
        buffer[0] = (length >> 24) & 0xFF;
        buffer[1] = (length >> 16) & 0xFF;
        buffer[2] = (length >> 8) & 0xFF;
        buffer[3] = length & 0xFF;
        // 写入类型
        buffer[4] = static_cast<uint8_t>(type);
        // 写入负载
        std::copy(payload.begin(), payload.end(), buffer.begin() + HEADER_SIZE);
        return buffer;
    }

    // 从缓冲区反序列化消息
    static std::shared_ptr<Message> deserialize(const std::vector<uint8_t>& buffer) {
        if (buffer.size() < HEADER_SIZE) {
            return nullptr;
        }

        auto msg = std::make_shared<Message>();
        // 读取长度
        msg->length = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
        
        // 验证长度
        if (buffer.size() < HEADER_SIZE + msg->length) {
            return nullptr;
        }

        // 读取类型
        msg->type = static_cast<MessageType>(buffer[4]);
        
        // 读取负载
        msg->payload.assign(buffer.begin() + HEADER_SIZE, 
                          buffer.begin() + HEADER_SIZE + msg->length);
        
        return msg;
    }
};

} // namespace libuv_net 