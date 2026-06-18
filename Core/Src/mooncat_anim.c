/**
 ****************************************************************************************************
 * @file        mooncat_anim.c
 * @brief       "月薪喵跳舞" 动画引擎实现
 *
 *              双模式:
 *              - MOONCAT_USE_REAL_FRAMES: 从 Flash const 数组读取 RGB565 帧并绘制
 *              - 默认: 用几何图形实时绘制占位帧（不同颜色/位置的方块模拟舞蹈动作）
 *
 *              使用 LCD_DispFlush 高效整帧刷新, 减少闪屏
 ****************************************************************************************************
 */

#include "mooncat_anim.h"

#ifdef MOONCAT_USE_REAL_FRAMES
#include "mooncat_frames.h"  /* 真实帧数据 (const RGB565 数组) */
#endif

/* ---- 外部 LCD API ---- */
extern void LCD_Init(void);
extern void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);
extern void LCD_String(uint16_t x, uint16_t y, char *pFont, uint8_t size, uint32_t fColor, uint32_t bColor);
extern void LCD_DispFlush(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pData);

/*===========================================================================
 * 占位帧颜色调色板 (RGB565)
 *===========================================================================*/
#define PLACEHOLDER_CAT_BODY    0xFEE0   /* 浅橙色 (猫身)                      */
#define PLACEHOLDER_CAT_HEAD    0xFE80   /* 橙色 (猫头)                        */
#define PLACEHOLDER_CAT_EAR     0xFC00   /* 深橙 (耳朵)                        */
#define PLACEHOLDER_CAT_EYE     0x0000   /* 黑色 (眼睛)                        */
#define PLACEHOLDER_CAT_MOUTH   0xF800   /* 红色 (嘴)                          */
#define PLACEHOLDER_CAT_ARM     0xFDA0   /* 中橙 (手臂)                        */
#define PLACEHOLDER_CAT_LEG     0xFD20   /* 深中橙 (腿)                        */
#define PLACEHOLDER_CAT_TAIL    0xFE60   /* 尾橙 (尾巴)                        */
#define PLACEHOLDER_CAT_MONEY   0xFFE0   /* 黄色 (金币/工资袋)                 */
#define PLACEHOLDER_BG          0x0000   /* 黑色背景                           */

/* ---- 辅助: 绘制一个填充矩形到精灵局部坐标 ---- */
static void drawRectLocal(uint16_t lx, uint16_t ly, uint16_t w, uint16_t h, uint16_t color)
{
    /* 裁剪到精灵区域 */
    if (lx >= MCAT_SPRITE_WIDTH || ly >= MCAT_SPRITE_HEIGHT) return;
    if (lx + w > MCAT_SPRITE_WIDTH)  w = MCAT_SPRITE_WIDTH - lx;
    if (ly + h > MCAT_SPRITE_HEIGHT) h = MCAT_SPRITE_HEIGHT - ly;
    if (w == 0 || h == 0) return;

    uint16_t sx = MCAT_SPRITE_POS_X + lx;
    uint16_t sy = MCAT_SPRITE_POS_Y + ly;
    uint16_t ex = sx + w - 1;
    uint16_t ey = sy + h - 1;
    LCD_Fill(sx, sy, ex, ey, color);
}

/*===========================================================================
 * 占位帧绘制: 每帧用简单几何体拼出"跳舞猫"形象
 *
 * 坐标系说明:
 *   精灵区域 80×80 像素, 左上角为 (0,0)
 *   drawRectLocal(x, y, w, h, color): 以局部坐标绘制
 *
 * 帧设计 (简单猫跳舞蹈):
 *   Frame 0: 站立, 双手平举
 *   Frame 1: 右手上挥, 左脚抬起
 *   Frame 2: 双手上举, 双脚并拢 (跳起)
 *   Frame 3: 站立, 左手叉腰, 右手上挥
 *   Frame 4: 右手叉腰, 左手上挥, 右脚抬起
 *   Frame 5: 双手下摆, 半蹲
 *===========================================================================*/

#ifdef MOONCAT_USE_REAL_FRAMES
/* ---- 使用 Flash 中的真实帧数据 ---- */
static void drawRealFrame(uint8_t frameIdx)
{
    if (frameIdx >= MCAT_FRAME_COUNT) return;
    LCD_DispFlush(MCAT_SPRITE_POS_X, MCAT_SPRITE_POS_Y,
                  MCAT_SPRITE_WIDTH, MCAT_SPRITE_HEIGHT,
                  mooncat_frames[frameIdx]);
}
#endif

/* ---- 占位帧 0: 站立, 双手平举 ---- */
static void drawPlaceholderFrame0(void)
{
    /* 身体 (20,32) 40×28 */
    drawRectLocal(20, 32, 40, 28, PLACEHOLDER_CAT_BODY);
    /* 头 (25,8) 30×26 */
    drawRectLocal(25, 8, 30, 26, PLACEHOLDER_CAT_HEAD);
    /* 左耳 (26,2) 8×8 */
    drawRectLocal(26, 2, 8, 8, PLACEHOLDER_CAT_EAR);
    /* 右耳 (46,2) 8×8 */
    drawRectLocal(46, 2, 8, 8, PLACEHOLDER_CAT_EAR);
    /* 左眼 (30,16) 5×5 */
    drawRectLocal(30, 16, 5, 5, PLACEHOLDER_CAT_EYE);
    /* 右眼 (43,16) 5×5 */
    drawRectLocal(43, 16, 5, 5, PLACEHOLDER_CAT_EYE);
    /* 嘴 (36,24) 8×3 */
    drawRectLocal(36, 24, 8, 3, PLACEHOLDER_CAT_MOUTH);
    /* 左臂平举 (4,34) 18×7 */
    drawRectLocal(4, 34, 18, 7, PLACEHOLDER_CAT_ARM);
    /* 右臂平举 (58,34) 18×7 */
    drawRectLocal(58, 34, 18, 7, PLACEHOLDER_CAT_ARM);
    /* 左腿 (24,60) 11×18 */
    drawRectLocal(24, 60, 11, 18, PLACEHOLDER_CAT_LEG);
    /* 右腿 (45,60) 11×18 */
    drawRectLocal(45, 60, 11, 18, PLACEHOLDER_CAT_LEG);
    /* 尾巴 (0,38) 12×6 */
    drawRectLocal(0, 38, 12, 6, PLACEHOLDER_CAT_TAIL);
    /* 金币 (12,30) 10×10 */
    drawRectLocal(12, 30, 10, 10, PLACEHOLDER_CAT_MONEY);
}

/* ---- 占位帧 1: 右手上挥, 左脚微抬 ---- */
static void drawPlaceholderFrame1(void)
{
    drawRectLocal(20, 32, 40, 28, PLACEHOLDER_CAT_BODY);
    drawRectLocal(25, 8, 30, 26, PLACEHOLDER_CAT_HEAD);
    drawRectLocal(26, 2, 8, 8, PLACEHOLDER_CAT_EAR);
    drawRectLocal(46, 2, 8, 8, PLACEHOLDER_CAT_EAR);
    drawRectLocal(30, 16, 5, 5, PLACEHOLDER_CAT_EYE);
    drawRectLocal(43, 16, 5, 5, PLACEHOLDER_CAT_EYE);
    drawRectLocal(36, 24, 8, 3, PLACEHOLDER_CAT_MOUTH);
    /* 左臂微抬 */
    drawRectLocal(2, 30, 20, 6, PLACEHOLDER_CAT_ARM);
    /* 右臂上挥 */
    drawRectLocal(60, 20, 6, 18, PLACEHOLDER_CAT_ARM);
    /* 左脚微抬 (向左偏移) */
    drawRectLocal(20, 60, 11, 16, PLACEHOLDER_CAT_LEG);
    /* 右腿正常 */
    drawRectLocal(45, 60, 11, 18, PLACEHOLDER_CAT_LEG);
    /* 尾巴上翘 */
    drawRectLocal(0, 28, 8, 12, PLACEHOLDER_CAT_TAIL);
    drawRectLocal(12, 30, 10, 10, PLACEHOLDER_CAT_MONEY);
}

/* ---- 占位帧 2: 双手上举, 双脚微开 (跳跃感) ---- */
static void drawPlaceholderFrame2(void)
{
    drawRectLocal(20, 28, 40, 28, PLACEHOLDER_CAT_BODY);
    drawRectLocal(25, 4, 30, 26, PLACEHOLDER_CAT_HEAD);
    drawRectLocal(26, 0, 8, 6, PLACEHOLDER_CAT_EAR);
    drawRectLocal(46, 0, 8, 6, PLACEHOLDER_CAT_EAR);
    drawRectLocal(30, 12, 5, 5, PLACEHOLDER_CAT_EYE);
    drawRectLocal(43, 12, 5, 5, PLACEHOLDER_CAT_EYE);
    /* 张嘴 (开心跳起) */
    drawRectLocal(36, 20, 8, 5, PLACEHOLDER_CAT_MOUTH);
    /* 左臂上举 */
    drawRectLocal(2, 14, 6, 18, PLACEHOLDER_CAT_ARM);
    /* 右臂上举 */
    drawRectLocal(72, 14, 6, 18, PLACEHOLDER_CAT_ARM);
    /* 双腿微开 */
    drawRectLocal(22, 56, 11, 20, PLACEHOLDER_CAT_LEG);
    drawRectLocal(47, 56, 11, 20, PLACEHOLDER_CAT_LEG);
    /* 尾巴摇摆 */
    drawRectLocal(2, 42, 14, 5, PLACEHOLDER_CAT_TAIL);
    drawRectLocal(12, 26, 10, 10, PLACEHOLDER_CAT_MONEY);
}

/* ---- 占位帧 3: 左手叉腰, 右手上挥, 身体微侧 ---- */
static void drawPlaceholderFrame3(void)
{
    drawRectLocal(18, 32, 42, 28, PLACEHOLDER_CAT_BODY);
    drawRectLocal(22, 8, 32, 26, PLACEHOLDER_CAT_HEAD);
    drawRectLocal(23, 2, 8, 8, PLACEHOLDER_CAT_EAR);
    drawRectLocal(44, 2, 8, 8, PLACEHOLDER_CAT_EAR);
    drawRectLocal(27, 16, 5, 5, PLACEHOLDER_CAT_EYE);
    drawRectLocal(41, 16, 5, 5, PLACEHOLDER_CAT_EYE);
    drawRectLocal(33, 24, 8, 3, PLACEHOLDER_CAT_MOUTH);
    /* 左手叉腰 (短臂在身体侧边) */
    drawRectLocal(4, 36, 16, 6, PLACEHOLDER_CAT_ARM);
    /* 右手上挥 */
    drawRectLocal(62, 18, 6, 20, PLACEHOLDER_CAT_ARM);
    drawRectLocal(22, 60, 11, 18, PLACEHOLDER_CAT_LEG);
    drawRectLocal(44, 60, 11, 18, PLACEHOLDER_CAT_LEG);
    drawRectLocal(0, 40, 10, 6, PLACEHOLDER_CAT_TAIL);
    drawRectLocal(10, 28, 10, 10, PLACEHOLDER_CAT_MONEY);
}

/* ---- 占位帧 4: 右手叉腰, 左手上挥, 右脚微抬 ---- */
static void drawPlaceholderFrame4(void)
{
    drawRectLocal(20, 32, 40, 28, PLACEHOLDER_CAT_BODY);
    drawRectLocal(25, 8, 30, 26, PLACEHOLDER_CAT_HEAD);
    drawRectLocal(26, 2, 8, 8, PLACEHOLDER_CAT_EAR);
    drawRectLocal(46, 2, 8, 8, PLACEHOLDER_CAT_EAR);
    drawRectLocal(30, 16, 5, 5, PLACEHOLDER_CAT_EYE);
    drawRectLocal(43, 16, 5, 5, PLACEHOLDER_CAT_EYE);
    drawRectLocal(36, 24, 8, 3, PLACEHOLDER_CAT_MOUTH);
    /* 左手上挥 */
    drawRectLocal(2, 18, 6, 20, PLACEHOLDER_CAT_ARM);
    /* 右手叉腰 */
    drawRectLocal(60, 36, 16, 6, PLACEHOLDER_CAT_ARM);
    drawRectLocal(24, 60, 11, 18, PLACEHOLDER_CAT_LEG);
    /* 右脚微抬 */
    drawRectLocal(47, 58, 11, 16, PLACEHOLDER_CAT_LEG);
    drawRectLocal(0, 34, 6, 14, PLACEHOLDER_CAT_TAIL);
    drawRectLocal(64, 28, 10, 10, PLACEHOLDER_CAT_MONEY);
}

/* ---- 占位帧 5: 半蹲, 双手下摆, 看起来像收尾动作 ---- */
static void drawPlaceholderFrame5(void)
{
    drawRectLocal(20, 30, 40, 26, PLACEHOLDER_CAT_BODY);
    drawRectLocal(25, 6, 30, 26, PLACEHOLDER_CAT_HEAD);
    drawRectLocal(26, 0, 8, 8, PLACEHOLDER_CAT_EAR);
    drawRectLocal(46, 0, 8, 8, PLACEHOLDER_CAT_EAR);
    drawRectLocal(30, 14, 5, 5, PLACEHOLDER_CAT_EYE);
    drawRectLocal(43, 14, 5, 5, PLACEHOLDER_CAT_EYE);
    /* 微笑 */
    drawRectLocal(36, 22, 8, 2, PLACEHOLDER_CAT_MOUTH);
    /* 双手下摆 */
    drawRectLocal(6, 42, 16, 6, PLACEHOLDER_CAT_ARM);
    drawRectLocal(58, 42, 16, 6, PLACEHOLDER_CAT_ARM);
    /* 半蹲双腿 */
    drawRectLocal(22, 54, 12, 22, PLACEHOLDER_CAT_LEG);
    drawRectLocal(46, 54, 12, 22, PLACEHOLDER_CAT_LEG);
    /* 尾巴下垂 */
    drawRectLocal(0, 50, 12, 5, PLACEHOLDER_CAT_TAIL);
    drawRectLocal(34, 30, 10, 10, PLACEHOLDER_CAT_MONEY);
}

/* ---- 占位帧绘制函数表 ---- */
typedef void (*drawPlaceholderFunc)(void);
static const drawPlaceholderFunc placeholderDrawers[MCAT_FRAME_COUNT] = {
    drawPlaceholderFrame0,
    drawPlaceholderFrame1,
    drawPlaceholderFrame2,
    drawPlaceholderFrame3,
    drawPlaceholderFrame4,
    drawPlaceholderFrame5,
};

/*===========================================================================
 * 公共 API 实现
 *===========================================================================*/

/**
 * @brief  动画引擎初始化
 */
void MoonCat_Init(void)
{
    LCD_Init();
    /* 清屏 */
    LCD_Fill(0, 0, MCAT_SCREEN_WIDTH - 1, MCAT_SCREEN_HEIGHT - 1, MCAT_BG_COLOR);
}

/**
 * @brief  绘制指定帧
 */
void MoonCat_DrawFrame(uint8_t frameIdx)
{
    if (frameIdx >= MCAT_FRAME_COUNT) return;

#ifdef MOONCAT_USE_REAL_FRAMES
    drawRealFrame(frameIdx);
#else
    /* 先用背景色清空精灵区域, 避免残影 */
    MoonCat_ClearSpriteArea();
    if (placeholderDrawers[frameIdx] != NULL) {
        placeholderDrawers[frameIdx]();
    }
#endif
}

/**
 * @brief  播放一轮完整动画
 */
void MoonCat_PlayOneLoop(void)
{
    for (uint8_t i = 0; i < MCAT_FRAME_COUNT; i++)
    {
        MoonCat_DrawFrame(i);
        HAL_Delay(MCAT_FRAME_DELAY_MS);
    }
}

/**
 * @brief  清空精灵区域
 */
void MoonCat_ClearSpriteArea(void)
{
    LCD_Fill(MCAT_SPRITE_POS_X, MCAT_SPRITE_POS_Y,
             MCAT_SPRITE_POS_X + MCAT_SPRITE_WIDTH - 1,
             MCAT_SPRITE_POS_Y + MCAT_SPRITE_HEIGHT - 1,
             MCAT_BG_COLOR);
}

/**
 * @brief  显示顶部标题 "月薪喵"
 */
void MoonCat_DrawTitle(void)
{
    /* 在屏幕顶部居中显示 "月薪喵" 文字 */
    /* 使用 24 号字体, 白色文字, 黑色背景 */
    /* 注意: LCD_String 不支持中文 (需要W25Q128外部字库) */
    /* 这里用英文占位, 后续用户可替换为中文显示方案 */

    /* 方案: 在顶部画一个标题背景条 */
    LCD_Fill(0, 0, MCAT_SCREEN_WIDTH - 1, 30, 0x2104);  /* 深蓝灰标题栏 */

    /* 显示英文标题作为占位 (LCD_String 仅支持 ASCII) */
    LCD_String(30, 4, (char *)"Dancing MoonCat!", 16, 0xFFFF, 0x2104);

    /* 用户如需显示中文 "月薪喵":
     *   - 方案A: 使用 LCD_ShowChinese + 自取中文字模 (放入 FONT.H)
     *   - 方案B: 用一张中文标题图片通过 LCD_Image 显示
     *   - 方案C: 使用 W25Q128 外部字库 (需确认硬件连接)
     */
}
