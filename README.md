# AkiDesk

一款基于 Qt6 开发的 AI 桌宠应用，支持 AI 实时对话、语音交互、立绘动画展示和对话控制打开应用功能。

## 功能特性

### 核心功能

- **立绘展示** - 支持自定义角色立绘，可拖拽移动，支持透明无边框窗口
- **AI 对话** - 支持 OpenAI 兼容 API（如 DeepSeek、通义千问等），支持流式响应和多轮对话
- **Token 优化** - 智能历史对话管理，自动裁剪旧对话，控制单次请求 token 数量
- **语音合成** - 集成 VITS 语音合成引擎，支持按句合成和队列播放
- **语音识别** - 集成百度语音识别 API，支持实时语音输入
- **动画系统** - 支持插件化动画扩展，可自定义动画效果
- **对话控制** - AI 可控制打开网站和应用程序（通过桌面/开始菜单快捷方式）
- **丰富设置** - 提供角色、LLM、VITS、STT 等多项配置

### 特色亮点

- **多角色支持** - 可创建多个角色，每个角色拥有独立的立绘、Prompt 和对话上下文
- **心情联动** - AI 回复可自动触发不同的立绘动画效果
- **系统托盘** - 后台运行，右键托盘图标快速访问设置
- **跨平台** - 支持 Windows 和 Linux 平台

## 技术栈

- **框架**: Qt 6.2+ (Widgets, Network, Multimedia, Svg)
- **语言**: C++17
- **构建工具**: CMake 3.16+
- **平台**: Windows / Linux

## 项目结构

```
AkiDesk/
├── assets/                    # 资源文件
│   ├── default_config/        # 默认配置和角色资产
│   │   ├── AkiDesk/
│   │   │   ├── Character/     # 默认角色（Atri）
│   │   │   │   ├── Atri/
│   │   │   │   │   ├── Tachie/
│   │   │   │   │   │   └── default.png
│   │   │   │   │   ├── config.json
│   │   │   │   │   └── context.json
│   │   │   │   └── Config/
│   │   │   │       └── config.json
│   │   │   └── Plugin/
│   │   │       └── Anime/
│   │   │           └── Basic Animation Package.json
│   │   └── config.ini         # 默认配置文件
│   ├── logo.png               # 应用图标
│   └── resources.qrc          # Qt 资源文件
├── config/                    # 配置管理模块
│   ├── AppPaths.h             # 应用路径常量
│   └── JsonConfig.cpp/h       # JSON 配置解析（支持嵌套路径）
├── core/                      # 核心功能模块
│   ├── AiProvider.cpp/h       # AI 对话接口（OpenAI 兼容，支持多轮对话）
│   ├── VitsEngine.cpp/h       # VITS 语音合成引擎
│   ├── SpeechRecognizer.cpp/h # 百度语音识别
│   └── CommandExecutor.cpp/h  # 命令执行器（网站/应用控制）
├── ui/                        # 用户界面模块
│   ├── characterwindow/       # 立绘窗口（透明、可拖拽、无边框）
│   ├── chatdialog/            # 聊天对话框
│   │   ├── chatdialog.*       # 主对话框
│   │   └── historypanel.*     # 历史记录面板
│   └── settingswindow/        # 设置窗口
│       └── pages/             # 各设置页面
│           ├── page_llm.*     # LLM 配置页
│           ├── page_character.* # 角色配置页
│           ├── page_vits.*    # VITS 配置页
│           ├── page_stt.*     # 语音识别配置页
│           ├── page_plugin.*  # 插件配置页
│           └── page_about.*   # 关于页面
├── utils/                     # 工具类
│   ├── DragHelper.cpp/h       # 窗口拖拽辅助
│   ├── ScrollHelper.cpp/h     # 滚动辅助
│   ├── AnimePlugin.cpp/h      # 动画插件实现
│   └── PluginManager.cpp/h    # 插件管理器
├── installer/                 # 安装包配置
│   ├── AkiDesk.iss            # Inno Setup 脚本
│   └── AkiDesk.iss.in         # 脚本模板
├── main.cpp                   # 应用程序入口
├── CMakeLists.txt             # CMake 构建配置
└── app_version.h.in           # 版本信息模板
```

## 编译说明

### 依赖项

- **Qt 6.2+** - 需要以下模块：
  - Qt Widgets
  - Qt Network
  - Qt Multimedia
  - Qt Svg
- **CMake 3.16+**
- **C++17 编译器**
  - Windows: MSVC 2019+ 或 MinGW
  - Linux: GCC 9+ 或 Clang 10+

## 使用说明

### 首次运行

应用首次启动时，会自动将默认配置部署到用户文档目录：

- **Windows**: `C:\Users\<用户名>\Documents\AkiDesk\`
- **Linux**: `~/Documents/AkiDesk/`

默认包含一个示例角色 **Atri（亚托莉）**，可直接开始对话。

### 配置文件说明

应用运行后会在用户文档目录生成以下配置文件：

```
~/Documents/AkiDesk/
├── config.json              # 全局配置（API Key 等，可迁移）
├── config.ini               # 本地配置（窗口位置等，不可迁移）
├── Character/               # 角色资产目录
│   ├── Atri/                # 默认角色
│   │   ├── config.json      # 角色配置（Prompt、动画绑定）
│   │   ├── context.json     # 对话上下文（结构化存储）
│   │   └── Tachie/          # 立绘图片目录
│   │       └── default.png  # 默认立绘
│   └── Config/              # 用户运行配置
│       └── config.json      # 当前角色设置（立绘大小、模型选择等）
└── Plugin/                  # 插件目录
    └── Anime/               # 动画插件
        └── Basic Animation Package.json
```

### 角色配置

每个角色位于 `Character/<角色名>/` 目录下，包含：

#### `config.json` - 角色配置

```json
{
  "prompt": "你是一个活泼的高中女生...",
  "tachieAnimations": {
    "default": "Basic Animation Package_轻微放大缩小",
    "happy": "Basic Animation Package_上抬下落",
    "angry": "Basic Animation Package_轻微放大缩小",
    "shy": "Basic Animation Package_轻微淡入淡出"
  }
}
```

- `prompt`: 角色 Prompt，定义角色的性格和行为
- `tachieAnimations`: 心情与动画的映射关系

#### `context.json` - 对话上下文

存储对话历史，采用结构化格式：

```json
{
  "history": [
    {"role": "user", "content": "你好"},
    {"role": "assistant", "content": "你好呀！"}
  ]
}
```

可手动编辑或删除以重置对话。

#### `Tachie/` - 立绘图片

支持 PNG 格式，必须使用透明背景的立绘图片。

### AI 对话系统

#### Token 优化策略

为节省 API 费用，应用采用以下优化策略：

- **轮数限制**: 最多保留最近 6 轮对话
- **Token 预算**: 单次请求总 token 不超过 3000
- **智能裁剪**: 超过预算时自动丢弃最旧的历史消息
- **精简提示词**: 系统提示词压缩为简洁指令

#### 输出格式

AI 回复采用严格的管道分隔格式：

```
心情|中文|日语|||COMMAND:type:value
```

- **心情**: 从角色配置的心情列表中选择（如 happy、angry、shy）
- **中文**: AI 的中文回复内容
- **日语**: 中文内容的日语翻译（用于 VITS 语音合成）
- **COMMAND**: 可选的命令执行部分

#### 支持的命令

| 命令类型 | 格式 | 示例 |
|----------|------|------|
| 打开应用 | `COMMAND:openapp:应用名` | `COMMAND:openapp:网易云音乐` |
| 打开网站 | `COMMAND:openurl:网址/快捷名` | `COMMAND:openurl:b站` |
| 搜索 | `COMMAND:search:搜索内容` | `COMMAND:search:人工智能` |

### 动画插件系统

动画插件位于 `Plugin/Anime/` 目录，JSON 格式定义：

```json
{
  "name": "Basic Animation Package",
  "version": "1.0.0",
  "author": "AkiDesk",
  "animations": [
    {
      "name": "轻微放大缩小",
      "steps": [
        {
          "type": "scale",
          "duration": 0.25,
          "from": 1.0,
          "to": 1.08
        },
        {
          "type": "scale",
          "duration": 0.25,
          "from": 1.08,
          "to": 1.0
        }
      ]
    }
  ]
}
```

支持的动画类型：
- `move`: 移动动画（x, y 坐标）
- `scale`: 缩放动画（from, to 缩放比例）
- `opacity`: 透明度动画（from, to 透明度值）

### API 配置

#### LLM 配置

支持 OpenAI 兼容的 API，推荐使用：

- **DeepSeek**: `https://api.deepseek.com/v1`
- **本地部署**: Ollama、vLLM 等

在设置页面配置：
1. API URL（如 `https://api.deepseek.com/v1`）
2. API Key
3. 模型名称（如 `deepseek-chat`）

#### VITS 配置

需要部署 VITS API 服务，推荐项目：
- [vits-simple-api](https://github.com/Artrajz/vits-simple-api)

配置参数：
- API URL
- 模型名称
- 说话人 ID

#### 语音识别配置

使用百度语音识别 API：
1. 注册百度智能云账号
2. 创建语音识别应用
3. 获取 API Key 和 Secret Key

### 应用启动机制

应用通过搜索桌面和开始菜单快捷方式来启动应用程序：

- **搜索路径**:
  - 桌面目录
  - 用户开始菜单程序目录
  - 公共开始菜单程序目录
- **匹配策略**:
  - 优先精确匹配
  - 其次包含匹配
  - 最后字符全匹配
- **过滤规则**: 自动过滤包含"卸载"、"uninstall"、"remove"等关键词的快捷方式

### 快捷操作

- **右键立绘**: 显示/隐藏对话框
- **左键托盘图标**: 打开设置
- **右键托盘图标**: 显示菜单（设置、退出）

## 常见问题

### Q: 立绘窗口无法显示？

A: 检查立绘图片是否存在于 `Character/<角色名>/Tachie/` 目录，确保图片格式为 PNG 且包含透明背景。

### Q: AI 对话无响应？

A: 检查以下项目：
- API URL 是否正确
- API Key 是否有效
- 网络连接是否正常
- 是否触发了请求超时

### Q: 语音合成无法工作？

A: 确保：
- VITS API 服务已正确部署
- API URL 可访问
- 说话人 ID 正确

### Q: 如何添加新角色？

A: 在 `Character/` 目录下创建新的角色文件夹，包含：
- `config.json` - 角色配置
- `context.json` - 对话上下文（可为空对象 `{}`）
- `Tachie/` - 立绘图片目录

### Q: 如何自定义动画？

A: 在 `Plugin/Anime/` 目录下创建新的 JSON 文件，按照动画插件格式定义动画效果。

### Q: 如何清理对话历史？

A: 删除 `Character/<角色名>/context.json` 文件，下次启动时会自动创建空的上下文。

## 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。

## 贡献

欢迎提交 Issue 和 Pull Request！
