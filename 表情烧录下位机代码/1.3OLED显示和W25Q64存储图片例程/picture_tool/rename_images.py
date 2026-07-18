#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
图片文件批量重命名工具
将文件夹中的图片按当前排序重命名为数字序号（1, 2, 3, 4...）
"""

import os
import sys
from pathlib import Path

# 支持的图片格式
IMAGE_EXTENSIONS = {'.jpg', '.jpeg', '.png', '.bmp', '.gif', '.tiff', '.tif', '.webp'}


def get_images_sorted(folder_path):
    """获取文件夹中按文件名排序的图片列表"""
    folder = Path(folder_path)
    if not folder.exists() or not folder.is_dir():
        return []
    
    images = []
    for file_path in sorted(folder.iterdir()):
        if file_path.is_file() and file_path.suffix.lower() in IMAGE_EXTENSIONS:
            images.append(file_path)
    
    return images


def rename_images(folder_path, start_number=1, prefix='', suffix='', dry_run=False):
    """
    重命名图片文件
    
    Args:
        folder_path: 图片文件夹路径
        start_number: 起始编号（默认从1开始）
        prefix: 文件名前缀（可选，如'img_'）
        suffix: 文件名后缀（可选，如'_thumb'）
        dry_run: 是否为预览模式（True时不实际重命名，只显示）
    """
    images = get_images_sorted(folder_path)
    
    if not images:
        print(f"在文件夹 '{folder_path}' 中未找到图片文件")
        return
    
    print(f"找到 {len(images)} 张图片")
    print(f"{'预览模式' if dry_run else '开始重命名'}...\n")
    
    renamed_count = 0
    max_number = start_number + len(images) - 1
    number_width = len(str(max_number))  # 计算数字位数，用于补零
    
    for i, old_path in enumerate(images):
        # 获取原文件扩展名
        ext = old_path.suffix
        
        # 生成新文件名（支持补零，如 001, 002, ...）
        number = start_number + i
        new_name = f"{prefix}{number:0{number_width}d}{suffix}{ext}"
        new_path = old_path.parent / new_name
        
        # 如果新文件名与旧文件名相同，跳过
        if old_path.name == new_name:
            print(f"跳过: {old_path.name} (文件名已正确)")
            continue
        
        # 如果目标文件已存在且不是同一个文件，先检查
        if new_path.exists() and new_path != old_path:
            print(f"警告: 目标文件已存在，跳过: {old_path.name} -> {new_name}")
            continue
        
        if dry_run:
            print(f"[预览] {old_path.name} -> {new_name}")
        else:
            try:
                old_path.rename(new_path)
                print(f"✓ {old_path.name} -> {new_name}")
                renamed_count += 1
            except Exception as e:
                print(f"✗ 重命名失败: {old_path.name} -> {new_name}")
                print(f"  错误: {e}")
    
    if not dry_run:
        print(f"\n完成！共重命名 {renamed_count} 个文件")


def main():
    if len(sys.argv) < 2:
        print("用法:")
        print(f"  python {sys.argv[0]} <文件夹路径> [选项]")
        print("\n选项:")
        print("  --start <数字>    起始编号（默认: 1）")
        print("  --prefix <前缀>   文件名前缀（可选）")
        print("  --suffix <后缀>   文件名后缀（可选）")
        print("  --dry-run         预览模式（不实际重命名）")
        print("\n示例:")
        print(f"  python {sys.argv[0]} ./images")
        print(f"  python {sys.argv[0]} ./images --start 1")
        print(f"  python {sys.argv[0]} ./images --prefix img_ --start 1")
        print(f"  python {sys.argv[0]} ./images --dry-run  # 预览模式")
        sys.exit(1)
    
    folder_path = sys.argv[1]
    start_number = 1
    prefix = ''
    suffix = ''
    dry_run = False
    
    # 解析命令行参数
    i = 2
    while i < len(sys.argv):
        arg = sys.argv[i]
        if arg == '--start' and i + 1 < len(sys.argv):
            start_number = int(sys.argv[i + 1])
            i += 2
        elif arg == '--prefix' and i + 1 < len(sys.argv):
            prefix = sys.argv[i + 1]
            i += 2
        elif arg == '--suffix' and i + 1 < len(sys.argv):
            suffix = sys.argv[i + 1]
            i += 2
        elif arg == '--dry-run':
            dry_run = True
            i += 1
        else:
            print(f"未知参数: {arg}")
            sys.exit(1)
    
    # 执行重命名
    rename_images(folder_path, start_number, prefix, suffix, dry_run)


if __name__ == "__main__":
    main()

