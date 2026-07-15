/**
 * @file audio.c
 * @brief INMP441 I2S 麦克风驱动实现
 *
 * 实现流程：
 *   audio_init()
 *     -> i2s_new_channel(RX)     创建 I2S 接收通道
 *     -> i2s_channel_init_std()  配置为标准 I2S 模式（Philips 格式）
 *     -> i2s_channel_enable()    启用通道
 *
 *   audio_record(buf, size)
 *     -> i2s_channel_read()      从 I2S 读取 PCM 数据
 *     -> 返回实际读取字节数
 *
 *   audio_deinit()
 *     -> i2s_channel_disable()   停用通道
 *     -> i2s_del_channel()       删除通道
 *
 * 注意事项：
 *   - INMP441 输出标准 I2S 格式数据
 *   - L/R 引脚接 GND 时使用左声道
 *   - 需要 3.3V 供电，勿使用 5V
 */

#include "audio.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2s_std.h"

/* ── 日志标签 ────────────────────────────────────────── */
static const char *TAG = "aios_audio";

/* ── 全局 I2S 通道句柄 ──────────────────────────────── */
static i2s_chan_handle_t s_rx_handle = NULL;

/* ── DMA 缓冲区大小 ──────────────────────────────────── */
#define DMA_BUF_COUNT  4       /* DMA 缓冲区数量 */
#define DMA_BUF_LEN    1024    /* 每个 DMA 缓冲区样本数 */


esp_err_t audio_init(void)
{
    /* ── 1. 创建 I2S 通道配置 ──────────────────────── */
    /*
     * I2S_NUM_AUTO: 让驱动自动选择可用 I2S 控制器
     * I2S_ROLE_MASTER: ESP32 作为主设备，产生时钟信号
     *                     INMP441 作为从设备，接收时钟
     */
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_AUTO,
        .role = I2S_ROLE_MASTER,
        .dma_desc_count = DMA_BUF_COUNT,
        .dma_frame_num = DMA_BUF_LEN,
        .auto_clear = true,  /* 自动清除旧数据 */
    };

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &s_rx_handle));
    if (!s_rx_handle) {
        ESP_LOGE(TAG, "I2S 通道创建失败");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "I2S 通道已创建");

    /* ── 2. 配置标准 I2S 模式（Philips 格式） ──────── */
    /*
     * INMP441 使用标准 I2S Philips 格式：
     *   - BCLK 下降沿采样
     *   - 数据在 LRCK 切换后延迟 1 个 BCLK 开始
     *
     * 时钟配置：
     *   sample_rate_hz = 16000
     *   clk_src = I2S_CLK_SRC_DEFAULT（自动选择）
     *
     * 时隙配置：
     *   data_bit_width = 16-bit
     *   slot_bit_width = 16-bit
     *   slot_mode = MONO（单声道）
     *   slot_format = I2S_STD_PHILIPS
     *
     * 引脚配置（从 menuconfig 读取）：
     *   BCLK = CONFIG_AIOS_I2S_BCLK_PIN
     *   WS   = CONFIG_AIOS_I2S_LRCK_PIN
     *   DIN  = CONFIG_AIOS_I2S_DIN_PIN
     *   DOUT = I2S_GPIO_UNUSED（只接收，不发送）
     */
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = CONFIG_AIOS_I2S_BCLK_PIN,
            .ws   = CONFIG_AIOS_I2S_LRCK_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din  = CONFIG_AIOS_I2S_DIN_PIN,
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(s_rx_handle, &std_cfg));
    ESP_LOGI(TAG, "I2S 已配置: %dHz, 16-bit, MONO, Philips",
             AUDIO_SAMPLE_RATE);

    /* ── 3. 启用 I2S 接收通道 ──────────────────────── */
    ESP_ERROR_CHECK(i2s_channel_enable(s_rx_handle));
    ESP_LOGI(TAG, "I2S 麦克风已启动");

    return ESP_OK;
}


esp_err_t audio_record(char *buf, size_t buf_size, size_t *out_len)
{
    if (!s_rx_handle) {
        ESP_LOGE(TAG, "I2S 未初始化，请先调用 audio_init()");
        return ESP_FAIL;
    }

    /* ── 计算最大可读取样本数 ──────────────────────── */
    /*
     * 每次读取一个 16-bit 样本。
     * buf_size / 2 = 最大样本数
     */
    size_t samples_to_read = buf_size / (AUDIO_BITS / 8);
    if (samples_to_read == 0) {
        ESP_LOGE(TAG, "缓冲区太小，至少需要 2 字节");
        return ESP_ERR_INVALID_SIZE;
    }

    /* ── 从 I2S 读取 PCM 数据 ──────────────────────── */
    /*
     * i2s_channel_read() 是阻塞调用。
     * 内部使用 DMA，读取 DMA 缓冲区中累积的数据。
     *
     * 参数：
     *   s_rx_handle   - I2S 接收通道句柄
     *   buf           - 接收缓冲区
     *   buf_size      - 请求读取的字节数
     *   &bytes_read   - 实际读取的字节数
     *   portMAX_DELAY - 无限等待
     */
    size_t bytes_read = 0;
    esp_err_t ret = i2s_channel_read(
        s_rx_handle,
        buf,
        buf_size,
        &bytes_read,
        portMAX_DELAY
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S 读取失败: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    if (out_len) {
        *out_len = bytes_read;
    }

    ESP_LOGD(TAG, "读取了 %zu 字节（约 %.1f 秒）",
             bytes_read,
             (float)bytes_read / (AUDIO_SAMPLE_RATE * 2));

    return ESP_OK;
}


void audio_deinit(void)
{
    if (s_rx_handle) {
        i2s_channel_disable(s_rx_handle);
        i2s_del_channel(s_rx_handle);
        s_rx_handle = NULL;
        ESP_LOGI(TAG, "I2S 已释放");
    }
}
