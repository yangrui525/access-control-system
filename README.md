# 门禁系统 (Access Control System)

一个支持 Windows 和 Linux 跨平台的多线程门禁管理系统，包括服务端、客户端和日志查询工具。

## 📋 项目组成

### 1. 🖥️ server.c - 门禁服务端

**功能：**
- 多线程 TCP 服务器，监听客户端连接
- 接收和验证刷卡记录（门号、人员ID、时间戳）
- 将刷卡记录保存到 CSV 数据库
- 返回开锁指令给客户端
- 线程安全的文件操作

**报文格式：**
```
接收: 门号(3位),人员ID(6位),时间戳(YYYY-MM-DD HH:MM:SS)
发送: ^^门号,UNLOCK,OPEN##
```

**使用示例：**
```bash
./server              # 监听 0.0.0.0:9001
./server 0.0.0.0 8080  # 监听 0.0.0.0:8080
```

---

### 2. 📱 client.c - 门禁终端客户端

**功能：**
- 模拟门禁终端，定期发送刷卡记录
- 自动连接和重连失败处理
- 解析服务端返回的开锁指令
- 支持自定义发送间隔

**使用示例：**
```bash
./client 4 105 127.0.0.1 9001
# 每4秒发送一次刷卡记录
# 门号: 105
# 连接到 127.0.0.1:9001

./client 3 201          # 默认连接 127.0.0.1:9001
```

**参数说明：**
- 间隔秒：发送刷卡记录的时间间隔（秒）
- 门号：3位数字，如 105、201 等
- 服务IP（可选）：服务器地址，默认 127.0.0.1
- 端口（可选）：服务器端口，默认 9001

---

### 3. 📊 query.c - 刷卡记录查询

**功能：**
- 读取服务端生成的 CSV 日志文件
- 支持指定展示条数
- 支持按门号筛选
- 格式化表格输出

**使用示例：**
```bash
./query                          # 显示最后30条记录
./query data/access_log.csv 20   # 显示最后20条记录
./query data/access_log.csv 50 105  # 显示门号105的最后50条记录
```

**CSV 文件格式：**
```csv
record_id,door_id,user_id,card_time,client_ip,server_recv_time,raw_msg
1,105,123456,2024-01-15 10:30:45,192.168.1.100,2024-01-15 10:30:46,105,123456,2024-01-15 10:30:45
```

---

## 🛠️ 编译和运行

### 前置条件

**Windows:**
- Visual Studio 2019 或更新版本
- CMake 3.10+

**Linux/macOS:**
- GCC 或 Clang
- CMake 3.10+
- pthread 库（通常已预装）

### 编译步骤

```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 生成构建文件
cmake ..

# 3. 编译
cmake --build .

# 或使用 make（Linux/macOS）
make
```

编译完成后，可执行文件位于 `build/bin/` 目录：
- `server` 或 `server.exe`
- `client` 或 `client.exe`  
- `query` 或 `query.exe`

---

## 🚀 快速开始

### 场景：在本地测试系统

**终端1 - 启动服务端：**
```bash
cd build/bin
./server
# 输出: 门禁服务端启动 0.0.0.0:9001 日志路径:data/access_log.csv
```

**终端2 - 启动客户端1（门号105）：**
```bash
cd build/bin
./client 3 105
# 每3秒发送一次刷卡记录
```

**终端3 - 启动客户端2（门号201）：**
```bash
cd build/bin
./client 5 201
# 每5秒发送一次刷卡记录
```

**终端4 - 查询日志：**
```bash
cd build/bin
./query
# 显示最新的30条刷卡记录

./query data/access_log.csv 50 105
# 显示门号105的最新50条记录
```

---

## 📁 项目结构

```
access-control-system/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 项目说明文档
├── .gitignore              # Git 忽略配置
└── src/
    ├── server.c           # 门禁服务端
    ├── client.c           # 门禁终端客户端
    └── query.c            # 刷卡记录查询工具

build/
└── bin/
    ├── server             # 编译后的服务端
    ├── client             # 编译后的客户端
    └── query              # 编译后的查询工具

data/
└── access_log.csv         # 刷卡记录日志（自动生成）
```

---

## 🔒 数据安全

- **线程安全**：使用互斥锁保护文件操作
- **报文验证**：严格验证刷卡记录格式
- **错误处理**：完整的连接失败处理和重试机制

---

## 🐛 故障排除

### 问题1：编译失败，提示找不到 pthread

**Linux/macOS 解决方案：**
```bash
sudo apt-get install libpthread-stubs0-dev  # Ubuntu/Debian
```

### 问题2：客户端无法连接到服务端

检查事项：
- 确保服务端正在运行
- 确保防火墙允许 9001 端口
- 确保使用了正确的服务器地址和端口

### 问题3：查询工具无法找到日志文件

- 确保服务端已至少处理过一条刷卡记录
- 检查 `data/access_log.csv` 文件是否存在
- 在服务端目录运行查询工具，或指定完整路径

---

## 📝 许可证

MIT License

---

## 👥 作者

Yangrui525

---

## 💡 扩展功能建议

- [ ] Web 管理界面
- [ ] 数据库集成（SQLite/MySQL）
- [ ] 刷脸识别功能
- [ ] 权限分级管理
- [ ] 访问时间段控制
- [ ] 告警通知系统
