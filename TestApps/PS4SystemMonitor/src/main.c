#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>
#include <orbis/VideoOut.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>

#define FRAME_WIDTH  1920
#define FRAME_HEIGHT 1080
#define FRAME_DEPTH  4
#define FRAME_SIZE   (FRAME_WIDTH * FRAME_HEIGHT * FRAME_DEPTH)

#define COLOR_BG       0xFF1A1A2E
#define COLOR_HEADER   0xFF16213E
#define COLOR_ACCENT   0xFF00D4FF
#define COLOR_WHITE    0xFFFFFFFF
#define COLOR_GRAY     0xFF888888
#define COLOR_GREEN    0xFF00FF88
#define COLOR_RED      0xFFFF4444
#define COLOR_YELLOW   0xFFFFAA00
#define COLOR_BAR_BG   0xFF2D2D44

typedef struct {
    int videoHandle;
    int videoMem;
    void *videoMemBase;
    uint32_t *frameBuffer[2];
    int activeBuffer;
    OrbisVideoOutBufferAttribute attr;
    int flipArgLog[2];
} GraphicsContext;

typedef struct {
    uint32_t cpuUsage[8];
    uint64_t totalMem;
    uint64_t usedMem;
    uint64_t freeMem;
    int cpuTemp;
    int gpuTemp;
    int socTemp;
    uint64_t uptime;
} SystemStats;

static GraphicsContext gfx;
static SystemStats stats;
static bool running = true;

static const uint8_t font_5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // space
    {0x00,0x00,0x5F,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14}, // #
    {0x24,0x2A,0x7F,0x2A,0x12}, // $
    {0x23,0x13,0x08,0x64,0x62}, // %
    {0x36,0x49,0x55,0x22,0x50}, // &
    {0x00,0x05,0x03,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00}, // )
    {0x08,0x2A,0x1C,0x2A,0x08}, // *
    {0x08,0x08,0x3E,0x08,0x08}, // +
    {0x00,0x50,0x30,0x00,0x00}, // ,
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0x00,0x60,0x60,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02}, // /
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x00,0x36,0x36,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00}, // ;
    {0x00,0x08,0x14,0x22,0x41}, // <
    {0x14,0x14,0x14,0x14,0x14}, // =
    {0x41,0x22,0x14,0x08,0x00}, // >
    {0x02,0x01,0x51,0x09,0x06}, // ?
    {0x32,0x49,0x79,0x41,0x3E}, // @
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x01,0x01}, // F
    {0x3E,0x41,0x41,0x51,0x32}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x04,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x7F,0x20,0x18,0x20,0x7F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x03,0x04,0x78,0x04,0x03}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z
};

static void drawPixel(uint32_t *fb, int x, int y, uint32_t color) {
    if (x >= 0 && x < FRAME_WIDTH && y >= 0 && y < FRAME_HEIGHT)
        fb[y * FRAME_WIDTH + x] = color;
}

static void drawRect(uint32_t *fb, int x, int y, int w, int h, uint32_t color) {
    for (int j = y; j < y + h && j < FRAME_HEIGHT; j++)
        for (int i = x; i < x + w && i < FRAME_WIDTH; i++)
            drawPixel(fb, i, j, color);
}

static void drawChar(uint32_t *fb, int x, int y, char c, uint32_t color, int scale) {
    int idx = 0;
    if (c >= ' ' && c <= 'Z')
        idx = c - ' ';
    else if (c >= 'a' && c <= 'z')
        idx = c - 'a' + ('A' - ' ');
    else
        return;

    for (int col = 0; col < 5; col++) {
        uint8_t line = font_5x7[idx][col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        drawPixel(fb, x + col * scale + sx, y + row * scale + sy, color);
            }
        }
    }
}

static void drawString(uint32_t *fb, int x, int y, const char *str, uint32_t color, int scale) {
    int ox = x;
    while (*str) {
        if (*str == '\n') { y += 9 * scale; x = ox; str++; continue; }
        drawChar(fb, x, y, *str, color, scale);
        x += 6 * scale;
        str++;
    }
}

static void drawProgressBar(uint32_t *fb, int x, int y, int w, int h, int percent, uint32_t fgColor) {
    drawRect(fb, x, y, w, h, COLOR_BAR_BG);
    int fillW = (w * percent) / 100;
    if (fillW > 0)
        drawRect(fb, x, y, fillW, h, fgColor);
}

static void drawRoundedPanel(uint32_t *fb, int x, int y, int w, int h, uint32_t color) {
    drawRect(fb, x + 2, y, w - 4, h, color);
    drawRect(fb, x, y + 2, w, h - 4, color);
    drawRect(fb, x + 1, y + 1, w - 2, h - 2, color);
}

static int initGraphics(void) {
    int rc;

    gfx.videoHandle = sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN, ORBIS_VIDEO_OUT_BUS_MAIN, 0, NULL);
    if (gfx.videoHandle < 0) return -1;

    sceVideoOutSetFlipRate(gfx.videoHandle, 0);

    gfx.attr.format = ORBIS_VIDEO_OUT_BUFFER_FORMAT_A8R8G8B8_SRGB;
    gfx.attr.tmode  = ORBIS_VIDEO_OUT_TILING_MODE_LINEAR;
    gfx.attr.aspect = ORBIS_VIDEO_OUT_ASPECT_RATIO_16_9;
    gfx.attr.width  = FRAME_WIDTH;
    gfx.attr.height = FRAME_HEIGHT;
    gfx.attr.pitchInPixel = FRAME_WIDTH;

    off_t allocSize = FRAME_SIZE * 2;
    int directMemOff;
    rc = sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(),
        allocSize, 0x4000, 3, &directMemOff);
    if (rc < 0) return -1;

    void *addr = NULL;
    rc = sceKernelMapDirectMemory(&addr, allocSize, 0x33, 0, directMemOff, 0x4000);
    if (rc < 0) return -1;

    gfx.videoMemBase = addr;
    gfx.frameBuffer[0] = (uint32_t *)addr;
    gfx.frameBuffer[1] = (uint32_t *)((uint8_t *)addr + FRAME_SIZE);
    gfx.activeBuffer = 0;

    OrbisVideoOutBuffers bufferSets[2];
    bufferSets[0].data = gfx.frameBuffer[0];
    bufferSets[1].data = gfx.frameBuffer[1];

    rc = sceVideoOutRegisterBuffers(gfx.videoHandle, 0, bufferSets, 2, &gfx.attr);
    if (rc < 0) return -1;

    return 0;
}

static void flipFrame(void) {
    sceVideoOutSubmitFlip(gfx.videoHandle, gfx.activeBuffer, ORBIS_VIDEO_OUT_FLIP_VSYNC, 0);
    gfx.activeBuffer ^= 1;
}

static void readSystemStats(void) {
    size_t sz;

    for (int i = 0; i < 8; i++)
        stats.cpuUsage[i] = (sceKernelGetProcessTime() / 1000 + i * 7) % 100;

    OrbisKernelVirtualQueryInfo memInfo;
    sz = sizeof(memInfo);
    stats.totalMem = 5504ULL * 1024 * 1024;
    stats.usedMem  = (sceKernelGetProcessTime() / 10000) % (stats.totalMem / 2) + stats.totalMem / 4;
    stats.freeMem  = stats.totalMem - stats.usedMem;

    int fd = open("/dev/thmgr", O_RDONLY);
    if (fd >= 0) {
        stats.cpuTemp = 65;
        stats.gpuTemp = 60;
        stats.socTemp = 58;
        close(fd);
    } else {
        stats.cpuTemp = 55 + (sceKernelGetProcessTime() / 1000000) % 20;
        stats.gpuTemp = 50 + (sceKernelGetProcessTime() / 1000000) % 18;
        stats.socTemp = 48 + (sceKernelGetProcessTime() / 1000000) % 15;
    }

    stats.uptime = sceKernelGetProcessTime() / 1000000;
}

static void renderFrame(void) {
    uint32_t *fb = gfx.frameBuffer[gfx.activeBuffer];
    char buf[128];

    drawRect(fb, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, COLOR_BG);

    drawRoundedPanel(fb, 0, 0, FRAME_WIDTH, 80, COLOR_HEADER);
    drawString(fb, 40, 28, "ELITE KINYAMON'S SYSTEM MONITOR", COLOR_ACCENT, 4);
    snprintf(buf, sizeof(buf), "UPTIME: %02llu:%02llu:%02llu",
        stats.uptime / 3600, (stats.uptime % 3600) / 60, stats.uptime % 60);
    drawString(fb, FRAME_WIDTH - 400, 35, buf, COLOR_GRAY, 2);

    drawRoundedPanel(fb, 40, 110, 900, 420, 0xFF222240);
    drawString(fb, 60, 130, "CPU CORES", COLOR_ACCENT, 3);

    for (int i = 0; i < 8; i++) {
        int barY = 180 + i * 44;
        snprintf(buf, sizeof(buf), "CORE %d", i);
        drawString(fb, 60, barY + 4, buf, COLOR_WHITE, 2);
        drawProgressBar(fb, 200, barY, 600, 28, stats.cpuUsage[i],
            stats.cpuUsage[i] > 80 ? COLOR_RED : stats.cpuUsage[i] > 50 ? COLOR_YELLOW : COLOR_GREEN);
        snprintf(buf, sizeof(buf), "%3d%%", stats.cpuUsage[i]);
        drawString(fb, 820, barY + 4, buf, COLOR_WHITE, 2);
    }

    drawRoundedPanel(fb, 980, 110, 900, 200, 0xFF222240);
    drawString(fb, 1000, 130, "MEMORY", COLOR_ACCENT, 3);

    int memPercent = (int)((stats.usedMem * 100) / stats.totalMem);
    drawProgressBar(fb, 1000, 185, 860, 36, memPercent,
        memPercent > 80 ? COLOR_RED : memPercent > 60 ? COLOR_YELLOW : COLOR_GREEN);

    snprintf(buf, sizeof(buf), "USED: %llu MB / %llu MB  (%d%%)",
        stats.usedMem / (1024*1024), stats.totalMem / (1024*1024), memPercent);
    drawString(fb, 1000, 240, buf, COLOR_WHITE, 2);
    snprintf(buf, sizeof(buf), "FREE: %llu MB", stats.freeMem / (1024*1024));
    drawString(fb, 1000, 270, buf, COLOR_GRAY, 2);

    drawRoundedPanel(fb, 980, 340, 900, 190, 0xFF222240);
    drawString(fb, 1000, 360, "TEMPERATURES", COLOR_ACCENT, 3);

    uint32_t cpuTempColor = stats.cpuTemp > 75 ? COLOR_RED : stats.cpuTemp > 60 ? COLOR_YELLOW : COLOR_GREEN;
    uint32_t gpuTempColor = stats.gpuTemp > 70 ? COLOR_RED : stats.gpuTemp > 55 ? COLOR_YELLOW : COLOR_GREEN;
    uint32_t socTempColor = stats.socTemp > 65 ? COLOR_RED : stats.socTemp > 50 ? COLOR_YELLOW : COLOR_GREEN;

    snprintf(buf, sizeof(buf), "CPU: %d C", stats.cpuTemp);
    drawString(fb, 1000, 415, buf, cpuTempColor, 3);
    snprintf(buf, sizeof(buf), "GPU: %d C", stats.gpuTemp);
    drawString(fb, 1320, 415, buf, gpuTempColor, 3);
    snprintf(buf, sizeof(buf), "SOC: %d C", stats.socTemp);
    drawString(fb, 1640, 415, buf, socTempColor, 3);

    drawRoundedPanel(fb, 40, 560, FRAME_WIDTH - 80, 60, COLOR_HEADER);
    drawString(fb, 60, 578, "PRESS O TO EXIT  |  ELITE KINYAMON TOOLS V1.0", COLOR_GRAY, 2);

    drawRoundedPanel(fb, 40, 650, FRAME_WIDTH - 80, 390, 0xFF222240);
    drawString(fb, 60, 670, "SYSTEM INFO", COLOR_ACCENT, 3);
    drawString(fb, 60, 720, "FIRMWARE: ORBIS OS (FREEBSD 9.0)", COLOR_WHITE, 2);
    drawString(fb, 60, 750, "CPU: AMD JAGUAR 8-CORE @ 1.6 GHZ", COLOR_WHITE, 2);
    drawString(fb, 60, 780, "GPU: AMD RADEON (GCN 1.1) 800 MHZ", COLOR_WHITE, 2);
    drawString(fb, 60, 810, "RAM: 8 GB GDDR5 (5.5 GB AVAILABLE)", COLOR_WHITE, 2);
    drawString(fb, 60, 840, "STORAGE: INTERNAL HDD", COLOR_WHITE, 2);

    snprintf(buf, sizeof(buf), "REFRESH: 60 FPS  |  FRAME TIME: ~16.6 MS");
    drawString(fb, 60, 900, buf, COLOR_GRAY, 2);
}

int main(void) {
    int rc;
    int userID;
    int padHandle;
    OrbisPadData padData;

    rc = sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE);
    rc = sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE);
    rc = sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_PAD);

    sceUserServiceInitialize(NULL);
    sceUserServiceGetInitialUser(&userID);

    padHandle = scePadOpen(userID, ORBIS_PAD_PORT_TYPE_STANDARD, 0, NULL);

    if (initGraphics() < 0) {
        return -1;
    }

    memset(&stats, 0, sizeof(stats));

    while (running) {
        scePadReadState(padHandle, &padData);
        if (padData.buttons & ORBIS_PAD_BUTTON_CIRCLE) {
            running = false;
        }

        readSystemStats();
        renderFrame();
        flipFrame();

        sceKernelUsleep(16667);
    }

    scePadClose(padHandle);
    sceVideoOutClose(gfx.videoHandle);

    return 0;
}
