# Windows Codex 开发交接：Infinity 2.4.1

## 目标与当前状态

在现有项目基础上生成**能安装、能离线运行、能卸载的Windows x64 Setup.exe**，保持当前数学功能和中文界面。应用/开发者：Infinity / Lysandra。

- macOS Apple Silicon已编译、运行、验证发行包；最近完整回归18/18通过。
- 有Windows CMake分支、ICO、原生CAS进程桥接、部署脚本及Inno Setup安装器配置。
- **尚无经过Windows实机验证的安装包。** 现有脚本是开发起点，不是已成功运行的证明。
- 不需要把Mac的.deps目录复制到Windows；应安装对应Windows工具链。
- 作业报告的LaTeX源码、PDF和图片位于coursework/高数作业，开发/测试记录在docs中。

## 今晚如何开始

在Windows安装Git与Codex后：

```powershell
git clone https://github.com/Lysandratohka-ux/cpp_test.git
cd cpp_test\MathSolver
```

用Codex打开MathSolver目录，并发送本页末尾的提示词。若仓库已存在，先保留本地修改，再获取远端最新提交，不要强制覆盖自己的工作。

## 需要的原生环境

1. Visual Studio 2022或Build Tools：Desktop development with C++、MSVC x64和Windows SDK。
2. CMake、官方Qt 6 MSVC 2022 x64，包含Widgets、Concurrent和WebEngine。当前Mac使用Qt6.11.2，Windows可先选同版；实际依赖版本必须记录。
3. MSYS2 UCRT64，用于构建/链接libqalculate和独立CAS辅助程序。
4. Inno Setup 6（支持x64compatible架构标识的版本）。

Python可以作为下载Qt等构建工具的依赖，但**不能成为应用的运行时或数学后端**。无需复制Mac的conda依赖。

## 构建顺序

### 1. MSYS2 UCRT64终端

进入本项目目录，执行：

```sh
bash scripts/build-cas-windows.sh
```

脚本安装/使用MSYS2的C++依赖，以LINKED后端运行数学测试，并将worker、递归DLL依赖、定义及许可暂存到build-cas-windows/stage。

### 2. Visual Studio Developer PowerShell

回到本项目目录，Qt路径按实际安装位置修改：

```powershell
.\scripts\build-windows.ps1 -QtPrefix "C:\Qt\6.11.2\msvc2022_64"
```

这里必须是MSVC工具环境，不能拿MinGW Qt直接替代官方MSVC WebEngine构建。正常流程将用PROCESS后端构建Qt GUI、运行CTest并部署到releases/windows。

### 3. 生成安装器

```powershell
.\scripts\package-windows.ps1 -QtPrefix "C:\Qt\6.11.2\msvc2022_64"
```

必要时提供-CasDirectory和-IsccPath。此脚本会调用完整构建/测试流程，再复制可再分发CRT依赖、检查部署目录并调用scripts/Infinity.iss。预期文件名：

```text
releases/Infinity-2.4.1-Windows-x64-Setup.exe
```

脚本不提供跳过测试的打包选项。生成文件后仍须实际安装、运行和卸载验收；如果工具不存在或依赖检查失败，应修复环境，而不是创建占位EXE。

## 首轮重点排查

- ABI：Qt GUI必须使用MSVC；MinGW的libqalculate仅供独立worker使用。
- worker路径：主程序同目录下cas/mathsolver_cas_worker.exe及其依赖DLL必须齐全。
- 符号定义：主程序设置QALCULATE_DEFINITIONS_DIR到resources/qalculate；另查MSVC测试进程直接调用worker时如何定位定义，不能依赖开发机路径。
- WebEngine：部署QtWebEngineProcess.exe、运行资源、本地KaTeX和plugins/platforms目录；验证关闭开发工具后仍可显示公式。
- CRT：除了msvcp140.dll和vcruntime140.dll，检查实际导入是否需要vcruntime140_1.dll等；不能靠开发机预装环境掩盖缺失文件。
- CMake/UTF-8：保留MSVC /utf-8、中文错误消息、路径及窗口缩放；检查不同DPI下的启动动画和公式裁切。
- Inno：当前为普通用户安装到LocalAppData，含开始菜单、可选桌面快捷方式及卸载；实际验证版本架构标识、图标和应用关闭行为。
- MSYS2滚动更新可能改变libqalculate版本或显示形式。数学检查应比较恒等式/数值与条件，不应为匹配字符串而改变数学含义。

## 必须保留的数学基线

| 输入/设置 | 应有行为 |
| --- | --- |
| sin(x)/x，x→0 | 1 |
| (exp(x)-1-x)/x²，x→0 | 1/2 |
| sin(ax)/x，x→0 | a，a为实数 |
| x^a的不定积分 | x>0上区分a=-1与a≠-1 |
| exp(ax)的积分 | 区分a=0与a≠0 |
| x*sin(1/x)，x→0 | 夹逼得0 |
| x*sin(ln(-x))，x→0⁺ | 不得判0；无该侧实数定义域时保守返回无法判定 |
| (ln(-x))^0在x=1数值求值 | 不得因外层零次幂返回假的1 |
| 多项式化简、sec/csc/cot、切线、8条曲线、历史 | 原有行为保留 |

某些原函数只能通过数值抽查，界面须保留“符号结果 · 数值抽查通过”的标签。不要把原先标为Unknown的复杂参数分支误报为已经求解。

## 验收与产物

- 编译应用与worker；CTest全部通过，并保留错误处理测试。
- 从releases/windows直接启动，不能借助开发目录DLL、MSYS2 PATH或本机Qt插件目录。
- 在未安装Qt/MSYS2的Windows环境验证公式输入与排版、三种计算模式、含参数结果、历史、绘图和选点切线。
- 生成安装器，检查安装/启动/快捷方式/卸载；记录Windows版本、架构、Qt/CAS/编译器版本和SHA256。
- 维护docs/THIRD_PARTY.md以及实际发行依赖的许可证。
- 安装器放在releases下，保持Git忽略；如需发布大文件，请使用明确授权的GitHub Releases，不要将DLL/EXE/DMG塞入源码提交。

## 可直接复制给Windows Codex的提示词

> 请继续开发当前Infinity C++20/Qt6项目的Windows x64安装包。先读取AGENTS.md、README.md、docs/WINDOWS_HANDOFF.md和构建脚本。不要重新创建项目，不使用Python数学后端，不删除已有功能。当前macOS基线18/18测试通过，但Windows尚未构建验证。请检查并安装/配置MSVC、官方Qt WebEngine、MSYS2 UCRT64 CAS组件及Inno Setup，实际编译、运行测试、修复Windows问题，生成并验收真正的Setup.exe。保留Lysandra署名、中文界面、安全解析、数学定义域/参数条件及原有GUI流程。报告实际测试数量、安装包路径和未解决限制，不能把源码包或未验证文件称为已完成安装包。
