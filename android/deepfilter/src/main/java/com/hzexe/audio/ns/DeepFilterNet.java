package com.hzexe.audio.ns;

import android.content.Context;
import android.util.Log;
import java.io.IOException;
import java.io.InputStream;

/**
 * DeepFilterNet 降噪引擎 JNI 接口（基于 Tract 框架）
 * 
 * 功能说明：
 * 1. 加载 DeepFilterNet 模型
 * 2. 提供音频降噪处理接口
 * 3. 支持实时音频流处理
 * 4. 使用 Tract 框架进行模型推理
 * 5. 支持纯降噪模式，保守降噪强度
 * 
 * @author hzexe (https://github.com/hzexe)
 * @version 2.0
 */
public class DeepFilterNet {
    
    private static final String TAG = "DeepFilterNet";
    
    // JNI 库名称
    private static final String LIB_NAME = "deepfilter_ort";
    
    // 模型文件名（tar.gz 压缩包）
    private static final String MODEL_ARCHIVE = "DeepFilterNet3_ll_onnx.tgz";
    
    // 模型文件字节数组（tar.gz 压缩包）
    private byte[] modelBytes;
    
    // 原生句柄
    private long nativeHandle;
    
    // 是否已初始化
    private boolean initialized = false;
    
    // 静态初始化块：加载 JNI 库
    static {
        try {
            System.loadLibrary(LIB_NAME);
            Log.d(TAG, "JNI 库 " + LIB_NAME + " 加载成功");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "加载 JNI 库失败: " + e.getMessage());
        }
    }
    
    /**
     * 构造函数
     * 
     * @param modelBytes 模型文件字节数组（tar.gz 压缩包）
     */
    public DeepFilterNet(byte[] modelBytes) {
        this.modelBytes = modelBytes;
    }
    
    
    /**
     * 初始化 DeepFilterNet 引擎
     * 
     * @param postFilterBeta 后滤波器 beta 参数（控制降噪强度）
     * @param attenLimDb 衰减限制（dB）
     * @return true-初始化成功，false-初始化失败
     */
    public boolean initialize(float postFilterBeta, float attenLimDb) {
        if (modelBytes == null) {
            Log.e(TAG, "模型文件未加载，无法初始化");
            return false;
        }
        
        try {
            nativeHandle = nativeCreate(modelBytes, postFilterBeta, attenLimDb);
            if (nativeHandle != 0) {
                initialized = true;
                Log.d(TAG, "DeepFilterNet 初始化成功，句柄: " + nativeHandle);
                return true;
            } else {
                Log.e(TAG, "DeepFilterNet 初始化失败，返回句柄为 0");
                return false;
            }
        } catch (Exception e) {
            Log.e(TAG, "初始化异常: " + e.getMessage());
            return false;
        }
    }
    
     
    /**
     * 处理一帧音频数据（DirectByteBuffer 格式）
     * 
     * @param inputBuffer 输入音频数据缓冲区（DirectByteBuffer，f32，小端序，单声道）
     * @param inputOffset 输入数据在缓冲区中的偏移量（字节）
     * @param inputLength 输入数据长度（字节）
     * @param outputBuffer 输出音频数据缓冲区（DirectByteBuffer，f32，小端序，单声道）
     * @param outputOffset 输出数据在缓冲区中的偏移量（字节）
     * @param outputLength 输出数据长度（字节）
     * @return LSNR 值（负数表示失败）
     */
    public float process(ByteBuffer inputBuffer, int inputOffset, int inputLength,
                        ByteBuffer outputBuffer, int outputOffset, int outputLength) {
        if (!initialized) {
            Log.e(TAG, "引擎未初始化，无法处理音频");
            return -1.0f;
        }
        
        if (inputBuffer == null || outputBuffer == null) {
            Log.e(TAG, "输入或输出缓冲区为空");
            return -1.0f;
        }
        
        return nativeProcess(nativeHandle, inputBuffer, inputOffset, inputLength,
                           outputBuffer, outputOffset, outputLength);
    }
    
    /**
     * 释放资源
     */
    public void release() {
        if (initialized && nativeHandle != 0) {
            nativeDestroy(nativeHandle);
            nativeHandle = 0;
            initialized = false;
            Log.d(TAG, "DeepFilterNet 资源已释放");
        }
    }
    
    /**
     * 检查是否已初始化
     * 
     * @return true-已初始化，false-未初始化
     */
    public boolean isInitialized() {
        return initialized;
    }
    
    // ===== JNI 原生方法声明 =====
    
    /**
     * 创建 DeepFilterNet 实例
     * 
     * @param modelBytes 模型压缩包字节数组（tar.gz）
     * @param postFilterBeta 后滤波器 beta 参数（控制降噪强度）
     * @param attenLimDb 衰减限制（dB）
     * @return 原生句柄（0 表示失败）
     */
    private native long nativeCreate(byte[] modelBytes, float postFilterBeta, float attenLimDb);
    
    /**
     * 处理一帧音频数据（DirectByteBuffer 版本）
     * 
     * @param handle 原生句柄
     * @param inputBuffer 输入缓冲区（DirectByteBuffer，f32，小端序）
     * @param inputOffset 输入缓冲区起始偏移量（字节）
     * @param inputLength 输入缓冲区有效长度（字节）
     * @param outputBuffer 输出缓冲区（DirectByteBuffer，f32，小端序）
     * @param outputOffset 输出缓冲区起始偏移量（字节）
     * @param outputLength 输出缓冲区有效长度（字节）
     * @return LSNR 值（负数表示失败）
     */
    private native float nativeProcess(long handle, ByteBuffer inputBuffer, int inputOffset, int inputLength,
                                       ByteBuffer outputBuffer, int outputOffset, int outputLength);
    
    /**
     * 销毁 DeepFilterNet 实例
     * 
     * @param handle 原生句柄
     */
    private native void nativeDestroy(long handle);
}
