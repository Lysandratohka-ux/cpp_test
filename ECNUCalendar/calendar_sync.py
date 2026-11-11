#!/usr/bin/env python3
"""Read ECNU's visible semester timetable and upsert events using EventKit."""
import argparse
import datetime as dt
import hashlib
import json
import re
import sys
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
DETAIL = re.compile(r'\(([^周\n]+)周\)\s*\(([^节\n]+)节\s*(\d{1,2}:\d{2})\s*[-~]\s*(\d{1,2}:\d{2})\)\s*([^\n]+?)(?=\s*\([\d,~～单双() -]+周\)|\n|$)')


def weeks(expression):
    result = set()
    for part in expression.replace('，', ',').split(','):
        match = re.fullmatch(r'\s*(\d+)(?:[~～-](\d+))?(?:\(([单双])\))?\s*', part)
        if not match:
            raise ValueError(f'无法识别周次：{part}')
        a, b, parity = match.groups()
        a, b = int(a), int(b or a)
        if not 1 <= a <= b <= 60:
            raise ValueError(f'周次超出范围：{part}')
        result.update(n for n in range(a, b + 1) if not parity or n % 2 == (parity == '单'))
    return sorted(result)


def parse(snapshot):
    if 'weekly' in snapshot:
        events = {}
        for weekly in snapshot['weekly']:
            for event in parse(weekly):
                events[event['uid']] = event
        if not events:
            raise ValueError('所选周次没有课程，未写入日历。')
        return sorted(events.values(), key=lambda x: (x['start'], x['title']))
    monday = dt.date.fromisoformat(snapshot['start'])
    if monday.weekday() != 0:
        raise ValueError('学期起始日期不是星期一，请检查页面。')
    events = {}
    for cell in snapshot['cells']:
        text = cell['text'].replace('\xa0', ' ')
        code = re.search(r'教学班代码[：:]\s*([^\s]+)', text)
        if not code:
            raise ValueError(f'课程单元格缺少教学班代码：{text}')
        name = text[:code.start()].strip().splitlines()[0]
        details = list(DETAIL.finditer(text))
        if not details:
            raise ValueError(f'无法识别课程时间：{name}')
        for detail in details:
            expression, periods, begin, end, place = detail.groups()
            scheduled = weeks(expression)
            if 'week' in snapshot:
                scheduled = [snapshot['week']] if snapshot['week'] in scheduled else []
            for week in scheduled:
                date = monday + dt.timedelta(weeks=week - 1, days=cell['day'])
                start = dt.datetime.combine(date, dt.time.fromisoformat(begin.zfill(5)))
                finish = dt.datetime.combine(date, dt.time.fromisoformat(end.zfill(5)))
                if finish <= start:
                    raise ValueError(f'结束时间无效：{name}')
                identity = f'{snapshot["term"]}|{code[1]}|{start.isoformat()}|{finish.isoformat()}'
                uid = hashlib.sha256(identity.encode()).hexdigest()[:32]
                entry = events.setdefault(uid, dict(uid=uid, title=name, start=start.isoformat(),
                    end=finish.isoformat(), location=[], notes=f'{code[1]}；第{week}周；第{periods}节'))
                if place.strip() not in entry['location']:
                    entry['location'].append(place.strip())
    if not events and 'week' not in snapshot:
        raise ValueError('没有找到带时间的课程，停止同步。')
    for entry in events.values():
        entry['location'] = ' / '.join(entry['location'])
    return sorted(events.values(), key=lambda x: (x['start'], x['title']))


def find_timetable(context):
    # SSO may open another tab or change the iframe URL. Match rendered content.
    for tab in context.pages:
        for frame in tab.frames:
            try:
                body = frame.locator('body').inner_text(timeout=3000)
                if all(word in body for word in ('星期一', '星期日')) and frame.locator('#weeks-selectized').count():
                    return frame
            except Exception:
                continue
    return None


WEEK_CONTROL = r"""arg => {
    // Selectize keeps the full option list in the widget, while the original
    // select may contain only the selected item. Do not toggle its dropdown.
    const controls = [...document.querySelectorAll('select,input')];
    const candidates = [];
    for (const element of controls) {
        const widget = element.selectize;
        const options = widget
            ? Object.entries(widget.options).map(([key, option]) => ({
                value: String(option[widget.settings.valueField] ?? key),
                label: String(option[widget.settings.labelField] ?? '').trim()
            }))
            : element.tagName === 'SELECT'
                ? [...element.options].map(o => ({value: o.value, label: o.textContent.trim()})) : [];
        const weekly = options.filter(o => /^第\s*\d+\s*周$/.test(o.label));
        if (weekly.length) candidates.push({element, widget, options: weekly});
    }
    const target = candidates.find(x => /week/i.test(x.element.id)) || candidates[0];
    if (!target) throw Error('未找到周次控件的完整选项，请进入课表页');
    const {element, widget, options} = target;
    if (arg && arg.value !== undefined) {
        if (!options.some(o => o.value === arg.value)) throw Error('目标周次不存在');
        if (widget) widget.setValue(arg.value);
        else {
            element.value = arg.value;
            element.dispatchEvent(new Event('change', {bubbles: true}));
        }
    }
    return {options, selected: String(widget ? widget.getValue() : element.value)};
}"""


def week_options(frame):
    state = frame.evaluate(WEEK_CONTROL, None)
    return {int(re.search(r'\d+', option['label'])[0]): option for option in state['options']}


def select_week(frame, option):
    state = frame.evaluate(WEEK_CONTROL, {'value': option['value']})
    if state['selected'] != option['value']:
        raise ValueError(f'未能选中{option["label"]}，未写入日历。')



READ_TABLE = r"""() => {
    const body = document.body.innerText;
    const controls = [...document.querySelectorAll('select,input')].map(e =>
        e.tagName === 'SELECT' ? e.selectedOptions[0]?.textContent : e.value).join(' ');
    const date = body.match(/起始日期\s*(\d{4}-\d{2}-\d{2})/);
    const term = (controls + ' ' + body).match(/20\d{2}\s*[春秋]/);
    const table = [...document.querySelectorAll('table')].find(t =>
        t.innerText.includes('星期一') && t.innerText.includes('星期日'));
    if (!date || !term || !table) throw Error('无法读取学期、起始日期或课表');
    const occupied = [], cells = [];
    [...table.rows].forEach((row, r) => {
        let col = 0;
        [...row.cells].forEach(cell => {
            while (occupied[r]?.[col]) col++;
            const span = cell.colSpan || 1, height = cell.rowSpan || 1;
            for (let y=r; y<r+height; y++) {
                occupied[y] ||= [];
                for (let x=col; x<col+span; x++) occupied[y][x] = true;
            }
            if (cell.innerText.includes('教学班代码')) {
                if (col < 1 || col > 7 || span !== 1) throw Error('无法确定课程星期');
                cells.push({day: col-1, text: cell.innerText});
            }
            col += span;
        });
    });
    return {start: date[1], term: term[0].replace(/\s/g,''), cells};
}"""


def collect(requested=None, auto=False):
    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        context = p.chromium.launch_persistent_context(str(ROOT / '.browser-profile'),
            channel='msedge', headless=False)
        try:
            page = context.pages[0] if context.pages else context.new_page()
            page.goto('https://byyt.ecnu.edu.cn/student/for-std/course-table', wait_until='domcontentloaded')
            page.bring_to_front()
            frame = None
            deadline = time.monotonic() + 900
            while frame is None:
                if auto:
                    ready = ''
                    if time.monotonic() > deadline:
                        raise ValueError('等待课表超时，请重新运行。')
                else:
                    ready = input('请在新开的 Edge 中进入“我的课表”；完成后按回车，也可直接输入要导入的周次（如 9 或 7-10）：').strip()
                frame = find_timetable(context)
                if frame is None:
                    status = []
                    for tab in context.pages:
                        for candidate in tab.frames:
                            try:
                                status.append(candidate.evaluate("""() => ({
                                    host: location.host, title: document.title,
                                    timetable: document.body.innerText.includes('星期一'),
                                    weekInput: !!document.querySelector('#weeks-selectized')
                                })"""))
                            except Exception:
                                pass
                    (ROOT / 'status.json').write_text(json.dumps(status, ensure_ascii=False, indent=2))
                    if auto:
                        page.wait_for_timeout(2000)
                    else:
                        print('尚未识别课表。请确认是在本工具新开的 Edge 窗口中操作；窗口保留，可继续操作后重试。')
            available = week_options(frame)
            if requested is None:
                requested = ready or input('要导入哪些周？回车逐周读取所有周，或输入 7、7-10、1,3,5：').strip()
            chosen = weeks(requested) if requested else sorted(available)
            if not chosen or any(w not in available for w in chosen):
                raise ValueError('所选周次不在网站周次列表中。')
            collected = []
            print('即将逐周读取：' + '、'.join(str(w) for w in chosen))
            for week in chosen:
                select_week(frame, available[week])
                # setValue triggers the widget's change event and timetable refresh.
                frame.page.wait_for_load_state('networkidle', timeout=20000)
                previous = None
                for attempt in range(20):
                    current = frame.evaluate(READ_TABLE)
                    if current == previous:
                        break
                    previous = current
                    frame.wait_for_timeout(400)
                else:
                    raise ValueError(f'第{week}周课表持续刷新，请重试。')
                current['week'] = week
                collected.append(current)
                (ROOT / 'snapshot-latest.json').write_text(json.dumps({'weekly': collected, 'term': collected[0]['term']}, ensure_ascii=False, indent=2))
                print(f'已读取第{week}周：{len(current["cells"])} 个课程单元格')
            return {'weekly': collected, 'term': collected[0]['term']}
        finally:
            context.close()


def sync(events, term):
    import EventKit as EK
    from Foundation import NSDate, NSTimeZone, NSRunLoop
    store = EK.EKEventStore.alloc().init()
    done, result = threading.Event(), []
    def completed(granted, error):
        result.extend([granted, error])
        done.set()
    if hasattr(store, 'requestFullAccessToEventsWithCompletion_'):
        store.requestFullAccessToEventsWithCompletion_(completed)
    else:
        store.requestAccessToEntityType_completion_(EK.EKEntityTypeEvent, completed)
    print('正在请求 Apple 日历完整访问权限；若出现系统提示，请允许。', flush=True)
    deadline = time.monotonic() + 120
    while not done.is_set() and time.monotonic() < deadline:
        NSRunLoop.currentRunLoop().runUntilDate_(NSDate.dateWithTimeIntervalSinceNow_(0.1))
    if not done.is_set() or not result or not result[0]:
        reason = str(result[1]) if len(result) > 1 and result[1] else '授权未完成或被拒绝'
        raise RuntimeError(f'未取得日历权限：{reason}。课表已保留，无需重新登录。')
    title = f'华东师大课表 · {term}'
    matches = [c for c in store.calendarsForEntityType_(EK.EKEntityTypeEvent) if c.title() == title]
    if len(matches) > 1:
        raise RuntimeError(f'存在多个同名日历：{title}，请先重命名。')
    calendar = matches[0] if matches else None
    if calendar is None:
        default = store.defaultCalendarForNewEvents()
        if default is None:
            raise RuntimeError('Apple 日历中没有可写的默认日历，请先配置日历账户。')
        calendar = EK.EKCalendar.calendarForEntityType_eventStore_(EK.EKEntityTypeEvent, store)
        calendar.setTitle_(title)
        calendar.setSource_(default.source())
        ok, error = store.saveCalendar_commit_error_(calendar, True, None)
        if not ok:
            raise RuntimeError(str(error))
    if not calendar.allowsContentModifications():
        raise RuntimeError('目标日历不可写。')
    def date(value):
        from zoneinfo import ZoneInfo
        return NSDate.dateWithTimeIntervalSince1970_(dt.datetime.fromisoformat(value).replace(tzinfo=ZoneInfo('Asia/Shanghai')).timestamp())
    first = min(e['start'] for e in events)
    last = max(e['end'] for e in events)
    predicate = store.predicateForEventsWithStartDate_endDate_calendars_(date(first), date(last), [calendar])
    existing = {}
    for event in store.eventsMatchingPredicate_(predicate):
        marker = re.search(r'ECNU-SYNC-ID:([a-f0-9]{32})', event.notes() or '')
        if marker:
            existing[marker[1]] = event
    for item in events:
        event = existing.get(item['uid']) or EK.EKEvent.eventWithEventStore_(store)
        event.setCalendar_(calendar)
        event.setTitle_(item['title'])
        event.setStartDate_(date(item['start']))
        event.setEndDate_(date(item['end']))
        event.setTimeZone_(NSTimeZone.timeZoneWithName_('Asia/Shanghai'))
        event.setLocation_(item['location'])
        event.setNotes_(item['notes'] + '\nECNU-SYNC-ID:' + item['uid'])
        ok, error = store.saveEvent_span_commit_error_(event, EK.EKSpanThisEvent, False, None)
        if not ok:
            store.reset()
            raise RuntimeError(str(error))
    ok, error = store.commit_(None)
    if not ok:
        raise RuntimeError(str(error))
    print(f'已向“{title}”同步 {len(events)} 次课程（重复运行会更新同一课程）。')


def write_ics(events, term, path):
    from zoneinfo import ZoneInfo
    def escape(value):
        return value.replace('\\', '\\\\').replace('\n', '\\n').replace(';', '\\;').replace(',', '\\,')
    def stamp(value):
        return dt.datetime.fromisoformat(value).replace(tzinfo=ZoneInfo('Asia/Shanghai')).astimezone(dt.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    lines = ['BEGIN:VCALENDAR', 'VERSION:2.0', 'PRODID:-//ECNUCalendar//ZH',
             'CALSCALE:GREGORIAN', 'X-WR-CALNAME:' + escape('华东师大课表 · ' + term)]
    now = dt.datetime.now(dt.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    for event in events:
        lines.extend(['BEGIN:VEVENT', 'UID:' + event['uid'] + '@ecnu-calendar.local',
                      'DTSTAMP:' + now, 'DTSTART:' + stamp(event['start']), 'DTEND:' + stamp(event['end']),
                      'SUMMARY:' + escape(event['title']), 'LOCATION:' + escape(event['location']),
                      'DESCRIPTION:' + escape(event['notes'] + '\nECNU-SYNC-ID:' + event['uid']), 'END:VEVENT'])
    lines.append('END:VCALENDAR')
    folded = []
    for line in lines:
        chunk = ''
        for char in line:
            if len((chunk + char).encode('utf-8')) > 75:
                folded.append(chunk)
                chunk = ' '
            chunk += char
        folded.append(chunk)
    path.write_bytes(('\r\n'.join(folded) + '\r\n').encode('utf-8'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, help='使用本地课表快照，跳过浏览器')
    parser.add_argument('--preview', action='store_true', help='仅生成预览，不写入日历')
    parser.add_argument('--weeks', help='只导入指定周次，例如 9、7-10 或 1,3,5')
    parser.add_argument('--auto', action='store_true', help='自动等待课表出现并逐周读取，无需终端输入')
    parser.add_argument('--export-ics', type=Path, help='从读取结果导出 Apple 日历兼容文件，跳过 EventKit')
    args = parser.parse_args()
    if args.weeks:
        weeks(args.weeks)  # Validate before launching the browser.
    requested = '' if args.auto and args.weeks is None else args.weeks
    snapshot = json.loads(args.snapshot.read_text()) if args.snapshot else collect(requested, args.auto)
    events = parse(snapshot)
    preview = ROOT / 'preview.json'
    preview.write_text(json.dumps(dict(term=snapshot['term'], events=events), ensure_ascii=False, indent=2))
    print(f'读取到 {len(events)} 次课程，预览已保存：{preview}')
    print('未安排时间的课表外课程不会导入。')
    if args.export_ics:
        write_ics(events, snapshot['term'], args.export_ics)
        print(f'已导出 {len(events)} 次课程：{args.export_ics}')
    elif not args.preview:
        sync(events, snapshot['term'])


if __name__ == '__main__':
    try:
        main()
    except (Exception, KeyboardInterrupt) as error:
        print(f'同步未完成：{error}', file=sys.stderr)
        sys.exit(1)
