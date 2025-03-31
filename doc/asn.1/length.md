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



```
