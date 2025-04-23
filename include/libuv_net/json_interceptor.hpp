#pragma once

#include "libuv_net/message.hpp"
#include <nlohmann/json.hpp>

namespace libuv_net
{
    /**
     * @brief JSON拦截器
     *
     * 用于处理JSON格式的消息
     */
    class JsonInterceptor : public Interceptor
    {
    public:
        // 构造函数
        JsonInterceptor() = default;

        // 序列化数据
        std::vector<uint8_t> serialize(const std::any &data) override
        {
            try
            {
                // 获取JSON对象
                const auto &json = std::any_cast<nlohmann::json>(data);

                // 转换为字符串
                std::string json_str = json.dump();

                // 转换为字节向量
                return std::vector<uint8_t>(json_str.begin(), json_str.end());
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
                std::string json_str(data.begin(), data.end());

                // 解析JSON
                auto json = nlohmann::json::parse(json_str);

                return json;
            }
            catch (const nlohmann::json::parse_error &)
            {
                return std::any();
            }
        }

        // 获取支持的消息类型
        PacketType get_type() const override
        {
            return PacketType::JSON;
        }
    };

} // namespace libuv_net