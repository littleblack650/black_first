#!/usr/bin/env python3
"""
Sort image blocks in a picture text file by the numeric value found in their comment.

Usage:
  python sort_picture_txt_by_comment.py [-i INPUT] [-o OUTPUT] [--dry-run]

Defaults:
  INPUT:  picture_tool/picture.txt
  OUTPUT: picture_tool/picture_sort.txt

The script preserves the original block formatting exactly, only reorders whole blocks.
"""
import re
import argparse
from pathlib import Path
import sys


def is_comment_line_with_number(line: str):
    # Return number if line contains a comment marker and a number inside the comment.
    # Supports C-style comments (/* ... */) and C++-style (// ...).
    if '//' in line:
        m = re.search(r'//.*?(\d+)', line)
        if m:
            return int(m.group(1))
    if '/*' in line:
        m = re.search(r'/\*.*?(\d+).*?\*/', line)
        if m:
            return int(m.group(1))
        # in case comment not closed on same line
        m2 = re.search(r'/\*.*?(\d+)', line)
        if m2:
            return int(m2.group(1))
    # lines that start with '*' often belong to block comments
    if line.lstrip().startswith('*'):
        m = re.search(r'(\d+)', line)
        if m:
            return int(m.group(1))
    return None


def collect_blocks(lines):
    headers = []  # list of (index, number)
    for i, ln in enumerate(lines):
        num = is_comment_line_with_number(ln)
        if num is not None:
            headers.append((i, num))

    if not headers:
        return None, None  # no detectable blocks

    # preamble: lines before the first header
    first_header_idx = headers[0][0]
    preamble = lines[:first_header_idx]

    # build blocks from header starts
    blocks = []
    for idx, (start_idx, num) in enumerate(headers):
        end_idx = headers[idx + 1][0] if idx + 1 < len(headers) else len(lines)
        block_lines = lines[start_idx:end_idx]
        blocks.append((num, block_lines))

    return preamble, blocks


def write_sorted(preamble, blocks, out_path, dry_run=False):
    sorted_blocks = sorted(blocks, key=lambda x: x[0])
    if dry_run:
        print('Dry run: sorted order of block numbers:')
        print(', '.join(str(b[0]) for b in sorted_blocks))
        return

    with open(out_path, 'w', encoding='utf-8') as f:
        for ln in preamble:
            f.write(ln)
        for num, block in sorted_blocks:
            for ln in block:
                f.write(ln)


def main():
    p = argparse.ArgumentParser(description='Sort picture.txt blocks by number in comment')
    p.add_argument('-i', '--input', default='picture_tool/picture.txt')
    p.add_argument('-o', '--output', default='picture_tool/picture_sort.txt')
    p.add_argument('--dry-run', action='store_true')
    args = p.parse_args()

    inp = Path(args.input)
    out = Path(args.output)

    if not inp.exists():
        print(f'Input file not found: {inp}', file=sys.stderr)
        sys.exit(2)

    with open(inp, 'r', encoding='utf-8', errors='ignore') as fh:
        lines = fh.readlines()

    preamble, blocks = collect_blocks(lines)
    if blocks is None:
        print('No comment-number headers detected; copying file unchanged.')
        if args.dry_run:
            return
        out.write_text(''.join(lines), encoding='utf-8')
        return

    write_sorted(preamble, blocks, out, dry_run=args.dry_run)


if __name__ == '__main__':
    main()
