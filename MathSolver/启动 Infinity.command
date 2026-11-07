#!/bin/zsh
cd -- "${0:A:h}"
if [[ -d releases/macos/Infinity.app ]]; then
  open releases/macos/Infinity.app
else
  open build/Infinity.app
fi
