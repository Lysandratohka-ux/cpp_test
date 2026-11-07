# Infinity 2.4.1

**在Windows继续开发安装包：先读 [Windows Codex交接说明](docs/WINDOWS_HANDOFF.md) 和 [项目开发约定](AGENTS.md)。课程报告源码与PDF见 `coursework/高数作业/`。当前Windows脚本尚未实机验收。**

2.4.1：开发者名称更新为 Lysandra；修复夹逼极限与数值求值的定义域问题，并区分仅经数值抽查的符号原函数。当前限制及复现步骤见 [本轮检查记录](docs/REVIEW_2026_11_07.md)。

2.4 新增实数参数 `a、b、c、n` 的积分与极限：直接输出含字母公式，显示非零/定义域条件和部分退化分支。输入 `\sin(ax)/x` 并设置x→0得到a；积分 `x^a` 保留a=−1的对数分支。详见 [带参数计算](docs/PARAMETERS_2_4.md)。

2.2.1 将顶部导航改为明确的文字按钮：“返回首页”“LaTeX 教学”“软件说明”，保留悬停提示与键盘操作。

新增原生启动动画（Infinity / Lysandra开发）、空白启动首页、计算器与函数绘图双入口。独立绘图页支持最多8条LaTeX曲线、鼠标选点/指定横坐标求切线及方程、PNG导出；顶部“历史记录”打开独立窗口，首页只保留两个主要入口。右上角提供LaTeX教学、学习书目、Overleaf入口和能力说明。当前操作与范围见 [Infinity 2.3使用说明](docs/INFINITY_2_3.md)。

2.1.1 修复倒数型三角函数相减的极限：先通分并验证原分母在去心邻域非零，再比较解析首项。例如 `(\csc x-\cot x)/x` 在0处的左右极限均为1/2。新增5项极限回归和1项GUI流程，记录在 `docs/csc-cot-tests.log`。

专注于一元函数求导、积分和极限的 C++20 / Qt 6 桌面应用。首页提供计算器与函数绘图两个入口，计算页保留三个核心模式及次级工具。数学计算完全离线，不依赖 Python 或云端 API；JavaScript 仅用于本地 KaTeX 排版。

2.1 开始以用户提供的《高等数学（上）（第二版）》建立教材回归测试。已补齐 `\sec`、`\csc`、`\cot`、`\arccot`、`\log_3 x`、`\lg` 和分式别名，扩展根式、三角有理式及指数换元积分。**目前没有达到整本教材所有题型都可求解**；逐章范围、实际题号与缺口见 [教材覆盖记录](docs/TEXTBOOK_COVERAGE.md)。

2.3：品牌更新为 Infinity，蓝色 ∞ 图标；首页隐藏返回按钮，页脚注明基于 LaTeX 语言输入。新增混合根式积分换元、复合幂函数极限规则与多项式化简，详见 [本轮更新与验证](docs/INFINITY_2_3.md)。内部工程目录及数据身份保留旧名，以兼容已有历史。

## 启动

macOS Apple Silicon：

```sh
open releases/macos/Infinity.app
```

也可双击 `启动 Infinity.command`。`releases/macos/Infinity.app` 包含 Qt、libqalculate、WebEngine 辅助进程、离线排版和符号定义资源，开发构建仍在 `build/Infinity.app`。发行 Bundle 使用本地临时签名，未做 Apple 开发者签名和公证。

Windows：已经提供同一 Qt 界面的 MSVC 构建、ICO 资源和原生 C++ 符号辅助进程桥接，见下面的构建步骤。**本轮在 macOS 上完成实际运行；未提供声称已测试的 Windows EXE。Windows 打包脚本仍需在 Windows 上执行并通过测试。**

## 使用

- 首页选择“计算器”，再在顶部切换“求导 / 积分 / 极限”。输入LaTeX，预览实时更新，点击“计算”更新结果。首次启动不填示例，也不自动计算。
- 首页选择“函数绘图”，每行一个LaTeX函数。点击曲线求切线；拖动平移，滚轮缩放。曲线列表与横坐标输入可以明确指定切点，方程在画布下方排版显示。
- macOS 使用 `⌘ Return` 计算、`⌘ L` 定位并全选输入；Windows 对应 `Ctrl+Enter`、`Ctrl+L`。支持多行排版输入。
- 求导支持 1～8 阶，可留空指定点，仅计算导函数。
- 积分可选择不定积分或定积分。优先精确计算；符号结果不可得时，正常定积分可返回明确标注的数值近似。
- 极限设置有限数字、`inf` 或 `-inf`，选择双侧或单侧。
- “复制 LaTeX”只复制数学结果。方法、定义域条件、曲线、高级参数和历史均可展开。
- “更多工具”保留 Taylor、二分/Newton 求根、多函数绘图、数值算法实验、PNG 与 CSV 导出。
- 历史保留最近 50 条，沿用旧版应用数据路径。计算中修改输入不会把新参数错误记为旧结果的历史。

## 数学引擎

原有白名单 Tokenizer、递归下降解析器、不可变 AST 和纯 C++ 数值模块继续保留。新增 `calculus.h` 接口接入实际编译的 **libqalculate 5.13.1**，使用精确整数/有理数符号化简及积分。输入中的 `e`、`\pi`、分数和十进制数保留符号或原始数值词元，避免先转 double 再冒充精确系数。

支持四则运算、幂、根式、三角/反三角、指数/对数、绝对值、隐式乘法、单变量 x。角度为弧度；`\log` 为常用对数、`\ln` 为自然对数。可输入 `\int x e^{x^2}\,dx`，也可只输入被积函数。定积分上下限由界面设置，当前是数值输入框。

求导包含链式法则、高阶求导、化简与指定点计算，并在可计算点用原有 AST 求导进行独立交叉检查。输出 LaTeX 本身也有解析回读验证。

积分扩展覆盖换元、分部、三角、指数/对数和部分有理函数。成功算例包括 `x e^{x^2}`、`x sin x`、`ln x`、`(x²+1)/(x⁴+1)`。`e^{-x²}` 可返回 erf 特殊函数。返回原函数时优先作符号求导恒等验证；仅通过有限点抽查的结果会明确注明，抽查不等于证明。

极限优先使用带实解析邻域检查的首项展开（最高 12 阶）、连续性、零点阶数、倒数相位子序列及夹逼规则。其余使用有限的符号规则。结果分为：精确有限值、正/负无穷、不存在、当前算法无法判定。**不以数值采样宣布极限存在。**

四个重点例子的实际结果：

| 输入，x→0 | 结果 |
|---|---|
| `(e^x−1−x)/x²` | 1/2 |
| `(sin x−x)/x³` | −1/6 |
| `sin(1/x)` | 不存在 |
| `x sin(1/x)` | 0，夹逼定理 |

## 已知限制

- 不是完整 LaTeX 或通用计算机代数系统。只接收白名单单变量实表达式，不执行用户代码。
- 高阶求导 1～8 阶；局部展开最多 12 阶；复杂运算有工作量和时间预算。
- 通用符号极限仍不完整，例如 `sin(sin(1/x))` 返回“当前算法无法判定”。这个状态不等于极限存在或不存在。
- 部分原函数目前只完成求导抽查；结果按照原函数的实数定义域、各连续区间和分支条件解释。
- 不支持反常积分或 Cauchy 主值；区间定义域检查保守，可能拒绝实际合法但无法确认的区间。
- 数值定积分使用复合 Simpson 等算法，不自动给出严格误差上界。
- 特殊函数目前主要用于结果表示，不保证可以将所有特殊函数结果重新作为输入。
- 绘图为有限采样，不能保证发现所有极窄间断；没有交互式任务取消按钮。

## macOS 构建

本机依赖在 `.deps/qt`，包括原生 Qt 6.11.2、GMP、MPFR 和从随附官方源码编译的 libqalculate：

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="$PWD/.deps/qt" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/releases/macos"
```

最后一步部署共享库、WebEngine 资源与进程，并重新临时签名和验证。`scripts/build-macos.sh` 可执行完整流程。部署后的应用仍应在正常登录的桌面上运行，GUI 测试不可限制 WindowServer。

仅数学库/测试：

```sh
cmake -S . -B build-core -DMATHSOLVER_GUI=OFF -DCMAKE_PREFIX_PATH="$PWD/.deps/qt"
cmake --build build-core -j 4
ctest --test-dir build-core --output-on-failure
```

`third_party/libqalculate-5.13.1.tar.gz` 是实际使用的原始官方源码，SHA256 为 `cf8d3eaf3d85030115701e997b6585e69573e71add88e7758486c5d1b01d3cc3`。从源码构建时需要 GMP、MPFR 和 libxml2；本机配置关闭 NLS、curl、icu、readline、gnuplot-call 和 insecure 功能。macOS libxml2 头文件来自 Xcode SDK。

## Windows 构建

Qt WebEngine 不支持 MinGW；因此界面使用官方 **Qt 6 MSVC 2022 x64**，符号引擎通过独立 **MinGW UCRT64 C++ 进程**连接 libqalculate。两个进程交换受控结果，不加载对方的 C++ DLL ABI，不经过 shell 执行输入，不需要 Python。

1. 安装 Visual Studio 2022 C++ 工具和官方 Qt 6（MSVC x64、Widgets、Concurrent、WebEngine）。
2. 安装 MSYS2，在 UCRT64 终端进入项目，执行 `bash scripts/build-cas-windows.sh`。脚本安装原生编译依赖，构建并测试 C++ 辅助进程，收集所需 DLL。
3. 在 VS Developer PowerShell 中执行：

```powershell
./scripts/build-windows.ps1 -QtPrefix C:/Qt/6.11.2/msvc2022_64
```

脚本使用 PROCESS 后端、运行所有测试、通过 Qt 部署依赖并生成 `releases/Infinity-Windows-x64.zip`。必须保留 `cas/` 及全部 DLL 和资源。此流程已按官方平台要求配置，但本轮没有 Windows 主机，不能视为已验证的 Windows 安装包。

## 验证与图标

2.2 本机完整CTest为10/10通过，含原有26项数学GUI流程，以及新增动画可见性、空白启动、页面导航、Overleaf链接分发、8条曲线、数量限制、原生鼠标选点、拖动平移、两条曲线的切线、不可导/无定义点拒绝、历史恢复和紧凑窗口检查。日志见 `docs/home-ui-tests.log`，截图见 `docs/verification/home/`。

2.1 新增教材验收：求导24题、积分71题、极限6题；另有3项积分算法族回归和21项输入/定义域测试。GUI流程扩充至25项。题号、验证方式及未覆盖章节见 [教材覆盖记录](docs/TEXTBOOK_COVERAGE.md)。

2.0 保留的基线测试（本轮继续执行）：

| 类别 | 测试组 | 通过检查项 |
|---|---:|---:|
| 求导 | 18 | 46 |
| 积分 | 20 | 53 |
| 极限 | 22 | 33 |
| 新增解析/错误输入 | 9 | 9 |
| 原有核心回归 | 原测试集完整保留 | 64 |
| GUI 完整流程 | 20 | 20 |

基线积分组包括16个不定积分，均检查求导验证状态；15个可被本项目语法读回的原函数，还独立回读LaTeX并求导核对。验证强度按实际结果注明。历史日志在 `docs/v2-tests.log`，本轮日志在 `docs/textbook-tests.log`。

图标有三套原创可编辑 SVG：`integral.svg`、`curve.svg`、`orbit.svg`。选择积分方案，保留 16～1024 像素 Retina 图标集、ICNS 和多尺寸 Windows ICO，均在 `resources/icons/`。`tools/icons.cpp` 是原生 C++/Qt SVG 栅格化和 ICO 生成工具；不需要 Python。

## 目录与基线

- `src/engine.cpp`、`src/domain.cpp`：原有解析、求导、数值计算与区间检查。
- `src/calculus.cpp`：增强的 C++ 符号桥接、求导验证和极限规则。
- `src/calculus_process.cpp`、`tools/cas_worker.cpp`：Windows 本地进程桥接。
- `src/window.cpp`、`src/plot.cpp`：简化主界面及保留的可视化。
- `releases/baseline-v1/`：修改前源码归档与原应用；该开发应用仍依赖原 `.deps`。
- `docs/` 旧版课堂文档仅作历史保留，本轮未新增教师报告。

自编源码保留 MIT；包含 libqalculate 的组合发行构建按 GPL3 提供，见 [第三方与许可说明](docs/THIRD_PARTY.md)。
