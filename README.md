# CppGallery

简体中文 | English

---

## 简体中文

### 这是什么

`CppGallery` 是一个面向 Windows 的看图软件，使用 `GLFW`、`Dear ImGui` 和 `OpenGL` 编写。  
它适合大批量图片浏览，重点优化了滚动流畅度、缩放响应和 GPU 侧纹理处理。

### README 是什么

是的，`README` 通常就是项目首页说明。  
对这个项目来说，它既是源码仓库说明，也是软件的操作说明。

### 最推荐的用法

这个软件有一个很实用的主流用法：

1. 把 `CppGallery.exe` 直接放进你的图片文件夹里。
2. 这个文件夹可以是：
   - 表情包文件夹
   - 风景图文件夹
   - 壁纸文件夹
   - 收藏图文件夹
3. 之后直接双击 `CppGallery.exe`，软件会优先打开它所在的文件夹。
4. 再把这个 `CppGallery.exe` 右键固定到开始屏幕或开始菜单。
5. 以后你就可以一键打开对应图片库，不用每次先进文件夹再找图。

这套用法的核心是：

- `CppGallery.exe` 放在哪个图包里，就优先浏览哪个图包
- 一个文件夹放一个 `CppGallery.exe`，就相当于给这个图片库做了一个独立入口

### 截图占位

公开发布时，建议把下面这些截图补到仓库里：

- `docs/screenshots/gallery-overview.png`：主图库总览
- `docs/screenshots/zoom-view.png`：单图放大查看
- `docs/screenshots/start-pinned-launcher.png`：固定到开始菜单 / 开始屏幕后的入口示例

截图占位说明见 `docs/screenshots/README.md`。

### 常见使用场景

- 表情包库：把 `CppGallery.exe` 放进表情包文件夹，固定到开始菜单，一键打开常用梗图
- 风景图 / 壁纸库：适合快速翻找大批量风景图、桌面壁纸、摄影收藏
- 截图收藏库：适合浏览零散保存的网页截图、聊天截图、灵感素材
- 多图库入口：每个图库放一份 `CppGallery.exe`，分别固定，形成多个独立入口

### 启动方式

#### 方式 1：直接双击运行

如果 `CppGallery.exe` 放在图片文件夹里，直接双击即可。  
程序会优先尝试把当前目录或可执行文件所在目录当作图片根目录。

#### 方式 2：命令行指定文件夹

```powershell
.\CppGallery.exe D:\Pictures\MemePack
```

适合你想让一个程序入口指向任意图库时使用。

### 支持的内容

静态图片：

- `png`
- `jpg`
- `jpeg`
- `bmp`
- `tga`
- `hdr`
- `pic`
- `ppm`
- `pgm`
- `webp`（支持静态与动画 WebP）

动画图片：

- `gif`

视频预览：

- `mp4`
- `mkv`
- `webm`
- `avi`
- `mov`
- `m4v`

### 基本操作

图库视图：

- 鼠标滚轮：上下滚动图库
- 键盘 `Up` / `Down`：上下滚动
- 键盘 `Left` / `Right`：减少或增加列数
- 点击左上角 `Open Folder`：打开当前图库文件夹
- 点击 `Show Stats` / `Hide Stats`：显示或隐藏性能信息
- 键盘 `V`：跳到第一个视频
- 拖动右侧滚动条：快速定位

单图放大视图：

- 左键点击图片：进入放大查看
- 鼠标滚轮：线性缩放图片
- 拖动画面：平移查看
- 右键：复制当前图片路径
- `Delete`：删除当前正在查看的图片
- `Esc`：退出放大视图；如果当前不在放大视图，则关闭程序

### 适合怎样组织你的图片库

推荐按用途分目录，例如：

- `表情包`
- `风景`
- `壁纸`
- `截图收藏`

然后每个目录里都放一份 `CppGallery.exe`。  
这样你可以把这些入口分别固定到开始屏幕，形成多个一键入口。

例如：

```text
D:\Media\表情包\CppGallery.exe
D:\Media\风景\CppGallery.exe
D:\Media\壁纸\CppGallery.exe
```

### 可选的 FFmpeg 外置支持

这个仓库默认**不内置** `ffmpeg.exe` 和 `ffprobe.exe`。  
如果你需要视频探测或视频预览，请把外部 FFmpeg 工具放到下面任意一个位置：

- 和 `CppGallery.exe` 放在一起
- `third_party/ffmpeg/`
- `third_party/ffmpeg/bin/`
- `FFMPEG_ROOT` 环境变量指向的目录或其 `bin` 子目录

如果没有 FFmpeg：

- 静态图片仍然可以正常浏览
- GIF 仍然可以正常浏览
- 只有视频相关能力不可用

### 常见问题 FAQ

**Q: 为什么把 `CppGallery.exe` 放进图片文件夹后，双击就直接打开这个文件夹？**  
A: 这是刻意设计的。这个软件支持“程序跟着图库走”的用法，适合把一个 exe 当成一个图库入口。

**Q: 可以给不同图库各放一份 `CppGallery.exe` 吗？**  
A: 可以。这正是推荐用法之一。比如表情包、风景、壁纸各放一份，再分别固定到开始菜单。

**Q: 不放 FFmpeg 还能用吗？**  
A: 能。静态图片和 GIF 不受影响；只有视频探测和视频预览不可用。

**Q: `Delete` 删除图片时会进回收站吗？**  
A: 当前实现是直接删除文件，不走回收站。使用前要清楚这一点。

**Q: 最适合把这个软件放在哪里？**  
A: 最适合直接放在图片文件夹内部，而不是单独放在一个统一的软件目录里。这样双击和固定入口都更顺手。

### 从源码构建

要求：

- `CMake 3.20+`
- 支持 `C++17` 的编译器
- 支持 `OpenGL 3.3` 的显卡驱动

构建命令：

```powershell
cmake -S . -B build -DCPPGALLERY_FETCH_DEPS=ON
cmake --build build --config Release
```

运行：

```powershell
.\build\Release\CppGallery.exe <图片文件夹>
```

### 仓库内容说明

- `main.cpp`：主程序源码
- `CMakeLists.txt`：构建配置
- `stb_image.h`：本地图像解码依赖
- `publish/open-source/`：对外发布用的干净暂存区

这个仓库默认不包含：

- 构建产物
- 测试图片集
- 本地性能 trace
- FFmpeg 二进制

### 许可证

本项目使用 `MIT License`。  
详见 [LICENSE](LICENSE) 和 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

---

## English

### What This Is

`CppGallery` is a Windows-first image viewer built with `GLFW`, `Dear ImGui`, and `OpenGL`.  
It is designed for large image folders, smooth scrolling, responsive zooming, and GPU-friendly texture handling.

### What The README Is For

Yes. A `README` is usually the main project page and the first piece of user documentation.  
For this project, it serves as both repository documentation and end-user operating instructions.

### Recommended Real-World Usage

One of the most practical ways to use `CppGallery` is this:

1. Put `CppGallery.exe` directly inside a picture folder.
2. That folder can be a:
   - meme folder
   - scenery folder
   - wallpaper folder
   - personal image collection
3. Double-click `CppGallery.exe`.
4. The app will prefer the executable directory as the gallery root when launched this way.
5. Right-click that executable and pin it to Start / the Start menu on Windows.
6. After that, you get a one-click launcher for that exact image library.

The key idea is simple:

- the executable lives inside the image folder
- launching that executable opens that folder as a gallery
- one folder can have one dedicated launcher

### Screenshot Placeholders

For a public release, it is worth adding these screenshots to the repository:

- `docs/screenshots/gallery-overview.png`: main gallery overview
- `docs/screenshots/zoom-view.png`: zoomed single-image view
- `docs/screenshots/start-pinned-launcher.png`: example of the app pinned to Windows Start

See `docs/screenshots/README.md` for the placeholder notes.

### Common Use Cases

- Meme library: place `CppGallery.exe` inside a meme folder and pin it for one-click access
- Scenery / wallpaper library: useful for browsing large scenic or wallpaper collections quickly
- Screenshot archive: convenient for browsing saved screenshots, references, or inspiration images
- Multiple pinned libraries: put one `CppGallery.exe` in each folder and pin each launcher separately

### How To Launch

#### Option 1: Double-click the executable

If `CppGallery.exe` is placed inside a media folder, double-clicking it is usually enough.  
The app tries sensible defaults around the current working directory and the executable directory.

#### Option 2: Pass a folder on the command line

```powershell
.\CppGallery.exe D:\Pictures\MemePack
```

Use this when you want one executable entry point to open any folder you choose.

### Supported Media

Static images:

- `png`
- `jpg`
- `jpeg`
- `bmp`
- `tga`
- `hdr`
- `pic`
- `ppm`
- `pgm`
- `webp` (static and animated WebP)

Animated images:

- `gif`

Video preview:

- `mp4`
- `mkv`
- `webm`
- `avi`
- `mov`
- `m4v`

### Controls

Gallery view:

- Mouse wheel: scroll the gallery
- `Up` / `Down`: scroll
- `Left` / `Right`: decrease or increase the column count
- `Open Folder`: open the current gallery folder in Explorer
- `Show Stats` / `Hide Stats`: toggle performance information
- `V`: jump to the first video
- Drag the right-side scrollbar: fast navigation

Zoomed image view:

- Left click an image: open zoom view
- Mouse wheel: zoom the image linearly
- Drag: pan the zoomed image
- Right click: copy the current image path
- `Delete`: delete the currently zoomed file
- `Esc`: leave zoom view, or close the app if no zoom view is open

### Suggested Folder Organization

Recommended folder categories:

- memes
- scenery
- wallpapers
- screenshots

You can place one `CppGallery.exe` inside each folder and pin each one separately in Windows for fast access.

Example:

```text
D:\Media\Memes\CppGallery.exe
D:\Media\Scenery\CppGallery.exe
D:\Media\Wallpapers\CppGallery.exe
```

### Optional External FFmpeg Runtime

This repository does **not** bundle `ffmpeg.exe` or `ffprobe.exe` by default.  
If you want video probing or preview, place external FFmpeg tools in any of these locations:

- beside `CppGallery.exe`
- `third_party/ffmpeg/`
- `third_party/ffmpeg/bin/`
- the directory referenced by the `FFMPEG_ROOT` environment variable, or its `bin` subdirectory

Without FFmpeg:

- static images still work
- GIFs still work
- only video-related features are unavailable

### FAQ

**Q: Why does double-clicking `CppGallery.exe` inside a picture folder open that folder directly?**  
A: That is intentional. The app is designed to support a "portable launcher per gallery folder" workflow.

**Q: Can I keep separate copies of `CppGallery.exe` in different folders?**  
A: Yes. That is one of the recommended usage patterns. You can keep separate launchers for memes, scenery, wallpapers, and so on.

**Q: Does the app still work without FFmpeg?**  
A: Yes. Static images and GIFs still work normally. Only video probing and preview are unavailable.

**Q: Does `Delete` send files to the Recycle Bin?**  
A: No. The current implementation removes the file directly, so use it with care.

**Q: Where should I place the app for the best experience?**  
A: Usually inside the image folder itself, not in a separate central software folder. That makes double-click launch and Start pinning much more convenient.

### Building From Source

Requirements:

- `CMake 3.20+`
- a `C++17` compiler
- an `OpenGL 3.3` capable graphics driver

Build:

```powershell
cmake -S . -B build -DCPPGALLERY_FETCH_DEPS=ON
cmake --build build --config Release
```

Run:

```powershell
.\build\Release\CppGallery.exe <media-folder>
```

### Repository Layout

- `main.cpp`: application source
- `CMakeLists.txt`: build configuration
- `stb_image.h`: local image decoding dependency
- `publish/open-source/`: curated staging area for public release/export

This repository intentionally excludes:

- build output
- sample media sets
- local performance traces
- bundled FFmpeg binaries

### License

This project is released under the `MIT License`.  
See [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
