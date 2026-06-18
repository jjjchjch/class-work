#!/usr/bin/env python3
"""
=============================================================================
 png_to_carray.py  —  图片序列 → STM32 C 数组 转换工具
=============================================================================

 功能: 将 PNG / GIF / JPEG 图片序列转换为 RGB565 格式的 C 语言 const 数组,
       可直接用于 mooncat_anim 动画引擎。

 用法:
   python png_to_carray.py --input ./frames/ --width 80 --height 80 [选项]

 参数:
   --input DIR      输入目录, 包含帧图片 (按文件名排序)
   --width W        目标输出宽度 (像素)
   --height H       目标输出高度 (像素)
   --output FILE    输出 C 头文件路径 (默认: mooncat_frames_generated.h)
   --array-name N   数组名前缀 (默认: _mooncat_frame)
   --frame-count N  最大帧数 (默认: 0 = 全部)
   --big-endian     输出大端序 RGB565 (默认: 小端序, 适配 STM32)

 依赖: pip install Pillow

 示例工作流:
   1. 准备 GIF 动画, 用任意工具导出为帧序列 PNG:
      ffmpeg -i cat.gif frames/frame_%03d.png
      或在线工具 https://ezgif.com/split

   2. 运行转换:
      python tools/png_to_carray.py --input frames/ --width 80 --height 80

   3. 将生成的 mooncat_frames_generated.h 内容合并到 Core/Inc/mooncat_frames.h

   4. 在 Core/Inc/mooncat_anim.h 中取消注释:
      #define MOONCAT_USE_REAL_FRAMES

   5. 重新编译下载
=============================================================================
"""

import argparse
import os
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("错误: 需要 Pillow 库. 请运行: pip install Pillow")
    sys.exit(1)


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    """将 8-8-8 RGB 转换为 16-bit RGB565"""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5


def image_to_rgb565(img: Image.Image, width: int, height: int, big_endian: bool = False):
    """
    将 PIL Image 缩放至 width×height, 转为 RGB565 像素列表
    返回: list[int] (每个元素为 16-bit RGB565 值)
    """
    # 缩放到目标尺寸 (使用高质量 LANCZOS 重采样)
    img_resized = img.resize((width, height), Image.LANCZOS)

    # 统一转为 RGB 模式
    if img_resized.mode != 'RGB':
        img_resized = img_resized.convert('RGB')

    pixels = list(img_resized.getdata())  # [(r,g,b), ...]
    rgb565 = [rgb888_to_rgb565(r, g, b) for r, g, b in pixels]
    return rgb565


def format_c_array(pixels: list, array_name: str, width: int, height: int, indent: int = 4) -> str:
    """
    将 RGB565 像素列表格式化为 C 语言 const 数组字符串
    """
    total = width * height
    prefix = " " * indent
    lines = []
    lines.append(f"{prefix}// {width}x{height} RGB565, {total} pixels, {total * 2} bytes")
    lines.append(f"static const uint16_t {array_name}[{total}] = {{")

    # 每行 16 个值
    per_line = 16
    for i in range(0, total, per_line):
        chunk = pixels[i:i + per_line]
        hex_vals = ", ".join(f"0x{v:04X}" for v in chunk)
        if i + per_line < total:
            lines.append(f"{prefix}    {hex_vals},")
        else:
            lines.append(f"{prefix}    {hex_vals}")

    lines.append(f"{prefix}}};")
    return "\n".join(lines)


def format_header(frames: list, width: int, height: int, array_base: str, frame_count: int):
    """
    生成完整的 C 头文件内容
    """
    header = f"""/**
 * 自动生成 — 请勿手动编辑
 * 生成工具: tools/png_to_carray.py
 * 帧数: {frame_count}, 尺寸: {width}×{height} RGB565
 * 每帧大小: {width * height * 2} 字节, 总计: {frame_count * width * height * 2} 字节
 */

#ifndef __MOONCAT_FRAMES_GENERATED_H
#define __MOONCAT_FRAMES_GENERATED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {{
#endif

/*
 * 单个帧数组
 */
"""
    for i, pixels in enumerate(frames):
        name = f"{array_base}_{i}"
        header += format_c_array(pixels, name, width, height) + "\n\n"

    # 帧指针数组
    ptr_names = ", ".join(f"{array_base}_{i}" for i in range(frame_count))
    header += f"""/*
 * 帧指针数组 — 供 mooncat_anim 引擎索引
 */
const uint16_t* const mooncat_frames[{frame_count}] = {{
    {ptr_names},
}};

#ifdef __cplusplus
}}
#endif

#endif /* __MOONCAT_FRAMES_GENERATED_H */
"""
    return header


def main():
    parser = argparse.ArgumentParser(
        description="图片序列 → STM32 RGB565 C 数组转换工具"
    )
    parser.add_argument("--input", "-i", required=True,
                        help="输入目录, 包含帧图片 (PNG/GIF/JPEG)")
    parser.add_argument("--width", "-W", type=int, required=True,
                        help="目标宽度 (像素)")
    parser.add_argument("--height", "-H", type=int, required=True,
                        help="目标高度 (像素)")
    parser.add_argument("--output", "-o", default="mooncat_frames_generated.h",
                        help="输出 C 头文件路径 (默认: mooncat_frames_generated.h)")
    parser.add_argument("--array-name", "-n", default="_mooncat_frame",
                        help="C 数组名前缀 (默认: _mooncat_frame)")
    parser.add_argument("--frame-count", "-c", type=int, default=0,
                        help="最大帧数 (0 = 全部)")
    parser.add_argument("--big-endian", action="store_true",
                        help="输出大端序 (默认小端序, 适配 STM32)")
    args = parser.parse_args()

    input_dir = Path(args.input)
    if not input_dir.is_dir():
        print(f"错误: 输入目录不存在: {input_dir}")
        sys.exit(1)

    # 收集图片文件 (按文件名排序)
    extensions = {'.png', '.gif', '.jpg', '.jpeg', '.bmp', '.webp'}
    img_files = sorted(
        [f for f in input_dir.iterdir() if f.suffix.lower() in extensions]
    )

    if not img_files:
        print(f"错误: 在 {input_dir} 中未找到图片文件")
        print(f"支持的格式: {', '.join(extensions)}")
        sys.exit(1)

    max_frames = args.frame_count if args.frame_count > 0 else len(img_files)
    img_files = img_files[:max_frames]
    frame_count = len(img_files)

    print(f"找到 {len(img_files)} 张图片, 处理中...")

    # 逐帧转换
    frames = []
    for i, fpath in enumerate(img_files):
        try:
            img = Image.open(fpath)
            pixels = image_to_rgb565(img, args.width, args.height, args.big_endian)
            frames.append(pixels)
            print(f"  [{i+1}/{frame_count}] {fpath.name} → {args.width}×{args.height} OK")
        except Exception as e:
            print(f"  [{i+1}/{frame_count}] {fpath.name} → 错误: {e}")
            sys.exit(1)

    # 生成头文件
    header_content = format_header(frames, args.width, args.height, args.array_name, frame_count)
    output_path = Path(args.output)
    output_path.write_text(header_content, encoding="utf-8")

    total_bytes = frame_count * args.width * args.height * 2
    print(f"\n? 生成完成: {output_path}")
    print(f"  帧数: {frame_count}, 尺寸: {args.width}×{args.height}")
    print(f"  总 Flash 占用: {total_bytes:,} 字节 ({total_bytes / 1024:.1f} KB)")
    print(f"\n下一步:")
    print(f"  1. 将 {output_path} 的内容合并到 Core/Inc/mooncat_frames.h")
    print(f"  2. 在 Core/Inc/mooncat_anim.h 中取消注释 #define MOONCAT_USE_REAL_FRAMES")
    print(f"  3. 重新编译下载")


if __name__ == "__main__":
    main()
