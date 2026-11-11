import unittest
import tempfile
from pathlib import Path
from calendar_sync import parse, weeks


class ParserTests(unittest.TestCase):
    def test_ics_shanghai_time_and_utf8_folding(self):
        from calendar_sync import write_ics
        event = dict(uid='test', title='高等数学' * 30, start='2026-11-02T08:00:00',
                     end='2026-11-02T09:35:00', location='教室,一;二', notes='第一行\n第二行')
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / 'test.ics'
            write_ics([event], '2026秋', target)
            data = target.read_bytes()
            lines = data.split(b'\r\n')
            self.assertTrue(all(len(line) <= 75 for line in lines))
            unfolded = data.replace(b'\r\n ', b'').decode()
            self.assertIn('DTSTART:20261102T000000Z', unfolded)
            self.assertIn('DTEND:20261102T013500Z', unfolded)
            self.assertIn('SUMMARY:' + event['title'], unfolded)
            self.assertIn(r'LOCATION:教室\,一\;二', unfolded)
            self.assertEqual(unfolded.count('BEGIN:VEVENT'), 1)

    def test_mixed_parity(self):
        self.assertEqual(weeks('3~5(单),9~10,13'), [3, 5, 9, 10, 13])
        self.assertEqual(weeks('2~8(双)'), [2, 4, 6, 8])

    def test_merge_teachers_and_dates(self):
        data = {'start': '2026-09-14', 'term': '2026秋', 'cells': [
            {'day': 1, 'text': '微电子学导论\n教学班代码：COME1108.01\n'
             '(7~8周) (8-10节 14:50-17:15) 闵一教335 李晓梅\n'
             '(7周) (8-10节 14:50-17:15) 闵一教335 吴幸'}]}
        result = parse(data)
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]['start'], '2026-10-27T14:50:00')
        self.assertIn('吴幸', result[0]['location'])
        self.assertNotIn('吴幸', result[1]['location'])
        self.assertEqual(result, parse(data))

    def test_fail_closed(self):
        with self.assertRaises(ValueError):
            weeks('待定')
        with self.assertRaises(ValueError):
            parse({'start': '2026-09-14', 'term': '2026秋', 'cells': [
                {'day': 0, 'text': '未知课程\n教学班代码：TEST.01\n待定'}]})

    def test_weekly_only_imports_selected_week(self):
        cell = {'day': 0, 'text': '示例课程\n教学班代码：TEST.01\n'
                '(2~16周) (1-2节 8:00-9:35) 闵二教210 教师甲\n'
                '(4周) (1-2节 8:00-9:35) 其他教室 教师乙'}
        weekly = {'start': '2026-09-14', 'term': '2026秋', 'week': 7, 'cells': [cell]}
        events = parse({'weekly': [weekly], 'term': '2026秋'})
        self.assertEqual(len(events), 1)
        self.assertEqual(events[0]['start'], '2026-10-26T08:00:00')
        self.assertNotIn('教师乙', events[0]['location'])

    def test_empty_week_is_valid(self):
        self.assertEqual(parse({'start': '2026-09-14', 'term': '2026秋', 'week': 18, 'cells': []}), [])

    def test_weekly_different_rooms_and_inline_details(self):
        text = ('高等数学\n教学班代码：MATH.01\n'
                '(2~3,5~16周) (3-4节 9:50-11:25) 线上学习 教师甲 '
                '(4周) (3-4节 9:50-11:25) 闵四教431 教师甲')
        data = {'start': '2026-09-14', 'term': '2026秋', 'cells': [{'day': 2, 'text': text}]}
        fourth = dict(data, week=4)
        fifth = dict(data, week=5)
        events = parse({'term': '2026秋', 'weekly': [fourth, fifth]})
        self.assertEqual(len(events), 2)
        self.assertEqual(events[0]['location'], '闵四教431 教师甲')
        self.assertEqual(events[1]['location'], '线上学习 教师甲')

    def test_discovery_does_not_depend_on_url(self):
        from calendar_sync import find_timetable
        from unittest.mock import MagicMock
        frame = MagicMock()
        frame.url = 'https://example.invalid/changed-route'
        frame.locator.return_value.inner_text.return_value = '星期一 星期日'
        frame.locator.return_value.count.return_value = 1
        context = MagicMock()
        context.pages[0].frames = [frame]
        context.pages = [context.pages[0]]
        self.assertIs(find_timetable(context), frame)


if __name__ == '__main__':
    unittest.main()
