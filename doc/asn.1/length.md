# 简化版ASN.1编解码

## 简介
> ASN.1是一种与平台无关的协议定义格式，其主要采用TLV、TV格式作为底层编码方式，用户可自定义协议，包括每个字段或者结构的tag、序列化后的大小端、甚至字段的长度，其支持自定义的类型嵌套；

## 原理
> 



## 示例

### BER协议定义
```cpp
SimpleProtocol DEFINITIONS ::= BEGIN
    Message ::= SEQUENCE {
        messageId    INTEGER,       -- 消息ID
        version      INTEGER,       -- 协议版本
        isEncrypted  BOOLEAN,       -- 加密标志
        payload      OCTET STRING   -- 有效载荷
    }
END
```

### 编解码
```cpp

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace BER {
    // BER 类型标签 (Class = UNIVERSAL)
    enum Tag : uint8_t {
        SEQUENCE      = 0x30,
        INTEGER       = 0x02,
        BOOLEAN       = 0x01,
        OCTET_STRING  = 0x04
    };

    // 协议消息结构
    struct Message {
        int32_t messageId;
        int32_t version;
        bool isEncrypted;
        std::vector<uint8_t> payload;
    };

    // 编码长度字段 (BER规范)
    void encodeLength(std::vector<uint8_t> &output, size_t length) {
        if (length < 128) {
            output.push_back(static_cast<uint8_t>(length));
        } else {
            size_t numBytes = 0;
            size_t temp = length;
            while (temp != 0) {
                temp >>= 8;
                numBytes++;
            }
            if (numBytes > 127) throw std::runtime_error("Length too large");
            output.push_back(0x80 | static_cast<uint8_t>(numBytes));
            for (int i = numBytes - 1; i >= 0; --i) {
                output.push_back((length >> (8 * i)) & 0xFF);
            }
        }
    }

    // 解码长度字段
    size_t decodeLength(const uint8_t *&data) {
        if (*data < 128) return *data++;

        const uint8_t numBytes = *data++ & 0x7F;
        if (numBytes > sizeof(size_t)) throw std::runtime_error("Length field too large");

        size_t length = 0;
        for (uint8_t i = 0; i < numBytes; ++i) {
            length = (length << 8) | *data++;
        }
        return length;
    }

    // 修正后的整数编码（符合BER规范）
    void encodeInteger(std::vector<uint8_t> &output, int32_t value) {
        std::vector<uint8_t> bytes;
        bool isNegative = value < 0;
        uint32_t uvalue = static_cast<uint32_t>(value);

        // 生成原始字节（大端序）
        for (int i = 0; i < 4; ++i) {
            bytes.insert(bytes.begin(), (uvalue & 0xFF));
            uvalue >>= 8;
        }

        // 去除前导冗余字节
        auto it = bytes.begin();
        if (isNegative) {
            // 负数的冗余前导0xFF
            while (it + 1 != bytes.end() && *it == 0xFF && (*(it + 1) & 0x80)) {
                ++it;
            }
        } else {
            // 正数的冗余前导0x00
            while (it + 1 != bytes.end() && *it == 0x00 && !(*(it + 1) & 0x80)) {
                ++it;
            }
        }
        bytes.erase(bytes.begin(), it);

        // 检查是否需要添加符号位扩展
        if ((bytes.front() & 0x80) && !isNegative) {
            bytes.insert(bytes.begin(), 0x00); // 正数需要避免符号位为1
        } else if (!(bytes.front() & 0x80) && isNegative) {
            bytes.insert(bytes.begin(), 0xFF); // 负数需要符号位扩展
        }

        output.push_back(INTEGER);
        encodeLength(output, bytes.size());
        output.insert(output.end(), bytes.begin(), bytes.end());
    }

    // 修正后的整数解码
    int32_t decodeInteger(const uint8_t *&data) {
        if (*data++ != INTEGER) throw std::runtime_error("Expected INTEGER tag");

        size_t length = decodeLength(data);
        if (length > 4) throw std::runtime_error("Integer overflow");

        int32_t value = 0;
        bool isNegative = (data[0] & 0x80);

        // 符号位扩展
        if (isNegative) value = -1;
        for (size_t i = 0; i < length; ++i) {
            value = (value << 8) | *data++;
        }

        return value;
    }

    // 编码布尔值
    void encodeBoolean(std::vector<uint8_t> &output, bool value) {
        output.push_back(BOOLEAN);
        output.push_back(0x01);  // 长度总是1
        output.push_back(value ? 0xFF : 0x00);
    }

    // 解码布尔值
    bool decodeBoolean(const uint8_t *&data) {
        if (*data++ != BOOLEAN) {
            throw std::runtime_error("Expected BOOLEAN tag");
        }

        if (*data++ != 0x01) {
            throw std::runtime_error("Invalid BOOLEAN length");
        }

        return *data++ != 0x00;
    }

    // 编码OCTET STRING
    void encodeOctetString(std::vector<uint8_t> &output, const std::vector<uint8_t> &data) {
        output.push_back(OCTET_STRING);
        encodeLength(output, data.size());
        output.insert(output.end(), data.begin(), data.end());
    }

    // 解码OCTET STRING
    std::vector<uint8_t> decodeOctetString(const uint8_t *&data) {
        if (*data++ != OCTET_STRING) {
            throw std::runtime_error("Expected OCTET_STRING tag");
        }

        size_t length = decodeLength(data);
        std::vector<uint8_t> result(data, data + length);
        data += length;
        return result;
    }

    // 编码完整消息
    std::vector<uint8_t> encode(const Message &msg) {
        std::vector<uint8_t> innerData;

        // 按顺序编码各字段
        encodeInteger(innerData, msg.messageId);
        encodeInteger(innerData, msg.version);
        encodeBoolean(innerData, msg.isEncrypted);
        encodeOctetString(innerData, msg.payload);

        // 构建外层SEQUENCE
        std::vector<uint8_t> result;
        result.push_back(SEQUENCE);
        encodeLength(result, innerData.size());
        result.insert(result.end(), innerData.begin(), innerData.end());

        return result;
    }

    // 解码完整消息
    Message decode(const uint8_t *data, size_t length) {
        const uint8_t* end = data + length;
        Message msg;

        // 检查外层SEQUENCE
        if (*data++ != SEQUENCE) {
            throw std::runtime_error("Expected SEQUENCE tag");
        }

        size_t seqLength = decodeLength(data);
        if (data + seqLength > end) {
            throw std::runtime_error("Invalid SEQUENCE length");
        }

        // 解码各字段
        msg.messageId = decodeInteger(data);
        msg.version = decodeInteger(data);
        msg.isEncrypted = decodeBoolean(data);
        msg.payload = decodeOctetString(data);

        if (data != end) {
            throw std::runtime_error("Extra data after SEQUENCE");
        }

        return msg;
    }
}

// 测试用例
void testIntegerEncoding() {
    using namespace BER;

    struct TestCase {
        int32_t value;
        std::vector<uint8_t> expected;
    };

    TestCase testCases[] = {
        {0,         {0x02, 0x01, 0x00}},                // 零
        {1,         {0x02, 0x01, 0x01}},                // 小正数
        {127,       {0x02, 0x01, 0x7F}},                // 正数边界
        {128,       {0x02, 0x02, 0x00, 0x80}},         // 需要符号位扩展的正数
        {-1,        {0x02, 0x01, 0xFF}},                // -1
        {-128,      {0x02, 0x01, 0x80}},                // 负数边界
        {-129,      {0x02, 0x02, 0xFF, 0x7F}},         // 需要扩展的负数
        {32767,     {0x02, 0x02, 0x7F, 0xFF}},          // 最大两字节正数
        {-32768,    {0x02, 0x02, 0x80, 0x00}},          // 最小两字节负数
        {2147483647, {0x02, 0x04, 0x7F, 0xFF, 0xFF, 0xFF}}, // INT32_MAX
        {-2147483648, {0x02, 0x04, 0x80, 0x00, 0x00, 0x00}} // INT32_MIN
    };

    for (const auto& tc : testCases) {
        std::vector<uint8_t> buffer;
        encodeInteger(buffer, tc.value);

        if (buffer != tc.expected) {
            std::cerr << "Test failed for value " << tc.value << ":"<<std::endl;
            std::cerr << "Encoded: ";
            for (auto b : buffer) printf("%02X ", b);
            std::cerr << "\nExpected: ";
            for (auto b : tc.expected) printf("%02X ", b);
            std::cerr << std::endl;
            throw std::runtime_error("Integer encoding test failed");
        }

        // 验证解码
        const uint8_t* data = buffer.data();
        int32_t decoded = decodeInteger(data);
        if (decoded != tc.value) {
            std::cerr << "Decoding mismatch for value " << tc.value
                      << ", got " << decoded <<std::endl;
            throw std::runtime_error("Integer decoding test failed");
        }
    }
    std::cout << "All integer encoding/decoding tests passed!"<<std::endl;
}

int main() {
    try {
        testIntegerEncoding();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test failed: " << e.what() <<std::endl;
        return 1;
    }
}



```
