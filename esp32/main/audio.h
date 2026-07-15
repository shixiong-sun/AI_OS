/**
 * @file audio.h
 * @brief INMP441 I2S 麦克风驱动
 *
 * 使用 ESP32-S3 的 I2S 接口采集 INMP441 数字麦克风音频。
 *
 * 硬件连接（默认引脚，可通过 menuconfig 修改）：
 *   INMP441 DOUT  -> ESP32 I2S_DIN  (GPIO6)
 *   INMP441 BCLK  -> ESP32 I2S_BCLK (GPIO4)
 *   INMP441 LRCK  -> ESP32 I2S_WS   (GPIO5)
 *   INMP441 L/R   -> GND（左声道模式）
 *   INMP441 VDD   -> 3.3V
 *   INMP441 GND   -> GND
 *
 * 使用流程：
 *   1. audio_init()          -- 初始化 I2S 接口
 *   2. audio_record(dur_ms)  -- 录音，返回 PCM 数据
 *   3. audio_deinit()        -- 释放 I2S 资源
 *
 * 数据格式：
 *   16-bit signed PCM, 16000Hz, 单声道
 */

#ifndef AUDIO_H
#define AUDIO_H

#include "esp_err.h"

/* ── 音频参数 ──────────────────────────────────────────── */
#define AUDIO_SAMPLE_RATE   16000   /* 采样率 16kHz（适合语音） */
#define AUDIO_BITS          16      /* 位深 16-bit */
#define AUDIO_CHANNELS      1       /* 单声道 */
#define AUDIO_MAX_DURATION_MS 10000 /* 最大录音时长 10 秒 */

/**
 * @brief 初始化 I2S 麦克风
 *
 * 配置 ESP32-S3 的 I2S 接口为 RX（接收）模式，
 * 连接 INMP441 数字麦克风。
 *
 * 引脚配置从 menuconfig 读取：
 *   CONFIG_AIOS_I2S_BCLK_PIN
 *   CONFIG_AIOS_I2S_LRCK_PIN
 *   CONFIG_AIOS_I2S_DIN_PIN
 *
 * @return ESP_OK  初始化成功
 * @return ESP_FAIL I2S 驱动安装失败
 */
esp_err_t audio_init(void);

/**
 * @brief 录制一段音频
 *
 * 从 I2S 麦克风读取音频数据到提供的缓冲区。
 * 阻塞调用，直到录制完成或超时。
 *
 * PCM 数据格式：16-bit signed, 16000Hz, 单声道（小端序）
 *
 * @param buf       接收音频数据的缓冲区
 * @param buf_size  缓冲区大小（字节）
 * @param out_len   实际读取的字节数（输出参数）
 *
 * @return ESP_OK  录音成功
 * @return ESP_ERR_INVALID_SIZE 缓冲区太小
 * @return ESP_FAIL I2S 读取错误
 */
esp_err_t audio_record(char *buf, size_t buf_size, size_t *out_len);

/**
 * @brief 释放 I2S 资源
 *
 * 停止录音通道并释放 I2S 驱动。
 * 在切换 WiFi 或进入休眠前调用。
 */
void audio_deinit(void);

#endif /* AUDIO_H */
