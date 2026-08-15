# AkiDesk

一款基于 Qt6 开发的 AI 桌宠应用，支持 AI 实时对话、语音交互、立绘动画展示和对话控制打开应用功能。

## 功能特性

### 核心功能

- **立绘展示** - 支持自定义角色立绘，可拖拽移动，支持透明无边框窗口
- **AI 对话** - 支持 OpenAI 兼容 API（如 DeepSeek、通义千问等），支持流式响应和多轮对话
- **Token 优化** - 智能历史对话管理，自动裁剪旧对话，控制单次请求 token 数量
- **启动招呼** - 应用启动后根据当前时段（早/中/下午/晚/深夜）自动发送一次招呼
- **语音合成** - 集成 VITS 语音合成引擎，支持按句合成和队列播放
- **语音识别** - 集成百度语音识别 API，支持实时语音输入
- **动画系统** - 支持插件化动画扩展，可自定义动画效果
- **对话控制** - AI 可控制打开网站和应用程序（通过桌面/开始菜单快捷方式）
- **心情联动** - AI 回复可自动触发不同的立绘图片与动画效果，并支持中文心情名容错映射
- **丰富设置** - 提供角色、LLM、VITS、STT、通用等多项配置

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
│   │   └── config.ini         # 默认本地配置文件
│   ├── logo.png               # 应用图标
│   └── resources.qrc          # Qt 资源文件
├── config/                    # 配置管理模块
│   ├── AppPaths.h             # 应用路径常量
│   └── JsonConfig.cpp/h       # JSON 配置解析（支持嵌套路径）
├── core/                      # 核心功能模块
│   ├── AiProvider.cpp/h       # AI 对话接口（OpenAI 兼容，支持流式与多轮）
│   ├── VitsEngine.cpp/h       # VITS 语音合成引擎
│   ├── SpeechRecognizer.cpp/h # 百度语音识别
│   └── CommandExecutor.cpp/h  # 命令执行器（网站/应用控制）
├── ui/                        # 用户界面模块
│   ├── characterwindow/       # 立绘窗口（透明、可拖拽、无边框）
│   ├── chatdialog/            # 聊天对话框
│   │   ├── chatdialog.*       # 主对话框（含启动招呼）
│   │   └── historypanel.*     # 历史记录面板
│   └── settingswindow/        # 设置窗口
│       └── pages/             # 各设置页面
│           ├── page_general.* # 通用配置页
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
- Linux 额外依赖：X11（`libx11-dev`、`libxext-dev`）

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
├── config.ini               # 本地配置（当前选中角色、窗口位置等，不可迁移）
├── Character/               # 角色资产目录
│   ├── Atri/                # 默认角色
│   │   ├── config.json      # 角色配置（Prompt、动画绑定）
│   │   ├── context.json     # 对话上下文（结构化存储）
│   │   └── Tachie/          # 立绘图片目录
│   │       └── default.png  # 默认立绘
│   └── Config/              # 用户运行配置
│       └── config.json      # 当前角色设置（立绘大小、模型选择、VITS 开关等）
└── Plugin/                  # 插件目录
    └── Anime/               # 动画插件
        └── Basic Animation Package.json
```

> 注意：应用不在根目录存放全局 config.json，用户可迁移的配置统一放在 `Character/Config/config.json`。

### 角色配置

每个角色位于 `Character/<角色名>/` 目录下，包含：

#### `config.json` - 角色配置

```json
{
  "prompt": "以下是一个初步的角色定位……",
  "tachieAnimations": {
    "default": "Basic Animation Package_轻微放大缩小",
    "happy": "Basic Animation Package_上抬下落",
    "angry": "Basic Animation Package_轻微放大缩小",
    "shy": "Basic Animation Package_轻微淡入淡出"
  }
}
```

- `prompt`: 角色 Prompt，定义角色的性格和行为
- `tachieAnimations`: 心情与动画的映射关系，key 为心情名（对应 `Tachie/` 下的图片名），value 为动画插件名_动画名

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

支持 PNG 格式，必须使用透明背景的立绘图片。文件名即心情名，例如：

- `default.png` - 默认/平静
- `happy.png` - 开心
- `angry.png` - 愤怒
- `shy.png` - 害羞

启动招呼与 AI 回复中的心情会触发对应图片切换；若图片不存在则回退到 `default.png`。

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

> 启用 VITS 前需在「角色设置」中选择对应的 VITS 模型（speaker）。若未配置 API URL 或说话人，合成请求会被拦截并提示。

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
- **过滤规则**: 自动过滤包含"卸载"、"uninstall"、"remove"、"删除"等关键词的快捷方式

## 常见问题

### Q: 立绘窗口无法显示？

A: 检查立绘图片是否存在于 `Character/<角色名>/Tachie/` 目录，确保图片格式为 PNG 且包含透明背景。

### Q: 立绘心情一直不变？

A: 检查 `Tachie/` 目录下是否有对应心情名的图片（如 `happy.png`、`angry.png`）。若只有 `default.png`，即使 AI 正确输出心情名也会回退到默认立绘。

### Q: AI 对话无响应？

A: 检查以下项目：
- API URL 是否正确
- API Key 是否有效
- 网络连接是否正常
- 是否触发了请求超时（默认 60 秒）

### Q: 语音合成无法工作？

A: 确保：
- 在「角色设置」中已勾选「启用 VITS」并选择了 VITS 模型
- VITS API 服务已正确部署且可访问
- 说话人 ID 正确
- AI 回复包含日语段（若缺失会用中文兜底合成，但效果取决于 VITS 模型是否支持中文）

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
