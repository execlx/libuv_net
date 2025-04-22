#pragma once

#include <vector>
#include <cstdint>
#include <memory>

namespace libuv_net {

// 消息类型枚举
enum class MessageType : uint8_t {
    TEXT = 0,    // 文本消息
    BINARY = 1,  // 二进制消息
    PING = 2,    // 心跳请求
    PONG = 3     // 心跳响应
};

// 消息头结构
struct MessageHeader {
    MessageType type;    // 消息类型
    uint32_t length;     // 消息长度
};

/**
 * @brief 消息类
 * 
 * 用于在网络中传输的数据包，包含：
 * - 消息类型
 * - 消息内容
 */
class Message {
public:
    // 默认构造函数
    Message() : type_(MessageType::TEXT), data_() {}

    // 带参数的构造函数
    Message(MessageType type, std::vector<uint8_t> data)
        : type_(type), data_(std::move(data)) {}

    // 获取消息类型
    MessageType type() const { return type_; }

    // 获取消息数据
    const std::vector<uint8_t>& data() const { return data_; }

    // 序列化消息
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> result;
        result.reserve(sizeof(MessageHeader) + data_.size());
        
        // 添加消息头
        MessageHeader header{type_, static_cast<uint32_t>(data_.size())};
        result.insert(result.end(), 
                     reinterpret_cast<const uint8_t*>(&header),
                     reinterpret_cast<const uint8_t*>(&header) + sizeof(MessageHeader));
        
        // 添加消息数据
        result.insert(result.end(), data_.begin(), data_.end());
        
        return result;
    }

    // 反序列化消息
    bool deserialize(const uint8_t* data, size_t length) {
        if (length < sizeof(MessageHeader)) {
            return false;
        }

        // 解析消息头
        const MessageHeader* header = reinterpret_cast<const MessageHeader*>(data);
        type_ = header->type;
        
        // 解析消息数据
        size_t data_length = header->length;
        if (length < sizeof(MessageHeader) + data_length) {
            return false;
        }
        
        data_.assign(data + sizeof(MessageHeader), 
                    data + sizeof(MessageHeader) + data_length);
        
        return true;
    }

private:
    MessageType type_;              // 消息类型
    std::vector<uint8_t> data_;     // 消息数据
};

} // namespace libuv_net 