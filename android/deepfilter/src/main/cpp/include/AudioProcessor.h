#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <aaudio/AAudio.h>

namespace deepfilter {

/**
 * 音频帧数据结构
 */
struct AudioFrame {
    float* data;
    int32_t numFrames;
    int64_t timestamp;
};

/**
 * 音频处理器类
 * 
 * 功能说明：
 * 1. 集成AAudio音频录制和deepfilter-ort降噪处理
 * 2. 实现录制后立即降噪的完整流程（异步处理）
 * 3. 支持配置降噪参数（tar_bytes、post_filter_beta、atten_lim_db）
 * 4. 提供实时音频降噪处理接口
 * 5. 音频格式固定：48kHz、单声道、PCM_FLOAT
 * 6. 使用异步处理避免阻塞音频采集线程
 * 
 * @author hzexe
 * @version 2.1
 */
class AudioProcessor {
public:
    /**
     * 降噪音频数据回调函数类型
     * 
     * @param audioData 降噪后的音频数据（f32格式，单声道）
     * @param numFrames 帧数
     * @param lsnr LSNR值（信噪比）
     */
    using AudioCallback = std::function<void(const float* audioData, int32_t numFrames, float lsnr)>;

    /**
     * 构造函数
     */
    AudioProcessor();

    /**
     * 析构函数
     */
    ~AudioProcessor();

    /**
     * 初始化音频处理器
     * 
     * @param tarBytes 模型文件字节数组（tar.gz格式）
     * @param tarBytesSize 模型文件字节数组大小
     * @param postFilterBeta 后滤波器beta参数（控制降噪强度）
     * @param attenLimDb 衰减限制（dB）
     * @return true-初始化成功，false-初始化失败
     */
    bool initialize(
        const uint8_t* tarBytes,
        size_t tarBytesSize,
        float postFilterBeta,
        float attenLimDb);

    /**
     * 开始录制和降噪处理
     * 
     * @param callback 降噪音频数据回调函数
     * @return true-开始成功，false-开始失败
     */
    bool start(AudioCallback callback);

    /**
     * 停止录制和降噪处理
     * 
     * @return true-停止成功，false-停止失败
     */
    bool stop();

    /**
     * 检查是否正在处理
     * 
     * @return true-正在处理，false-未处理
     */
    bool isProcessing() const;

    /**
     * 释放资源
     */
    void release();

    /**
     * 检查是否已初始化
     * 
     * @return true-已初始化，false-未初始化
     */
    bool isInitialized() const;

    /**
     * 获取最后的错误信息
     * 
     * @return 错误信息字符串
     */
    const char* getLastError() const;

    /**
     * 设置后滤波器beta参数
     * 
     * @param beta beta参数值
     * @return true-设置成功，false-设置失败
     */
    bool setPostFilterBeta(float beta);

    /**
     * 设置衰减限制
     * 
     * @param attenLimDb 衰减限制（dB）
     * @return true-设置成功，false-设置失败
     */
    bool setAttenLimDb(float attenLimDb);

    /**
     * 获取当前采样率
     * 
     * @return 采样率（Hz），固定为48000
     */
    int32_t getSampleRate() const;

    /**
     * 获取当前声道数
     * 
     * @return 声道数，固定为1
     */
    int32_t getChannelCount() const;

    /**
     * 获取当前帧大小
     * 
     * @return 帧大小（采样点数）
     */
    int32_t getFrameSize() const;

    /**
     * 获取队列大小
     * 
     * @return 当前队列中的帧数
     */
    size_t getQueueSize() const;

private:
    /**
     * AAudio数据回调函数（快速将数据放入队列）
     */
    static aaudio_data_callback_result_t dataCallback(
        AAudioStream* stream,
        void* userData,
        void* audioData,
        int32_t numFrames);

    /**
     * AAudio错误回调函数
     */
    static void errorCallback(
        AAudioStream* stream,
        void* userData,
        aaudio_result_t error);

    /**
     * 异步处理线程函数（从队列取数据进行降噪）
     */
    void processingThreadFunc();

    /**
     * 初始化AAudio流
     */
    bool initAAudioStream();

    /**
     * 关闭AAudio流
     */
    void closeAAudioStream();

    /**
     * 停止处理线程
     */
    void stopProcessingThread();

    /**
     * 分配音频帧
     */
    AudioFrame* allocateAudioFrame(int32_t numFrames);

    /**
     * 释放音频帧
     */
    void freeAudioFrame(AudioFrame* frame);

private:
    // DeepFilterNet状态
    void* dfState_;
    bool dfInitialized_;
    size_t frameSize_;

    // AAudio流
    AAudioStream* aaudioStream_;
    bool aaudioInitialized_;

    // 异步处理线程
    std::thread* processingThread_;
    std::atomic<bool> processingThreadRunning_;
    
    // 音频数据队列
    std::queue<AudioFrame*> audioQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    
    // 回调函数
    AudioCallback callback_;

    // 处理状态
    std::atomic<bool> isProcessing_;

    // 音频格式（固定）
    static const int32_t SAMPLE_RATE = 48000;
    static const int32_t CHANNEL_COUNT = 1;
    
    // 队列最大大小（防止内存溢出）
    static const size_t MAX_QUEUE_SIZE = 10;

    // 错误信息
    char lastError_[256];
};

} // namespace deepfilter

#endif // AUDIO_PROCESSOR_H