#!/bin/zsh
cd -- "${0:A:h}" || exit 1
if [[ ! -x .venv/bin/python ]]; then
  python3 -m venv .venv || exit 1
fi
if ! .venv/bin/python -c 'import playwright, EventKit' 2>/dev/null; then
  .venv/bin/python -m pip install -r requirements.txt || exit 1
fi
.venv/bin/python calendar_sync.py "$@"
echo
read '?按回车关闭窗口…'
