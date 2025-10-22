# STWINKT1B_VIBRATIONS

基于 STM32 和 NanoEdgeAI 的振动异常检测系统

## 项目简介

本项目是一个运行在 STEVAL-STWINKT1B 开发板上的**振动分析与异常检测**应用程序,利用 STMicroelectronics 的 NanoEdgeAI 技术实现基于机器学习的预测性维护。系统通过实时采集和分析振动数据,检测机械系统中的异常振动模式。

### 核心功能

- ✅ 实时振动数据采集与处理
- ✅ 基于 NanoEdgeAI 的机器学习异常检测
- ✅ 学习阶段(训练)+ 检测阶段(推理)两阶段运行
- ✅ USB 通信实现数据记录和结果输出
- ✅ LED 指示灯显示系统状态
- ✅ 可配置的阈值和运行模式

## 硬件平台

### 微控制器
- **型号**: STM32L4R9ZIJx
- **架构**: ARM Cortex-M4
- **系列**: STM32L4 超低功耗系列
- **浮点单元**: ARM FPv4-SP-D16 (硬件浮点)

### 开发板
- **型号**: STEVAL-STWINKT1B (STWIN Kit)
- **特点**: 多功能开发和原型设计板,适用于物联网和传感器应用
- **接口**: 集成多种传感器和通信接口

### 主传感器
- **型号**: ISM330DHCX (6 轴 MEMS IMU)
- **功能**: 3 轴加速度计 + 3 轴陀螺仪
- **通信接口**: SPI3 (通过 STMOD+ 引脚连接)
- **数据速率**: 833 Hz
- **量程**: ±2g 加速度范围
- **用途**: 高速振动数据采集

## 项目结构

```
STWINKT1B_VIBRATIONS/
├── Core/                              # 应用程序核心
│   ├── Inc/                          # 头文件
│   │   ├── main.h                    # 主应用程序头文件
│   │   ├── NanoEdgeAI.h             # NanoEdgeAI API 头文件
│   │   ├── stm32l4xx_hal_conf.h     # HAL 配置
│   │   └── stm32l4xx_it.h           # 中断处理程序
│   ├── Src/                          # 源文件
│   │   ├── main.c                    # 主应用程序 (~30 KB)
│   │   ├── libneai.a                # NanoEdgeAI 编译库 (14 KB)
│   │   ├── stm32l4xx_hal_msp.c      # HAL MSP 配置
│   │   ├── stm32l4xx_it.c           # 中断处理
│   │   ├── system_stm32l4xx.c       # 系统初始化
│   │   └── ...
│   └── Startup/                      # 启动代码
│
├── Drivers/                           # 硬件驱动
│   ├── ISM330DHCX/                   # IMU 传感器驱动
│   │   ├── ism330dhcx_reg.h         # 寄存器定义
│   │   └── ism330dhcx_reg.c         # 驱动实现
│   ├── STM32L4xx_HAL_Driver/        # STM32 HAL 驱动库
│   │   ├── Inc/                     # HAL 头文件 (UART, SPI, I2C, USB, GPIO, DMA 等)
│   │   └── Src/                     # HAL 实现文件
│   └── CMSIS/                        # ARM CMSIS 标准接口
│       ├── Device/ST/STM32L4xx/     # STM32L4 设备相关头文件
│       └── Include/                  # ARM Cortex-M4 通用头文件
│
├── Middlewares/                       # ST 中间件
│   └── ST/STM32_USB_Device_Library/ # USB 设备协议栈
│       ├── Core/                    # USB 核心
│       └── Class/CDC/               # CDC 类 (通信设备类)
│
├── USB_DEVICE/                        # USB 应用层
│   ├── App/                          # USB 应用
│   │   ├── usb_device.c/h           # USB 设备初始化
│   │   ├── usbd_cdc_if.c/h          # CDC 接口
│   │   └── usbd_desc.c/h            # USB 描述符
│   └── Target/                       # USB 目标配置
│
├── Debug/                             # 编译输出目录
│
├── 配置文件:
│   ├── STWINKT1B_VIBRATIONS.ioc      # STM32CubeIDE 设备配置
│   ├── STM32L4R9ZIJX_FLASH.ld       # Flash 链接脚本
│   ├── STM32L4R9ZIJX_RAM.ld         # RAM 链接脚本
│   └── .project / .cproject          # Eclipse 项目配置
│
└── README.md                          # 本文件
```

## 技术架构

### 数据流程

```
ISM330DHCX 传感器 (SPI3)
    ↓ (3 轴加速度数据 @ 833 Hz)
HAL SPI 驱动
    ↓
数据缓冲区: input_user_buffer[256×3] (浮点数组)
    ↓
NanoEdgeAI 库 (libneai.a)
    ↓ (学习或检测)
结果: 相似度分数 (0-100)
    ↓
USB 通信 (UART/USB-CDC)
    ↓
主机系统输出
```

### 核心技术栈

| 技术 | 说明 |
|------|------|
| **STM32 HAL** | 硬件抽象层驱动库 |
| **CMSIS** | ARM Cortex 微控制器软件接口标准 |
| **STM32 USB Device Library** | USB 协议栈实现 |
| **NanoEdgeAI** | STMicroelectronics 机器学习库 |
| **通信协议** | SPI, UART, USB 2.0 Full-Speed (CDC) |
| **开发环境** | STM32CubeIDE (基于 Eclipse) |
| **编译工具链** | GCC ARM (arm-none-eabi-gcc) |

### NanoEdgeAI 配置

- **库文件**: `libneai.a` (针对 STM32 优化的编译库)
- **模型 ID**: `62bf6a20a8dfcb0f23a6caa1`
- **输入数据**: 256 样本 × 3 轴 = 768 个浮点值
- **输出**: 相似度分数 (0-100)
- **最小学习迭代次数**: 10
- **实际学习样本数**: 50

## 应用逻辑

### 两阶段运行模式

#### 1. 学习阶段 (Learning Phase)
- **目的**: 采集正常/参考振动模式
- **样本数**: 50 个学习样本
- **函数**: `neai_anomalydetection_learn()`
- **LED 指示**: LED2 亮起

#### 2. 检测阶段 (Detection Phase)
- **目的**: 分析振动数据并检测异常
- **函数**: `neai_anomalydetection_detect()`
- **推理平均**: 3 次连续推理的平均值
- **输出**: 相似度分数 (0-100)

### 状态指示

| LED | 颜色 | 含义 |
|-----|------|------|
| LED1 | 绿色 | 正常振动模式 (相似度 ≥ 90%) |
| LED2 | 橙色 | 异常振动模式 (相似度 < 90%) |

### 关键参数配置 (main.c)

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `LEARN_NB` | 50 | 学习样本数量 |
| `NEAI_MODE` | 0 | 0=数据记录, 1=异常检测 |
| `NEAI_NB_VERIFS` | 3 | 每次结果平均的推理次数 |
| `THRESHOLD` | 90 | 正常/异常判定阈值 |
| `COM_MODE` | 0 | 0=UART (ST-Link), 1=USB-CDC |
| 传感器 ODR | 833 Hz | 输出数据速率 |
| 传感器量程 | 2g | 加速度测量范围 |

## 编译与运行

### 开发环境要求
- **IDE**: STM32CubeIDE (推荐最新版本)
- **调试器**: ST-Link/V2 或集成在开发板上的 ST-Link
- **操作系统**: Windows / Linux / macOS

### 编译步骤

1. **导入项目**
   ```
   File → Import → Existing Projects into Workspace
   选择 STWINKT1B_VIBRATIONS 文件夹
   ```

2. **构建项目**
   ```
   Project → Build Project
   或使用快捷键: Ctrl+B
   ```

3. **下载到开发板**
   ```
   Run → Debug 或 Run
   ```

### 编译配置

- **微控制器**: STM32L4R9ZIJx
- **浮点单元**: ARM FPv4-SP-D16 (硬浮点)
- **优化级别**: None (调试模式, -g3)
- **宏定义**: `USE_HAL_DRIVER`, `STM32L4R9xx`, `DEBUG`

## 使用说明

### 基本操作流程

1. **连接硬件**
   - 将 STWINKT1B 开发板通过 USB 连接到电脑
   - 确保 ST-Link 驱动已安装

2. **上电启动**
   - 系统自动进入学习阶段
   - LED2 (橙色) 亮起,开始采集正常振动数据

3. **学习阶段**
   - 让设备在**正常工作状态**下运行
   - 系统自动采集 50 个样本 (约 15 秒)
   - 完成后自动进入检测阶段

4. **检测阶段**
   - 系统持续监测振动模式
   - **绿色 LED1**: 振动正常
   - **橙色 LED2**: 检测到异常振动

5. **查看结果**
   - 通过串口终端 (UART2) 或 USB 虚拟串口查看输出
   - 波特率: 115200 (如使用 UART 模式)

### 串口监控

使用串口调试工具 (如 PuTTY, Tera Term, 或 Arduino Serial Monitor):
- **端口**: 查找 STM32 Virtual COM Port
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验**: None

## 技术亮点

1. **轻量级机器学习**: 使用 NanoEdgeAI 而非复杂神经网络,针对资源受限的微控制器优化

2. **实时处理**: 256 样本缓冲区 @ 833 Hz = 每 ~307 毫秒分析一次振动数据

3. **无监督学习**: 仅需正常工作状态的数据,无需标注异常样本

4. **双通信支持**: 支持 UART (开发) 和 USB-CDC (生产部署)

5. **低功耗设计**: 基于 STM32L4 超低功耗系列,适合电池供电应用

## 应用场景

- ⚙️ **工业设备监测**: 电机、泵、风机等旋转设备的振动监测
- 🏭 **预测性维护**: 提前发现设备异常,避免意外停机
- 🔧 **质量控制**: 生产线上产品的振动测试
- 🚗 **车辆诊断**: 汽车零部件的振动分析
- 🏗️ **结构健康监测**: 桥梁、建筑物的振动监测

## 文件统计

- **总源文件数**: 107 个 C/H 文件
- **项目总大小**: 99 MB (包含 HAL 驱动和库)
- **主应用程序**: ~30 KB (main.c)
- **ISM330DHCX 驱动**: ~11,414 行代码
- **NanoEdgeAI 库**: 14 KB (ARM 静态库)
- **核心应用逻辑**: ~859 行 (main.c)

## 学习资源

### 相关文档
- `../sujet_TP.pdf` - 实验指导书
- STWIN 开发板用户手册 (5.3 MB PDF)
- STM32CubeIDE 用户手册

### 参考链接
- [STM32L4 产品页](https://www.st.com/stm32l4)
- [STWINKT1B 开发板](https://www.st.com/stwinkt1b)
- [NanoEdgeAI Studio](https://www.st.com/nanoedgeai)
- [ISM330DHCX 数据手册](https://www.st.com/ism330dhcx)

## 许可证

本项目为学术用途,遵循 STMicroelectronics 软件许可协议。

## 作者

TPTR 课程实验项目 - TP_5GE_Nano_edge

---

**注意**: 本项目使用 STMicroelectronics 的专有 NanoEdgeAI 库,该库为编译后的二进制文件 (`libneai.a`)。如需生成自定义 AI 模型,请访问 [NanoEdgeAI Studio](https://www.st.com/nanoedgeai)。
