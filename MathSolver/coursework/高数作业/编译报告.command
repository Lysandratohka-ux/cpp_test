#!/bin/zsh
set -euo pipefail
cd -- "${0:A:h}"
if [[ -x /Library/TeX/texbin/xelatex ]]; then
  infinity_xelatex=/Library/TeX/texbin/xelatex
else
  infinity_xelatex=$(command -v xelatex)
fi
mkdir -p .build
"$infinity_xelatex" -interaction=nonstopmode -halt-on-error -output-directory=.build Infinity高数作业报告.tex
"$infinity_xelatex" -interaction=nonstopmode -halt-on-error -output-directory=.build Infinity高数作业报告.tex
cp .build/Infinity高数作业报告.pdf Infinity高数作业报告.pdf
open Infinity高数作业报告.pdf
