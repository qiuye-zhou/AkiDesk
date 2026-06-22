# AkiDesk

一款基于 Qt6 开发的 AI 桌宠应用，支持实时聊天、语音交互和立绘动画展示。

## 功能特性

### 核心功能

- **立绘展示** - 支持自定义角色立绘，可拖拽移动，支持透明窗口
- **AI 对话** - 支持 OpenAI 兼容 API（如 DeepSeek、通义千问等），支持流式响应
- **语音合成** - 集成 VITS 语音合成引擎，支持按句合成和队列播放
- **语音识别** - 集成百度语音识别 API，支持实时语音输入
- **动画系统** - 支持插件化动画扩展，可自定义动画效果
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
- **许可协议**: MIT License

## 项目结构

```
AkiDesk/
├── assets/                    # 资源文件
│   ├── default_config/        # 默认配置和角色资产
│   │   ├── AkiDesk/
│   │   │   ├── Character/     # 默认角色（Atri）
│   │   │   └── Plugin/         # 动画插件
│   │   └── config.ini          # 默认配置文件
│   ├── logo.png               # 应用图标
│   └── resources.qrc          # Qt 资源文件
├── config/                    # 配置管理模块
│   ├── AppPaths.h             # 应用路径常量
│   └── JsonConfig.cpp         # JSON 配置解析
├── core/                      # 核心功能模块
│   ├── AiProvider.cpp         # AI 对话接口（OpenAI 兼容）
│   ├── VitsEngine.cpp         # VITS 语音合成引擎
│   └── SpeechRecognizer.cpp   # 百度语音识别
├── ui/                        # 用户界面模块
│   ├── characterwindow/       # 立绘窗口（透明、可拖拽）
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
│   ├── DragHelper.cpp         # 窗口拖拽辅助
│   ├── ScrollHelper.cpp       # 滚动辅助
│   ├── AnimePlugin.cpp        # 动画插件实现
│   └── PluginManager.cpp      # 插件管理器
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
│   │   ├── context.json     # 对话上下文
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

存储对话历史，可手动编辑或删除以重置对话。

#### `Tachie/` - 立绘图片

支持 PNG 格式，建议使用透明背景的立绘图片。

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

### 快捷操作

- **右键立绘**: 显示/隐藏对话框
- **左键托盘图标**: 打开设置
- **右键托盘图标**: 显示菜单（设置、退出）

## 常见问题

### Q: 立绘窗口无法显示？

A: 检查立绘图片是否存在于 `Character/<角色名>/Tachie/` 目录，确保图片格式为 PNG。

### Q: AI 对话无响应？

A: 检查以下项目：
- API URL 是否正确
- API Key 是否有效
- 网络连接是否正常
- 查看应用日志获取详细错误信息

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

## 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。