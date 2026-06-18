/**
 ****************************************************************************************************
 * @file        mooncat_anim.h
 * @brief       "月薪喵跳舞" 动画引擎 — 配置与API
 *
 *              支持两种帧数据来源:
 *              1. MOONCAT_USE_REAL_FRAMES — 使用 Flash 中的 const RGB565 帧数组
 *              2. 默认 — 使用运行时几何图形绘制占位帧（无需额外 Flash）
 *
 *              ILI9341 240×320 TFT LCD, FSMC 接口, RGB565 颜色格式
 *              STM32F407VET6, 512KB Flash / 192KB SRAM
 ****************************************************************************************************
 */

#ifndef __MOONCAT_ANIM_H
#define __MOONCAT_ANIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/*===========================================================================
 * 硬件/屏幕配置 (可根据实际硬件修改，无需改硬件)
 *===========================================================================*/
#define MCAT_SCREEN_WIDTH       240     /* 屏幕宽度 (像素)                    */
#define MCAT_SCREEN_HEIGHT      320     /* 屏幕高度 (像素)                    */

/*===========================================================================
 * 动画精灵配置
 *===========================================================================*/
#define MCAT_SPRITE_WIDTH       80      /* 精灵帧宽度 (像素)                  */
#define MCAT_SPRITE_HEIGHT      80      /* 精灵帧高度 (像素)                  */
#define MCAT_SPRITE_POS_X       ((MCAT_SCREEN_WIDTH  - MCAT_SPRITE_WIDTH)  / 2)  /* 居中X */
#define MCAT_SPRITE_POS_Y       ((MCAT_SCREEN_HEIGHT - MCAT_SPRITE_HEIGHT) / 2)  /* 居中Y */

/*===========================================================================
 * 动画播放配置
 *===========================================================================*/
#define MCAT_FRAME_COUNT        6       /* 动画帧总数                         */
#define MCAT_FRAME_DELAY_MS     150     /* 每帧显示时长 (毫秒), 约 6.7 FPS    */
#define MCAT_BG_COLOR           0x0000  /* 背景颜色 (BLACK)                   */

/*===========================================================================
 * 编译选项: 定义此宏使用 Flash 中的真实帧数据
 * 未定义时使用几何图形占位帧 (无需 Python 脚本生成)
 *
 * 当你有真实"月薪喵"帧数据后:
 *   1. 用 tools/png_to_carray.py 转换 PNG/GIF 序列
 *   2. 将输出粘贴到 mooncat_frames.h
 *   3. 在下面取消注释 #define MOONCAT_USE_REAL_FRAMES
 *===========================================================================*/
/* #define MOONCAT_USE_REAL_FRAMES */

/*===========================================================================
 * 公共 API
 *===========================================================================*/

/**
 * @brief  动画引擎初始化
 * @note   必须在使用其他 API 前调用; 自动初始化 LCD
 */
void MoonCat_Init(void);

/**
 * @brief  播放一轮完整动画 (MCAT_FRAME_COUNT 帧, 每帧 MCAT_FRAME_DELAY_MS)
 * @note   阻塞式调用, 播放完成后返回
 */
void MoonCat_PlayOneLoop(void);

/**
 * @brief  绘制指定帧到屏幕 (不延时)
 * @param  frameIdx  帧索引 [0, MCAT_FRAME_COUNT-1]
 */
void MoonCat_DrawFrame(uint8_t frameIdx);

/**
 * @brief  在屏幕顶部显示标题文字 "月薪喵"
 * @note   在动画开始前调用, 只画一次
 */
void MoonCat_DrawTitle(void);

/**
 * @brief  清空精灵区域 (填充背景色)
 */
void MoonCat_ClearSpriteArea(void);

#ifdef __cplusplus
}
#endif

#endif /* __MOONCAT_ANIM_H */
