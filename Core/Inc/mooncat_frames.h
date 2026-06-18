/**
 ****************************************************************************************************
 * @file        mooncat_frames.h
 * @brief       "月薪喵" 动画帧数据 — RGB565 格式, 存放在 Flash (const)
 *
 *              【如何替换为真实帧数据】
 *
 *              1. 准备一组 PNG/GIF 动画帧 (建议 80×80 像素)
 *              2. 运行 tools/png_to_carray.py 生成帧数组:
 *                   python tools/png_to_carray.py --input frames/ --width 80 --height 80
 *              3. 将生成的 C 数组代码粘贴到本文件中
 *              4. 在 mooncat_anim.h 中取消注释 #define MOONCAT_USE_REAL_FRAMES
 *              5. 重新编译下载
 *
 *              帧数据格式:
 *                 const uint16_t mooncat_frames[MCAT_FRAME_COUNT][MCAT_SPRITE_WIDTH * MCAT_SPRITE_HEIGHT]
 *
 *              每帧数据大小 = 宽 × 高 × 2 字节 (RGB565)
 *              示例: 80×80 = 6,400 像素 → 12,800 字节/帧
 *              6 帧总计 ≈ 75 KB → 完全可放入 STM32F407 512KB Flash
 ****************************************************************************************************
 */

#ifndef __MOONCAT_FRAMES_H
#define __MOONCAT_FRAMES_H

#include "mooncat_anim.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 真实帧数据占位
 *
 * 当你用 Python 脚本生成帧数据后, 替换下面的内容。
 * 当前为占位数据 (纯色测试帧), 确保编译通过。
 *===========================================================================*/

#ifdef MOONCAT_USE_REAL_FRAMES

/*
 * TODO: 将 png_to_carray.py 生成的帧数组粘贴到这里
 *
 * 示例格式:
 *
 * static const uint16_t _frame0[MCAT_SPRITE_WIDTH * MCAT_SPRITE_HEIGHT] = {
 *     0x0000, 0x0000, 0xF800, ...  // 80×80 = 6400 个 RGB565 值
 * };
 * static const uint16_t _frame1[MCAT_SPRITE_WIDTH * MCAT_SPRITE_HEIGHT] = {
 *     ...
 * };
 * ...
 *
 * const uint16_t* const mooncat_frames[MCAT_FRAME_COUNT] = {
 *     _frame0, _frame1, _frame2, _frame3, _frame4, _frame5,
 * };
 */

/* ---- 占位: 单帧纯蓝色方块, 确保编译通过 ---- */
/* 实际使用时会由 png_to_carray.py 生成真实数据替换下方内容 */
static const uint16_t _frame_placeholder[MCAT_SPRITE_WIDTH * MCAT_SPRITE_HEIGHT] = {0x001F /*蓝*/};

const uint16_t* const mooncat_frames[MCAT_FRAME_COUNT] = {
    _frame_placeholder,
    _frame_placeholder,
    _frame_placeholder,
    _frame_placeholder,
    _frame_placeholder,
    _frame_placeholder,
};

#endif /* MOONCAT_USE_REAL_FRAMES */

#ifdef __cplusplus
}
#endif

#endif /* __MOONCAT_FRAMES_H */
