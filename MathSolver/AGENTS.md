# Infinity 开发交接

本目录是已有的 C++20 / Qt 6 桌面应用。应用名称 Infinity，开发者 Lysandra，当前版本2.4.1；工程目录和内部数据身份沿用MathSolver。请在现有项目上迭代，不要重写成Python或删除已有功能。

## 开始前

先阅读 README.md、docs/WINDOWS_HANDOFF.md、CMakeLists.txt 和 docs/THIRD_PARTY.md。Windows任务是生成并实际验证原生安装包；目前只有macOS完成验收，不能把源码ZIP或未经测试的EXE称为已交付Windows安装包。

## 架构和约束

- 全部数学计算离线、C++实现；JavaScript仅供本地KaTeX排版。
- Windows GUI使用官方Qt的MSVC x64构建；libqalculate使用MSYS2 UCRT64构建的独立C++进程，通过管道通信。不要直接混合MSVC和MinGW的C++ ABI。
- LINKED后端位于src/calculus.cpp；PROCESS后端位于src/calculus_process.cpp，辅助程序为tools/cas_worker.cpp。
- 用户输入必须先经过安全AST解析，不能传入任意CAS命令或执行代码。
- 默认只使用变量x及实数参数a/b/c/n；n不自动假定为整数。保留参数条件、定义域和结果验证程度的区分。
- 极限不能以数值采样认定存在；新增audit_tests中的定义域保护不能移除。
- 保留启动动画、空白首页、历史记录、LaTeX教学、计算/绘图入口、选点切线、PNG/CSV及已有次级工具。

## Windows构建与验证

详细步骤见docs/WINDOWS_HANDOFF.md。先运行scripts/build-cas-windows.sh，再在VS Developer PowerShell运行scripts/package-windows.ps1。

保留完整自动化测试，按数学恒等关系验证导数/原函数，不以固定显示字符串掩盖数学失败。最近的macOS基线为18/18个CTest目标通过（含34条GUI流程和15项新增定义域断言）；Windows的首次构建仍需修复实际发现的问题。

Windows交付应包括Qt插件、QtWebEngineProcess、CAS辅助程序及DLL、数学定义、本地KaTeX、图标和许可证。必须在干净Windows环境验证安装、启动、核心计算、历史、绘图和卸载，并报告未解决事项。不要为通过测试而悄悄取消定义域检查或替换成硬编码答案。

本地依赖、构建目录、安装包、凭据与私有证书不提交Git。大型安装包用于GitHub Releases或其他明确授权的分发方式。不要将其他课程练习加入本项目的提交。
