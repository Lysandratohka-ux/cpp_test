# 华东师大课表 → Apple 日历

独立 Python 工具，适用于 macOS 和 Microsoft Edge。根据 2026-11-01 实际查看的 `byyt.ecnu.edu.cn/student/for-std/course-table` 页面开发。已安装依赖，通过真实网站第 1–18 周读取和 Apple 日历原生 ICS 导入验证；EventKit 授权在当前运行环境未能完成，提供本地快照及 ICS 导出作为替代路径。

## 使用

双击 `启动课表同步.command`。首次启动会建立 Python 虚拟环境并从 PyPI 安装 Playwright 和 PyObjC，需要网络及 Python 3.10 或更新版本。使用本机 Edge，无需下载 Chromium。

1. 在工具打开的专用 Edge 窗口中登录教务系统。
2. 进入“我的课表”，选择需要的学期。无需选择“全部周次”。
3. 回到终端按回车。选择需要导入的周次（如 `7`、`7-10`、`1,3,5`，回车表示逐周读取所有周）。工具自动切换周次并读取各周表格，保存 `preview.json`，随后请求日历访问权限并同步。
4. 打开 Apple 日历查看 `华东师大课表 · 学期名称`。

已有的普通 Edge 登录窗口无法直接复用；专用窗口的登录状态保存在本目录 `.browser-profile` 中，包含敏感会话信息，请勿分享或提交该目录。工具不读取或记录密码。退出同步后自动关闭专用窗口。

只检查课表、不写入日历：

```sh
.venv/bin/python calendar_sync.py --preview
```

指定第 9 周预览（登录进入课表后只需按回车）：

```sh
./启动课表同步.command --preview --weeks 9
```

`--weeks 7-10` 可指定范围。去掉 `--preview` 后直接写入日历。未传 `--weeks` 时，也可在进入课表后的第一个提示直接输入周次，不会再忽略该输入。

周次切换使用 Selectize 控件的完整选项与 `setValue` 接口，触发网站原有的课程刷新；不再依赖下拉框展开状态或当前可见的选项文字。

直接同步通过 EventKit 完成，不需要手动打开 ICS。不修改其他日历。逐周读取网站“第 N 周”视图，按上海时区和页面上的时间逐次建立事件，支持单双周、不同周次的教室和教师；同一教学班同一时段的多位教师合并成一条事件。重复运行通过事件备注中的固定标记更新已有事件。

当前不删除已取消或时间已改变的旧事件；这类变化需在专用课表日历内手动清理。请保留事件备注中的 `ECNU-SYNC-ID` 以支持去重。没有上课时间的“课表外课程”跳过。日期完全采用网站展示的学期起始日期与周次，不自行猜测节假日调课。

未找到课表时保留窗口并提示重试；按页面内容识别，不依赖固定 iframe 地址。周次或时间解析失败时会停止。同步过程中如 EventKit 报错，终端会显示原因；预览文件可用于检查。首次日历权限需你在 macOS 提示框内授权。系统版本或账户策略可能禁止终端访问或创建日历。

验证解析器：`python3 -m unittest -v test_calendar_sync.py`。

## 无需重新登录的恢复方式

逐周读取结果保存在 `snapshot-latest.json`，不含密码；该文件及课表导出文件已加入忽略列表。权限失败后可从快照继续：

```sh
.venv/bin/python calendar_sync.py --snapshot snapshot-latest.json
```

若 EventKit 授权仍无响应，可生成文件后通过 Apple 日历“文件 → 导入”选择专用课表日历：

```sh
.venv/bin/python calendar_sync.py --snapshot snapshot-latest.json --export-ics timetable.ics
```

`--auto` 自动等待课表出现并逐周读取全部周次。Mac 上的导入与 iCloud 上传是两个步骤；如日历显示账户更新错误，需等账户同步恢复后才能确认其他设备收到课程。
