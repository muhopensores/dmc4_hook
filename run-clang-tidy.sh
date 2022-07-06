#!/bin/sh

clang_bin_dir='C:/Program Files/LLVM/bin'
clang_apply_replacements="$clang_bin_dir/clang-apply-replacements.exe";
clang_tidy_binary="$clang_bin_dir/clang-tidy.exe";
python "C:/Program Files/LLVM/share/clang/run-clang-tidy.py" -clang-apply-replacements "$clang_apply_replacements" -clang-tidy-binary "$clang_tidy_binary" "$@";