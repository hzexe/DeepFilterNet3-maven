#include "AudioProcessor.h"
#include <android/log.h>
#include <cstring>
#include <chrono>

#define LOG_TAG "AudioProcessor"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace deepfilter {

extern "C" {
    /**
     * 创建DeepFilterNet实例
     * 
     * @param tar_buf 模型文件字节数组指针（tar.gz格式）
     * @param tar_size 模型文件字节数组大小
     * @param post_filter_beta 后滤波器beta参数
     * @param atten_lim_db 衰减限制（dB）
     * @return DeepFilterNet状态指针（nullptr表示失败）
     */
    void* df_create(
        const uint8_t* tar_buf,
        size_t tar_size,
        float post_filter_beta,
        float atten_lim_db);

    /**
     * 销毁DeepFilterNet实例
     * 
     * @param state DeepFilterNet状态指针
     */
    void df_destroy(void* state);

    /**
     * 处理音频帧
     * 
     * @param state DeepFilterNet状态指针
     * @param input 输入音频数据指针（f32格式）
     * @param output 输出音频数据指针（f32格式）
     * @param frame_size 帧大小（采样点数）
     * @return LSNR值（负数表示失败）
     */
    float df_process_frame(
        void* state,
        const float* input,
        float* output,
        size_t frame_size);

    /**
     * 设置后滤波器beta参数
     * 
     * @param state DeepFilterNet状态指针
     * @param beta beta参数值
     */
    void df_set_post_filter_beta(void* state, float beta);

    /**
     * 设置衰减限制
     * 
     * @param state DeepFilterNet状态指针
     * @param lim_db 衰减限制（dB）
     */
    void df_set_atten_lim(void* state, float lim_db);

    /**
     * 获取帧大小
     * 
     * @param state DeepFilterNet状态指针
     * @return 帧大小（采样点数）
     */
    size_t df_get_frame_size(void* state);
}

AudioProcessor::AudioProcessor()
    : dfState_(nullptr)
    , dfInitialized_(false)
    , frameSize_(512)
    , aaudioStream_(nullptr)
    , aaudioInitialized_(false)
    , processingThread_(nullptr)
    , processingThreadRunning_(false)
    , callback_(nullptr)
    , isProcessing_(false) {
    memset(lastError_, 0, sizeof(lastError_));
}

AudioProcessor::~AudioProcessor() {
    release();
}

bool AudioProcessor::initialize(
    const uint8_t* tarBytes,
    size_t tarBytesSize,
    float postFilterBeta,
    float attenLimDb) {
    
    if (tarBytes == nullptr || tarBytesSize == 0) {
        snprintf(lastError_, sizeof(lastError_), "模型文件字节数组为空");
        LOGE("%s", lastError_);
        return false;
    }

    if (dfState_ != nullptr) {
        release();
    }

    LOGI("初始化音频处理器: postFilterBeta=%.2f, attenLimDb=%.2f", postFilterBeta, attenLimDb);

    dfState_ = df_create(tarBytes, tarBytesSize, postFilterBeta, attenLimDb);
    
    if (dfState_ == nullptr) {
        snprintf(lastError_, sizeof(lastError_), "创建DeepFilterNet实例失败");
        LOGE("%s", lastError_);
        return false;
    }

    frameSize_ = df_get_frame_size(dfState_);
    dfInitialized_ = true;

    LOGI("DeepFilterNet初始化成功: 帧大小=%zu", frameSize_);

    if (!initAAudioStream()) {
        snprintf(lastError_, sizeof(lastError_), "初始化AAudio流失败");
        LOGE("%s", lastError_);
        release();
        return false;
    }

    LOGI("音频处理器初始化成功: 采样率=%d, 声道数=%d, 帧大小=%zu",
         SAMPLE_RATE, CHANNEL_COUNT, frameSize_);
    
    return true;
}

bool AudioProcessor::initAAudioStream() {
    AAudioStreamBuilder* builder;

    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        snprintf(lastError_, sizeof(lastError_), "创建AAudio流构建器失败: %s", 
                 AAudio_convertResultToText(result));
        LOGE("%s", lastError_);
        return false;
    }

    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setSampleRate(builder, SAMPLE_RATE);
    AAudioStreamBuilder_setChannelCount(builder, CHANNEL_COUNT);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setFramesPerDataCallback(builder, static_cast<int32_t>(frameSize_));
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, this);

    result = AAudioStreamBuilder_openStream(builder, &aaudioStream_);
    AAudioStreamBuilder_delete(builder);

    if (result != AAUDIO_OK) {
        snprintf(lastError_, sizeof(lastError_), "打开AAudio流失败: %s", 
                 AAudio_convertResultToText(result));
        LOGE("%s", lastError_);
        aaudioStream_ = nullptr;
        return false;
    }

    aaudioInitialized_ = true;
    LOGI("AAudio流初始化成功");
    return true;
}

void AudioProcessor::closeAAudioStream() {
    if (aaudioStream_ != nullptr) {
        if (isProcessing_) {
            AAudioStream_requestStop(aaudioStream_);
        }
        AAudioStream_close(aaudioStream_);
        aaudioStream_ = nullptr;
        aaudioInitialized_ = false;
        LOGI("AAudio流已关闭");
    }
}

bool AudioProcessor::start(AudioCallback callback) {
    if (!dfInitialized_ || !aaudioInitialized_) {
        snprintf(lastError_, sizeof(lastError_), "音频处理器未初始化");
        LOGE("%s", lastError_);
        return false;
    }

    if (callback == nullptr) {
        snprintf(lastError_, sizeof(lastError_), "回调函数为空");
        LOGE("%s", lastError_);
        return false;
    }

    callback_ = callback;

    // 启动异步处理线程
    processingThreadRunning_ = true;
    processingThread_ = new std::thread(&AudioProcessor::processingThreadFunc, this);

    aaudio_result_t result = AAudioStream_requestStart(aaudioStream_);
    if (result != AAUDIO_OK) {
        snprintf(lastError_, sizeof(lastError_), "启动AAudio流失败: %s", 
                 AAudio_convertResultToText(result));
        LOGE("%s", lastError_);
        stopProcessingThread();
        return false;
    }

    isProcessing_ = true;
    LOGI("音频录制和降噪处理已启动（异步模式）");
    return true;
}

bool AudioProcessor::stop() {
    if (!isProcessing_) {
        return true;
    }

    if (aaudioStream_ != nullptr) {
        aaudio_result_t result = AAudioStream_requestStop(aaudioStream_);
        if (result != AAUDIO_OK) {
            snprintf(lastError_, sizeof(lastError_), "停止AAudio流失败: %s", 
                     AAudio_convertResultToText(result));
            LOGE("%s", lastError_);
        }
    }

    stopProcessingThread();

    isProcessing_ = false;
    LOGI("音频录制和降噪处理已停止");
    return true;
}

void AudioProcessor::stopProcessingThread() {
    if (processingThread_ != nullptr) {
        processingThreadRunning_ = false;
        queueCondition_.notify_all();
        
        if (processingThread_->joinable()) {
            processingThread_->join();
        }
        
        delete processingThread_;
        processingThread_ = nullptr;
        
        // 清空队列
        std::lock_guard<std::mutex> lock(queueMutex_);
        while (!audioQueue_.empty()) {
            AudioFrame* frame = audioQueue_.front();
            audioQueue_.pop();
            freeAudioFrame(frame);
        }
        
        LOGI("处理线程已停止，队列已清空");
    }
}

bool AudioProcessor::isProcessing() const {
    return isProcessing_;
}

void AudioProcessor::release() {
    stop();
    closeAAudioStream();

    if (dfState_ != nullptr) {
        df_destroy(dfState_);
        dfState_ = nullptr;
        dfInitialized_ = false;
        LOGI("DeepFilterNet资源已释放");
    }

    callback_ = nullptr;
}

bool AudioProcessor::isInitialized() const {
    return dfInitialized_ && aaudioInitialized_;
}

const char* AudioProcessor::getLastError() const {
    return lastError_;
}

bool AudioProcessor::setPostFilterBeta(float beta) {
    if (!dfInitialized_ || dfState_ == nullptr) {
        snprintf(lastError_, sizeof(lastError_), "音频处理器未初始化");
        LOGE("%s", lastError_);
        return false;
    }

    if (beta < 0.0f) {
        snprintf(lastError_, sizeof(lastError_), "beta参数值无效: %.2f", beta);
        LOGE("%s", lastError_);
        return false;
    }

    df_set_post_filter_beta(dfState_, beta);
    LOGI("设置后滤波器beta参数: %.2f", beta);
    return true;
}

bool AudioProcessor::setAttenLimDb(float attenLimDb) {
    if (!dfInitialized_ || dfState_ == nullptr) {
        snprintf(lastError_, sizeof(lastError_), "音频处理器未初始化");
        LOGE("%s", lastError_);
        return false;
    }

    if (attenLimDb < 0.0f) {
        snprintf(lastError_, sizeof(lastError_), "衰减限制值无效: %.2f", attenLimDb);
        LOGE("%s", lastError_);
        return false;
    }

    df_set_atten_lim(dfState_, attenLimDb);
    LOGI("设置衰减限制: %.2f dB", attenLimDb);
    return true;
}

int32_t AudioProcessor::getSampleRate() const {
    return SAMPLE_RATE;
}

int32_t AudioProcessor::getChannelCount() const {
    return CHANNEL_COUNT;
}

int32_t AudioProcessor::getFrameSize() const {
    return static_cast<int32_t>(frameSize_);
}

size_t AudioProcessor::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return audioQueue_.size();
}

aaudio_data_callback_result_t AudioProcessor::dataCallback(
    AAudioStream* stream,
    void* userData,
    void* audioData,
    int32_t numFrames) {
    
    AudioProcessor* processor = static_cast<AudioProcessor*>(userData);
    
    // 快速复制音频数据到队列，避免阻塞音频采集线程
    AudioFrame* frame = processor->allocateAudioFrame(numFrames);
    if (frame != nullptr) {
        memcpy(frame->data, audioData, numFrames * sizeof(float));
        frame->numFrames = numFrames;
        frame->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
        // 将帧放入队列
        {
            std::lock_guard<std::mutex> lock(processor->queueMutex_);
            
            // 检查队列是否已满
            if (processor->audioQueue_.size() >= MAX_QUEUE_SIZE) {
                LOGW("音频队列已满，丢弃最旧的帧");
                AudioFrame* oldFrame = processor->audioQueue_.front();
                processor->audioQueue_.pop();
                processor->freeAudioFrame(oldFrame);
            }
            
            processor->audioQueue_.push(frame);
        }
        
        // 通知处理线程有新数据
        processor->queueCondition_.notify_one();
    }
    
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AudioProcessor::errorCallback(
    AAudioStream* stream,
    void* userData,
    aaudio_result_t error) {
    
    AudioProcessor* processor = static_cast<AudioProcessor*>(userData);
    
    snprintf(processor->lastError_, sizeof(processor->lastError_), 
             "AAudio错误回调: %s", AAudio_convertResultToText(error));
    LOGE("%s", processor->lastError_);

    if (error == AAUDIO_ERROR_DISCONNECTED) {
        LOGW("AAudio流断开连接，尝试恢复...");
        processor->stop();
    }
}

void AudioProcessor::processingThreadFunc() {
    LOGI("异步处理线程已启动");
    
    while (processingThreadRunning_) {
        AudioFrame* frame = nullptr;
        
        // 从队列中获取音频帧
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // 等待队列中有数据或线程停止
            queueCondition_.wait(lock, [this]() {
                return !audioQueue_.empty() || !processingThreadRunning_;
            });
            
            if (!processingThreadRunning_) {
                break;
            }
            
            if (!audioQueue_.empty()) {
                frame = audioQueue_.front();
                audioQueue_.pop();
            }
        }
        
        // 处理音频帧（降噪）
        if (frame != nullptr && dfState_ != nullptr) {
            float* outputBuffer = new float[frame->numFrames];
            if (outputBuffer != nullptr) {
                float lsnr = df_process_frame(dfState_, frame->data, outputBuffer, 
                                              static_cast<size_t>(frame->numFrames));
                
                if (lsnr >= 0.0f && callback_ != nullptr) {
                    // 调用回调函数，将降噪后的音频数据返回给Java层
                    callback_(outputBuffer, frame->numFrames, lsnr);
                } else if (lsnr < 0.0f) {
                    LOGE("音频处理失败: LSNR=%.2f", lsnr);
                }
                
                delete[] outputBuffer;
            }
            
            freeAudioFrame(frame);
        }
    }
    
    LOGI("异步处理线程已停止");
}

AudioFrame* AudioProcessor::allocateAudioFrame(int32_t numFrames) {
    AudioFrame* frame = new AudioFrame();
    if (frame != nullptr) {
        frame->data = new float[numFrames];
        frame->numFrames = numFrames;
        frame->timestamp = 0;
    }
    return frame;
}

void AudioProcessor::freeAudioFrame(AudioFrame* frame) {
    if (frame != nullptr) {
        if (frame->data != nullptr) {
            delete[] frame->data;
        }
        delete frame;
    }
}

} // namespace deepfilter