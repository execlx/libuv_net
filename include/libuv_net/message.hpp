#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <functional>
#include <string>
#include <any>
#include <map>

namespace libuv_net
{
    // 协议版本
    constexpr uint8_t PROTOCOL_VERSION = 1;

    // 消息类型枚举
    enum class PacketType : uint8_t
    {
        TEXT = 0,      // 文本消息
        BINARY = 1,    // 二进制消息
        PING = 2,      // 心跳请求
        PONG = 3,      // 心跳响应
        HEARTBEAT = 4, // 心跳包
        JSON = 5,      // JSON消息
        PROTOBUF = 6   // Protobuf消息
    };

    // 消息头结构
    struct PacketHeader
    {
        uint8_t version;   // 协议版本
        PacketType type;   // 消息类型
        uint32_t length;   // 消息长度
        uint32_t sequence; // 序列号
    };

    // 心跳相关常量
    constexpr int HEARTBEAT_INTERVAL_MS = 30000; // 30秒
    constexpr int HEARTBEAT_TIMEOUT_MS = 90000;  // 90秒

    // 拦截器接口
    class Interceptor
    {
    public:
        virtual ~Interceptor() = default;

        // 序列化数据
        virtual std::vector<uint8_t> serialize(const std::any &data) = 0;

        // 反序列化数据
        virtual std::any deserialize(const std::vector<uint8_t> &data) = 0;

        // 获取支持的消息类型
        virtual PacketType get_type() const = 0;
    };

    /**
     * @brief 数据包类
     *
     * 用于在网络中传输的数据包，包含：
     * - 协议版本
     * - 消息类型
     * - 消息内容
     * - 序列号
     */
    class Packet
    {
    public:
        // 默认构造函数
        Packet() : type_(PacketType::TEXT), data_(), sequence_(0) {}

        // 带参数的构造函数
        Packet(PacketType type, std::vector<uint8_t> data, uint32_t sequence = 0)
            : type_(type), data_(std::move(data)), sequence_(sequence) {}

        // 获取消息类型
        PacketType type() const { return type_; }

        // 设置消息类型
        void set_type(PacketType type) { type_ = type; }

        // 获取消息数据
        const std::vector<uint8_t> &data() const { return data_; }

        // 设置消息数据
        void set_data(std::vector<uint8_t> data) { data_ = std::move(data); }

        // 获取序列号
        uint32_t sequence() const { return sequence_; }

        // 设置序列号
        void set_sequence(uint32_t sequence) { sequence_ = sequence; }

        // 序列化消息
        std::vector<uint8_t> serialize() const
        {
            std::vector<uint8_t> result;
            result.reserve(sizeof(PacketHeader) + data_.size());

            // 添加消息头
            PacketHeader header{
                PROTOCOL_VERSION,
                type_,
                static_cast<uint32_t>(data_.size()),
                sequence_};
            result.insert(result.end(),
                          reinterpret_cast<const uint8_t *>(&header),
                          reinterpret_cast<const uint8_t *>(&header) + sizeof(PacketHeader));

            // 添加消息数据
            result.insert(result.end(), data_.begin(), data_.end());

            return result;
        }

        // 反序列化消息
        bool deserialize(const uint8_t *data, size_t length)
        {
            if (length < sizeof(PacketHeader))
            {
                return false;
            }

            // 解析消息头
            const PacketHeader *header = reinterpret_cast<const PacketHeader *>(data);

            // 检查版本兼容性
            if (header->version != PROTOCOL_VERSION)
            {
                return false;
            }

            type_ = header->type;
            sequence_ = header->sequence;

            // 解析消息数据
            size_t data_length = header->length;
            if (length < sizeof(PacketHeader) + data_length)
            {
                return false;
            }

            data_.assign(data + sizeof(PacketHeader),
                         data + sizeof(PacketHeader) + data_length);

            return true;
        }

    private:
        PacketType type_;           // 消息类型
        std::vector<uint8_t> data_; // 消息数据
        uint32_t sequence_;         // 序列号
    };

    // 数据包处理回调函数类型
    using PacketHandler = std::function<void(std::shared_ptr<Packet>)>;

    // 拦截器管理类
    class InterceptorManager
    {
    public:
        // 添加拦截器
        void add_interceptor(std::shared_ptr<Interceptor> interceptor)
        {
            interceptors_[interceptor->get_type()] = std::move(interceptor);
        }

        // 获取拦截器
        std::shared_ptr<Interceptor> get_interceptor(PacketType type)
        {
            auto it = interceptors_.find(type);
            if (it != interceptors_.end())
            {
                return it->second;
            }
            return nullptr;
        }

        // 序列化数据
        std::vector<uint8_t> serialize(PacketType type, const std::any &data)
        {
            auto interceptor = get_interceptor(type);
            if (interceptor)
            {
                return interceptor->serialize(data);
            }
            return std::vector<uint8_t>();
        }

        // 反序列化数据
        std::any deserialize(PacketType type, const std::vector<uint8_t> &data)
        {
            auto interceptor = get_interceptor(type);
            if (interceptor)
            {
                return interceptor->deserialize(data);
            }
            return std::any();
        }

    private:
        std::map<PacketType, std::shared_ptr<Interceptor>> interceptors_;
    };

} // namespace libuv_net