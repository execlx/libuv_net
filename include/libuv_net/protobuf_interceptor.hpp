#pragma once

#include "libuv_net/message.hpp"
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>

namespace libuv_net
{
    /**
     * @brief Protobuf拦截器
     *
     * 用于处理Protobuf格式的消息
     */
    class ProtobufInterceptor : public Interceptor
    {
    public:
        // 构造函数
        ProtobufInterceptor() = default;

        // 序列化数据
        std::vector<uint8_t> serialize(const std::any &data) override
        {
            try
            {
                // 获取Protobuf消息
                const auto &message = std::any_cast<std::shared_ptr<google::protobuf::Message>>(data);

                // 序列化消息
                std::string serialized;
                if (!message->SerializeToString(&serialized))
                {
                    return std::vector<uint8_t>();
                }

                // 转换为字节向量
                return std::vector<uint8_t>(serialized.begin(), serialized.end());
            }
            catch (const std::bad_any_cast &)
            {
                return std::vector<uint8_t>();
            }
        }

        // 反序列化数据
        std::any deserialize(const std::vector<uint8_t> &data) override
        {
            try
            {
                // 转换为字符串
                std::string serialized(data.begin(), data.end());

                // 创建动态消息工厂
                google::protobuf::DynamicMessageFactory factory;

                // 获取消息类型
                const google::protobuf::Descriptor *descriptor =
                    google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName("google.protobuf.Message");

                if (!descriptor)
                {
                    return std::any();
                }

                // 创建消息对象
                std::unique_ptr<google::protobuf::Message> message(factory.GetPrototype(descriptor)->New());

                // 解析消息
                if (!message->ParseFromString(serialized))
                {
                    return std::any();
                }

                return std::shared_ptr<google::protobuf::Message>(message.release());
            }
            catch (const std::exception &)
            {
                return std::any();
            }
        }

        // 获取支持的消息类型
        PacketType get_type() const override
        {
            return PacketType::PROTOBUF;
        }
    };

} // namespace libuv_net