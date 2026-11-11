# Windows 构建状态：尚未生成安装包

当前交付环境只有 macOS：没有 Windows 系统、MSVC、Windows Qt WebEngine 或已编译的 Infinity.exe。因此本文件夹是源码构建资料，不是可安装的 Windows 版本；未提供伪装为安装包的 ZIP/EXE。

应用源码位于上一级“应用源码/Infinity-2.4.1-source.zip”。解压后，以下脚本都在源码的 scripts 文件夹中。

## 有 Windows 环境后如何生成 EXE

1. 在 Windows x64 安装 Visual Studio 2022 C++ 开发工具、CMake、官方 Qt 6 MSVC 2022 x64（包含 WebEngine、Concurrent），以及 MSYS2 UCRT64 和 Inno Setup 6。
2. 在 MSYS2 UCRT64 终端进入源码目录，执行：

```sh
bash scripts/build-cas-windows.sh
```

3. 在 Visual Studio Developer PowerShell 中进入源码目录，执行（Qt路径按实际安装目录修改）：

```powershell
.\scripts\package-windows.ps1 -QtPrefix "C:\Qt\6.11.2\msvc2022_64"
```

4. 脚本先构建并运行测试、部署Qt及CAS依赖，再调用Inno Setup，预期输出：

```text
releases/Infinity-2.4.1-Windows-x64-Setup.exe
```

5. 必须在干净的Windows电脑验证安装、公式预览、求导/积分/极限与卸载后再分发。当前脚本未在Windows实际执行，不代表已经通过验收。安装器使用当前用户目录，包含开始菜单和可选桌面快捷方式及卸载功能。

第三方许可证随部署目录递归包含。MSVC CRT依赖检查不通过时脚本会停止，不会继续生成声称完整的安装包。

## 官方参考

- Qt Windows部署：https://doc.qt.io/qt-6/windows-deployment.html
- Inno Setup架构设置：https://jrsoftware.org/ishelp/topic_setup_architecturesallowed.htm
- Inno Setup普通用户安装：https://jrsoftware.org/ishelp/topic_setup_privilegesrequired.htm

本次交付的可用安装包只有“安装包/macOS”里的DMG。Windows EXE仍待Windows构建环境完成。
