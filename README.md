# AkiDesk

一款基于 Qt6 开发的 AI 桌宠应用，支持实时聊天、语音交互和立绘动画展示。

## 功能特性

- **立绘展示** - 支持自定义角色立绘，支持拖拽移动
- **AI 对话** - 支持 OpenAI 兼容的 API，支持流式响应
- **语音合成** - 集成 VITS 语音合成引擎
- **语音识别** - 支持语音输入
- **动画系统** - 支持插件化动画扩展
- **丰富设置** - 提供角色、LLM、VITS、STT 等多项配置

## 技术栈

- **框架**: Qt 6 (Widgets, Network, Multimedia, Svg)
- **语言**: C++17
- **构建工具**: CMake
- **平台**: Windows / Linux

## 项目结构

```
AkiDesk/
├── assets/           # 资源文件
│   ├── default_config/  # 默认配置和角色资产
│   └── resources.qrc    # Qt 资源文件
├── config/           # 配置管理
│   ├── AppPaths.h      # 路径常量
│   └── JsonConfig.cpp  # JSON 配置解析
├── core/             # 核心模块
│   ├── AiProvider.cpp    # AI 对话接口
│   ├── VitsEngine.cpp    # 语音合成
│   └── SpeechRecognizer.cpp # 语音识别
├── ui/               # 界面模块
│   ├── characterwindow/   # 立绘窗口
│   ├── chatdialog/        # 聊天对话框
│   └── settingswindow/    # 设置窗口
├── utils/            # 工具类
│   ├── DragHelper.cpp     # 拖拽辅助
│   └── PluginManager.cpp  # 插件管理
├── installer/        # 安装包配置
└── main.cpp          # 主入口
```

## 编译说明

### 依赖项

- Qt 6.2+ (Widgets, Network, Multimedia, Svg)
- CMake 3.16+
- C++17 编译器

### 编译步骤

```bash
# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt6

# 编译
cmake --build . --config Release
```

### Windows 平台

需要额外链接 `dwmapi` 库以支持窗口效果。

### Linux 平台

需要安装 X11 开发库：

```bash
sudo apt install libx11-dev libxext-dev
```

## 使用说明

### 首次运行

应用首次启动时，会自动将默认配置部署到用户文档目录 `~/Documents/AkiDesk/`。

### 角色配置

角色资产位于 `~/Documents/AkiDesk/Character/` 目录下，每个角色包含：
- `config.json` - 角色配置
- `context.json` - 对话上下文
- `Tachie/` - 立绘图片目录

### 插件系统

动画插件位于 `~/Documents/AkiDesk/Plugin/Anime/` 目录，支持 JSON 格式的动画定义。

## 贡献

欢迎提交 Issue 和 Pull Request！