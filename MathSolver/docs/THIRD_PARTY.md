# 第三方组件与发行许可

MathSolver 2.0 的自编源文件保留原有 MIT 许可。启用 libqalculate 的组合发行二进制按 **GPL-3.0** 条款提供；MIT 不替代链接库的许可。GPL 原文见 `third_party/licenses/MathSolver-binary/`。所有相关源码和重新构建方式随项目提供。

| 组件 | 实际用途 | 许可 |
|---|---|---|
| Qt 6 Widgets / Concurrent / WebEngine | C++ 桌面、后台任务、本地排版 | Qt 部分 LGPL-3.0；WebEngine/Chromium 有多种第三方许可 |
| libqalculate 5.13.1 | 原生 C++ 化简、积分、求导、有限符号极限 | GPL-2.0-or-later；与 Qt LGPL3 组合按 GPL3 发布 |
| GMP、MPFR | 精确数与高精度运算 | LGPL 等，详见逐包原文 |
| KaTeX 0.16.22 | 数学排版；不执行数学计算 | MIT |
| Qt 传递依赖与字体 | 图形、字体、压缩等 | 保留发行包原始许可证 |

libqalculate 对应官方源码完整保留在 `third_party/libqalculate-5.13.1.tar.gz`，下载地址：
https://github.com/Qalculate/libqalculate/releases/download/v5.13.1/libqalculate-5.13.1.tar.gz 。本机使用该源码原样编译，关闭网络获取汇率、外部命令、gnuplot 和不安全函数；没有修改库源码。Windows 使用 MSYS2 官方 libqalculate 5.13.1 包和独立 C++ 辅助进程，不将 MinGW C++ ABI 与 MSVC 混用。分发 Windows 包时，需保留 MSYS2 传递依赖的许可和对应源码获取途径。

程序只向库传递安全解析器生成的表达式树，不传递用户原始代码或任意 qalc 命令。不加载用户本地定义、不调用在线计算 API。库的通用极限能力有限，关键的有限点抵消和振荡情形优先使用本项目带前提检查的展开、子序列和夹逼规则。

官方参考：

- [libqalculate API](https://qalculate.github.io/reference/)
- [libqalculate COPYING](https://github.com/Qalculate/libqalculate/blob/master/COPYING)
- [Qt Licensing](https://doc.qt.io/qt-6/licensing.html)
- [Qt WebEngine Licensing](https://doc.qt.io/qt-6.8/qtwebengine-licensing.html)
- [Qt Windows WebEngine platform requirements](https://doc.qt.io/qt-6.8/qtwebengine-platform-notes.html)
- [MSYS2 Windows libqalculate package](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-libqalculate)
- [KaTeX MIT License](https://github.com/KaTeX/KaTeX/blob/main/LICENSE)

`third_party/licenses/` 是本项目本机依赖包的许可证快照，不能替代公开分发前对实际部署依赖清单的检查。macOS 当前使用本地临时签名，无 Apple 开发者签名/公证；Windows 构建尚需在 Windows 机器验证。
