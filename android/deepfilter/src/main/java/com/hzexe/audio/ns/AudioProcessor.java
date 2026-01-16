package com.hzexe.audio.ns;

import android.util.Log;

/**
 * 音频处理器类
 * 
 * 功能说明：
 * 1. 集成AAudio音频录制和deepfilter-ort降噪处理
 * 2. 实现录制后立即降噪的完整流程
 * 3. 支持配置降噪参数（tar_bytes、post_filter_beta、atten_lim_db）
 * 4. 提供实时音频降噪处理接口
 * 5. 音频格式固定：48kHz、单声道、PCM_FLOAT
 * 
 * @author hzexe
 * @version 2.0
 */
public class AudioProcessor {
    
    private static final String TAG = "AudioProcessor";
    
    // 原生句柄
    private long nativeHandle;
    
    // 是否已初始化
    private boolean initialized = false;
    
    // 音频数据回调接口
    private AudioDataCallback callback;
    
    // 静态初始化块：加载JNI库
    static {
        try {
            System.loadLibrary("deepfilter_native");
            Log.d(TAG, "JNI库 deepfilter_native 加载成功");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "加载JNI库失败: " + e.getMessage());
        }
    }
    
    /**
     * 降噪音频数据回调接口
     */
    public interface AudioDataCallback {
        /**
         * 降噪音频数据回调
         * 
         * @param audioData 降噪后的音频数据（f32格式，单声道）
         * @param numFrames 帧数
         * @param lsnr LSNR值（信噪比）
         */
        void onAudioData(float[] audioData, float numFrames, float lsnr);
    }
    
    /**
     * 构造函数
     */
    public AudioProcessor() {
        nativeHandle = nativeCreate();
        if (nativeHandle == 0) {
            Log.e(TAG, "创建AudioProcessor失败");
        }
    }
    
    /**
     * 初始化音频处理器
     * 
     * @param tarBytes 模型文件字节数组（tar.gz格式）
     * @param postFilterBeta 后滤波器beta参数（控制降噪强度）
     * @param attenLimDb 衰减限制（dB）
     * @return true-初始化成功，false-初始化失败
     */
    public boolean initialize(byte[] tarBytes, float postFilterBeta, float attenLimDb) {
        if (nativeHandle == 0) {
            Log.e(TAG, "原生句柄为空，无法初始化");
            return false;
        }
        
        if (tarBytes == null || tarBytes.length == 0) {
            Log.e(TAG, "模型文件字节数组为空");
            return false;
        }
        
        boolean success = nativeInitialize(nativeHandle, tarBytes, postFilterBeta, attenLimDb);
        
        if (success) {
            initialized = true;
            Log.d(TAG, String.format("AudioProcessor初始化成功: postFilterBeta=%.2f, attenLimDb=%.2f, 采样率=%d, 声道数=%d",
                    postFilterBeta, attenLimDb, getSampleRate(), getChannelCount()));
        } else {
            String error = nativeGetLastError(nativeHandle);
            Log.e(TAG, "AudioProcessor初始化失败: " + error);
        }
        
        return success;
    }
    
    /**
     * 开始录制和降噪处理
     * 
     * @param callback 降噪音频数据回调函数
     * @return true-开始成功，false-开始失败
     */
    public boolean start(AudioDataCallback callback) {
        if (!initialized) {
            Log.e(TAG, "AudioProcessor未初始化，无法开始处理");
            return false;
        }
        
        if (callback == null) {
            Log.e(TAG, "回调函数为空");
            return false;
        }
        
        this.callback = callback;
        
        boolean success = nativeStart(nativeHandle, this);
        
        if (success) {
            Log.d(TAG, "AudioProcessor开始录制和降噪处理");
        } else {
            String error = nativeGetLastError(nativeHandle);
            Log.e(TAG, "AudioProcessor开始处理失败: " + error);
        }
        
        return success;
    }
    
    /**
     * 停止录制和降噪处理
     * 
     * @return true-停止成功，false-停止失败
     */
    public boolean stop() {
        if (!initialized) {
            Log.w(TAG, "AudioProcessor未初始化");
            return true;
        }
        
        boolean success = nativeStop(nativeHandle);
        
        if (success) {
            Log.d(TAG, "AudioProcessor停止录制和降噪处理");
        } else {
            String error = nativeGetLastError(nativeHandle);
            Log.e(TAG, "AudioProcessor停止处理失败: " + error);
        }
        
        return success;
    }
    
    /**
     * 检查是否正在处理
     * 
     * @return true-正在处理，false-未处理
     */
    public boolean isProcessing() {
        if (nativeHandle == 0) {
            return false;
        }
        return nativeIsProcessing(nativeHandle);
    }
    
    /**
     * 释放资源
     */
    public void release() {
        if (nativeHandle != 0) {
            stop();
            nativeRelease(nativeHandle);
            initialized = false;
            callback = null;
            Log.d(TAG, "AudioProcessor资源已释放");
        }
    }
    
    /**
     * 检查是否已初始化
     * 
     * @return true-已初始化，false-未初始化
     */
    public boolean isInitialized() {
        if (nativeHandle == 0) {
            return false;
        }
        return nativeIsInitialized(nativeHandle);
    }
    
    /**
     * 获取最后的错误信息
     * 
     * @return 错误信息字符串
     */
    public String getLastError() {
        if (nativeHandle == 0) {
            return "原生句柄为空";
        }
        return nativeGetLastError(nativeHandle);
    }
    
    /**
     * 设置后滤波器beta参数
     * 
     * @param beta beta参数值
     * @return true-设置成功，false-设置失败
     */
    public boolean setPostFilterBeta(float beta) {
        if (!initialized) {
            Log.e(TAG, "AudioProcessor未初始化，无法设置参数");
            return false;
        }
        
        if (beta < 0.0f) {
            Log.e(TAG, "beta参数值无效: " + beta);
            return false;
        }
        
        boolean success = nativeSetPostFilterBeta(nativeHandle, beta);
        
        if (success) {
            Log.d(TAG, "设置后滤波器beta参数: " + beta);
        } else {
            Log.e(TAG, "设置后滤波器beta参数失败: " + nativeGetLastError(nativeHandle));
        }
        
        return success;
    }
    
    /**
     * 设置衰减限制
     * 
     * @param attenLimDb 衰减限制（dB）
     * @return true-设置成功，false-设置失败
     */
    public boolean setAttenLimDb(float attenLimDb) {
        if (!initialized) {
            Log.e(TAG, "AudioProcessor未初始化，无法设置参数");
            return false;
        }
        
        if (attenLimDb < 0.0f) {
            Log.e(TAG, "衰减限制值无效: " + attenLimDb);
            return false;
        }
        
        boolean success = nativeSetAttenLimDb(nativeHandle, attenLimDb);
        
        if (success) {
            Log.d(TAG, "设置衰减限制: " + attenLimDb + " dB");
        } else {
            Log.e(TAG, "设置衰减限制失败: " + nativeGetLastError(nativeHandle));
        }
        
        return success;
    }
    
    /**
     * 获取当前采样率
     * 
     * @return 采样率（Hz），固定为48000
     */
    public int getSampleRate() {
        if (nativeHandle == 0) {
            return 0;
        }
        return nativeGetSampleRate(nativeHandle);
    }
    
    /**
     * 获取当前声道数
     * 
     * @return 声道数，固定为1
     */
    public int getChannelCount() {
        if (nativeHandle == 0) {
            return 0;
        }
        return nativeGetChannelCount(nativeHandle);
    }
    
    /**
     * 获取当前帧大小
     * 
     * @return 帧大小（采样点数）
     */
    public int getFrameSize() {
        if (nativeHandle == 0) {
            return 0;
        }
        return nativeGetFrameSize(nativeHandle);
    }
    
    /**
     * 获取队列大小
     * 
     * @return 当前队列中的帧数
     */
    public int getQueueSize() {
        if (nativeHandle == 0) {
            return 0;
        }
        return nativeGetQueueSize(nativeHandle);
    }
    
    // ===== JNI原生方法声明 =====
    
    /**
     * 创建AudioProcessor实例
     * 
     * @return 原生句柄（0表示失败）
     */
    private native long nativeCreate();
    
    /**
     * 初始化音频处理器
     * 
     * @param nativeHandle 原生句柄
     * @param tarBytes 模型文件字节数组（tar.gz格式）
     * @param postFilterBeta 后滤波器beta参数
     * @param attenLimDb 衰减限制（dB）
     * @return true-初始化成功，false-初始化失败
     */
    private native boolean nativeInitialize(long nativeHandle, byte[] tarBytes, float postFilterBeta, float attenLimDb);
    
    /**
     * 开始录制和降噪处理
     * 
     * @param nativeHandle 原生句柄
     * @param callback 回调对象
     * @return true-开始成功，false-开始失败
     */
    private native boolean nativeStart(long nativeHandle, AudioProcessor callback);
    
    /**
     * 停止录制和降噪处理
     * 
     * @param nativeHandle 原生句柄
     * @return true-停止成功，false-停止失败
     */
    private native boolean nativeStop(long nativeHandle);
    
    /**
     * 检查是否正在处理
     * 
     * @param nativeHandle 原生句柄
     * @return true-正在处理，false-未处理
     */
    private native boolean nativeIsProcessing(long nativeHandle);
    
    /**
     * 释放资源
     * 
     * @param nativeHandle 原生句柄
     */
    private native void nativeRelease(long nativeHandle);
    
    /**
     * 检查是否已初始化
     * 
     * @param nativeHandle 原生句柄
     * @return true-已初始化，false-未初始化
     */
    private native boolean nativeIsInitialized(long nativeHandle);
    
    /**
     * 获取最后的错误信息
     * 
     * @param nativeHandle 原生句柄
     * @return 错误信息字符串
     */
    private native String nativeGetLastError(long nativeHandle);
    
    /**
     * 设置后滤波器beta参数
     * 
     * @param nativeHandle 原生句柄
     * @param beta beta参数值
     * @return true-设置成功，false-设置失败
     */
    private native boolean nativeSetPostFilterBeta(long nativeHandle, float beta);
    
    /**
     * 设置衰减限制
     * 
     * @param nativeHandle 原生句柄
     * @param attenLimDb 衰减限制（dB）
     * @return true-设置成功，false-设置失败
     */
    private native boolean nativeSetAttenLimDb(long nativeHandle, float attenLimDb);
    
    /**
     * 获取当前采样率
     * 
     * @param nativeHandle 原生句柄
     * @return 采样率（Hz）
     */
    private native int nativeGetSampleRate(long nativeHandle);
    
    /**
     * 获取当前声道数
     * 
     * @param nativeHandle 原生句柄
     * @return 声道数
     */
    private native int nativeGetChannelCount(long nativeHandle);
    
    /**
     * 获取当前帧大小
     * 
     * @param nativeHandle 原生句柄
     * @return 帧大小（采样点数）
     */
    private native int nativeGetFrameSize(long nativeHandle);
    
    /**
     * 获取队列大小
     * 
     * @param nativeHandle 原生句柄
     * @return 当前队列中的帧数
     */
    private native int nativeGetQueueSize(long nativeHandle);
    
    /**
     * 销毁AudioProcessor实例
     * 
     * @param nativeHandle 原生句柄
     */
    private native void nativeDestroy(long nativeHandle);
}