package com.hzexe.audio.ns;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;

/**
 * AudioProcessor测试类
 * 
 * 功能说明：
 * 1. 测试音频录制和降噪功能
 * 2. 验证参数配置有效性
 * 3. 测试实时音频处理性能
 * 
 * @author hzexe
 * @version 1.0
 */
public class AudioProcessorTest {
    
    private static final String TAG = "AudioProcessorTest";
    
    // 模型文件名
    private static final String MODEL_ARCHIVE = "DeepFilterNet3_ll_onnx.tgz";
    
    // 测试参数
    private static final float TEST_POST_FILTER_BETA = 0.5f;
    private static final float TEST_ATTEN_LIM_DB = 30.0f;
    
    // 音频播放器（用于播放降噪后的音频）
    private AudioTrack audioTrack;
    
    // 音频处理器
    private AudioProcessor audioProcessor;
    
    // 测试统计信息
    private int frameCount;
    private float totalLsnr;
    private long startTime;
    private long endTime;
    
    /**
     * 测试初始化
     * 
     * @param context Android上下文
     * @return true-测试成功，false-测试失败
     */
    public boolean testInitialize(Context context) {
        Log.d(TAG, "========== 开始测试初始化 ==========");
        
        try {
            // 加载模型文件
            byte[] modelBytes = loadModelFile(context);
            if (modelBytes == null || modelBytes.length == 0) {
                Log.e(TAG, "模型文件加载失败");
                return false;
            }
            
            Log.d(TAG, "模型文件加载成功，大小: " + modelBytes.length + " 字节");
            
            // 创建音频处理器
            audioProcessor = new AudioProcessor();
            
            // 初始化音频处理器
            boolean success = audioProcessor.initialize(modelBytes, TEST_POST_FILTER_BETA, TEST_ATTEN_LIM_DB);
            if (!success) {
                Log.e(TAG, "AudioProcessor初始化失败: " + audioProcessor.getLastError());
                return false;
            }
            
            // 验证是否已初始化
            if (!audioProcessor.isInitialized()) {
                Log.e(TAG, "AudioProcessor初始化状态检查失败");
                return false;
            }
            
            // 验证音频格式
            int sampleRate = audioProcessor.getSampleRate();
            int channelCount = audioProcessor.getChannelCount();
            int frameSize = audioProcessor.getFrameSize();
            
            Log.d(TAG, String.format("音频格式验证: 采样率=%d Hz, 声道数=%d, 帧大小=%d",
                    sampleRate, channelCount, frameSize));
            
            if (sampleRate != 48000) {
                Log.e(TAG, "采样率不正确，期望: 48000, 实际: " + sampleRate);
                return false;
            }
            
            if (channelCount != 1) {
                Log.e(TAG, "声道数不正确，期望: 1, 实际: " + channelCount);
                return false;
            }
            
            if (frameSize <= 0) {
                Log.e(TAG, "帧大小不正确: " + frameSize);
                return false;
            }
            
            Log.d(TAG, "========== 初始化测试通过 ==========");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "初始化测试异常: " + e.getMessage());
            e.printStackTrace();
            return false;
        }
    }
    
    /**
     * 测试参数配置
     * 
     * @return true-测试成功，false-测试失败
     */
    public boolean testParameterConfiguration() {
        Log.d(TAG, "========== 开始测试参数配置 ==========");
        
        if (audioProcessor == null || !audioProcessor.isInitialized()) {
            Log.e(TAG, "AudioProcessor未初始化");
            return false;
        }
        
        try {
            // 测试设置后滤波器beta参数
            float[] betaValues = {0.1f, 0.5f, 1.0f, 2.0f};
            for (float beta : betaValues) {
                boolean success = audioProcessor.setPostFilterBeta(beta);
                if (!success) {
                    Log.e(TAG, "设置后滤波器beta参数失败: " + beta);
                    return false;
                }
                Log.d(TAG, "设置后滤波器beta参数成功: " + beta);
            }
            
            // 测试设置衰减限制
            float[] attenLimDbValues = {10.0f, 20.0f, 30.0f, 40.0f};
            for (float attenLimDb : attenLimDbValues) {
                boolean success = audioProcessor.setAttenLimDb(attenLimDb);
                if (!success) {
                    Log.e(TAG, "设置衰减限制失败: " + attenLimDb);
                    return false;
                }
                Log.d(TAG, "设置衰减限制成功: " + attenLimDb + " dB");
            }
            
            // 测试无效参数
            boolean invalidBetaResult = audioProcessor.setPostFilterBeta(-1.0f);
            if (invalidBetaResult) {
                Log.e(TAG, "无效的beta参数应该设置失败");
                return false;
            }
            
            boolean invalidAttenLimResult = audioProcessor.setAttenLimDb(-1.0f);
            if (invalidAttenLimResult) {
                Log.e(TAG, "无效的衰减限制应该设置失败");
                return false;
            }
            
            Log.d(TAG, "========== 参数配置测试通过 ==========");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "参数配置测试异常: " + e.getMessage());
            e.printStackTrace();
            return false;
        }
    }
    
    /**
     * 测试音频录制和降噪处理
     * 
     * @param durationMs 测试持续时间（毫秒）
     * @return true-测试成功，false-测试失败
     */
    public boolean testAudioProcessing(int durationMs) {
        Log.d(TAG, "========== 开始测试音频处理，持续时间: " + durationMs + " ms ==========");
        
        if (audioProcessor == null || !audioProcessor.isInitialized()) {
            Log.e(TAG, "AudioProcessor未初始化");
            return false;
        }
        
        try {
            // 初始化音频播放器
            initAudioTrack();
            
            // 重置统计信息
            frameCount = 0;
            totalLsnr = 0.0f;
            startTime = System.currentTimeMillis();
            
            // 创建音频数据回调
            AudioProcessor.AudioDataCallback callback = new AudioProcessor.AudioDataCallback() {
                @Override
                public void onAudioData(float[] audioData, float numFrames, float lsnr) {
                    // 统计信息
                    frameCount++;
                    totalLsnr += lsnr;
                    
                    // 播放降噪后的音频
                    playAudio(audioData);
                    
                    // 每100帧输出一次日志
                    if (frameCount % 100 == 0) {
                        float avgLsnr = totalLsnr / frameCount;
                        Log.d(TAG, String.format("处理进度: %d 帧, 平均LSNR: %.2f", frameCount, avgLsnr));
                    }
                }
            };
            
            // 开始录制和降噪处理
            boolean success = audioProcessor.start(callback);
            if (!success) {
                Log.e(TAG, "开始音频处理失败: " + audioProcessor.getLastError());
                return false;
            }
            
            // 等待指定时间
            Thread.sleep(durationMs);
            
            // 停止录制和降噪处理
            audioProcessor.stop();
            endTime = System.currentTimeMillis();
            
            // 输出测试结果
            float avgLsnr = totalLsnr / frameCount;
            long duration = endTime - startTime;
            float fps = (frameCount * 1000.0f) / duration;
            
            Log.d(TAG, "========== 音频处理测试结果 ==========");
            Log.d(TAG, "总帧数: " + frameCount);
            Log.d(TAG, "持续时间: " + duration + " ms");
            Log.d(TAG, "处理速度: " + fps + " 帧/秒");
            Log.d(TAG, "平均LSNR: " + avgLsnr);
            Log.d(TAG, "========== 测试完成 ==========");
            
            // 释放音频播放器
            releaseAudioTrack();
            
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "音频处理测试异常: " + e.getMessage());
            e.printStackTrace();
            return false;
        }
    }
    
    /**
     * 测试完整流程
     * 
     * @param context Android上下文
     * @param durationMs 测试持续时间（毫秒）
     * @return true-测试成功，false-测试失败
     */
    public boolean testFullProcess(Context context, int durationMs) {
        Log.d(TAG, "========== 开始完整流程测试 ==========");
        
        try {
            // 测试初始化
            if (!testInitialize(context)) {
                Log.e(TAG, "初始化测试失败");
                return false;
            }
            
            // 测试参数配置
            if (!testParameterConfiguration()) {
                Log.e(TAG, "参数配置测试失败");
                return false;
            }
            
            // 测试音频处理
            if (!testAudioProcessing(durationMs)) {
                Log.e(TAG, "音频处理测试失败");
                return false;
            }
            
            Log.d(TAG, "========== 完整流程测试通过 ==========");
            return true;
            
        } catch (Exception e) {
            Log.e(TAG, "完整流程测试异常: " + e.getMessage());
            e.printStackTrace();
            return false;
        }
    }
    
    /**
     * 释放资源
     */
    public void release() {
        Log.d(TAG, "释放测试资源");
        
        if (audioProcessor != null) {
            audioProcessor.release();
            audioProcessor = null;
        }
        
        releaseAudioTrack();
    }
    
    /**
     * 加载模型文件
     * 
     * @param context Android上下文
     * @return 模型文件字节数组
     */
    private byte[] loadModelFile(Context context) {
        try {
            InputStream is = context.getAssets().open(MODEL_ARCHIVE);
            byte[] buffer = new byte[is.available()];
            is.read(buffer);
            is.close();
            return buffer;
        } catch (IOException e) {
            Log.e(TAG, "加载模型文件失败: " + e.getMessage());
            return null;
        }
    }
    
    /**
     * 初始化音频播放器
     */
    private void initAudioTrack() {
        int sampleRate = audioProcessor.getSampleRate();
        int channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        int audioFormat = AudioFormat.ENCODING_PCM_FLOAT;
        int bufferSize = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat) * 2;
        
        audioTrack = new AudioTrack(
                AudioManager.STREAM_MUSIC,
                sampleRate,
                channelConfig,
                audioFormat,
                bufferSize,
                AudioTrack.MODE_STREAM);
        
        audioTrack.play();
        Log.d(TAG, "音频播放器初始化成功");
    }
    
    /**
     * 播放音频数据
     * 
     * @param audioData 音频数据（f32格式）
     */
    private void playAudio(float[] audioData) {
        if (audioTrack != null && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            audioTrack.write(audioData, 0, audioData.length, AudioTrack.WRITE_BLOCKING);
        }
    }
    
    /**
     * 释放音频播放器
     */
    private void releaseAudioTrack() {
        if (audioTrack != null) {
            audioTrack.stop();
            audioTrack.release();
            audioTrack = null;
            Log.d(TAG, "音频播放器已释放");
        }
    }
}