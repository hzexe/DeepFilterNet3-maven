#include <jni.h>
#include <android/log.h>
#include <cstring>
#include "AudioProcessor.h"

#define LOG_TAG "DeepFilterJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace deepfilter;

extern "C" {

// ===== AudioProcessor JNI接口 =====

JNIEXPORT jlong JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeCreate(
    JNIEnv* env,
    jobject thiz) {
    
    AudioProcessor* processor = new AudioProcessor();
    return reinterpret_cast<jlong>(processor);
}

JNIEXPORT jboolean JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeInitialize(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle,
    jbyteArray tarBytes,
    jfloat postFilterBeta,
    jfloat attenLimDb) {
    
    if (nativeHandle == 0) {
        LOGE("AudioProcessor句柄为空");
        return JNI_FALSE;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    
    if (tarBytes == nullptr) {
        LOGE("模型文件字节数组为空");
        return JNI_FALSE;
    }

    jbyte* tarBytesPtr = env->GetByteArrayElements(tarBytes, nullptr);
    jsize tarBytesSize = env->GetArrayLength(tarBytes);
    
    bool success = processor->initialize(
        reinterpret_cast<uint8_t*>(tarBytesPtr),
        static_cast<size_t>(tarBytesSize),
        postFilterBeta,
        attenLimDb);
    
    env->ReleaseByteArrayElements(tarBytes, tarBytesPtr, JNI_ABORT);
    
    if (!success) {
        LOGE("AudioProcessor初始化失败: %s", processor->getLastError());
    }
    
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeStart(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle,
    jobject callback) {
    
    if (nativeHandle == 0) {
        LOGE("AudioProcessor句柄为空");
        return JNI_FALSE;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    
    if (callback == nullptr) {
        LOGE("回调对象为空");
        return JNI_FALSE;
    }

    jclass callbackClass = env->GetObjectClass(callback);
    jmethodID onAudioDataMethod = env->GetMethodID(callbackClass, "onAudioData", "([FF)V");
    
    if (onAudioDataMethod == nullptr) {
        LOGE("找不到onAudioData方法");
        return JNI_FALSE;
    }

    auto callbackFunc = [env, callback, onAudioDataMethod](const float* audioData, int32_t numFrames, float lsnr) {
        jfloatArray jAudioData = env->NewFloatArray(numFrames);
        if (jAudioData == nullptr) {
            LOGE("创建float数组失败");
            return;
        }
        
        env->SetFloatArrayRegion(jAudioData, 0, numFrames, audioData);
        env->CallVoidMethod(callback, onAudioDataMethod, jAudioData, static_cast<jfloat>(numFrames), static_cast<jfloat>(lsnr));
        env->DeleteLocalRef(jAudioData);
    };

    bool success = processor->start(callbackFunc);
    
    if (!success) {
        LOGE("AudioProcessor启动失败: %s", processor->getLastError());
    }
    
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeStop(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle) {
    
    if (nativeHandle == 0) {
        LOGE("AudioProcessor句柄为空");
        return JNI_FALSE;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    
    bool success = processor->stop();
    
    if (!success) {
        LOGE("AudioProcessor停止失败: %s", processor->getLastError());
    }
    
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeIsProcessing(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle) {
    
    if (nativeHandle == 0) {
        return JNI_FALSE;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    return processor->isProcessing() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeRelease(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle) {
    
    if (nativeHandle != 0) {
        AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
        processor->release();
    }
}

JNIEXPORT jboolean JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeIsInitialized(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle) {
    
    if (nativeHandle == 0) {
        return JNI_FALSE;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    return processor->isInitialized() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeGetLastError(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle) {
    
    if (nativeHandle == 0) {
        return env->NewStringUTF("句柄为空");
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    return env->NewStringUTF(processor->getLastError());
}

JNIEXPORT jboolean JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeSetPostFilterBeta(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle,
    jfloat beta) {
    
    if (nativeHandle == 0) {
        LOGE("AudioProcessor句柄为空");
        return JNI_FALSE;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    
    bool success = processor->setPostFilterBeta(beta);
    
    if (!success) {
        LOGE("设置后滤波器beta参数失败: %s", processor->getLastError());
    }
    
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeSetAttenLimDb(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle,
    jfloat attenLimDb) {
    
    if (nativeHandle == 0) {
        LOGE("AudioProcessor句柄为空");
        return JNI_FALSE;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    
    bool success = processor->setAttenLimDb(attenLimDb);
    
    if (!success) {
        LOGE("设置衰减限制失败: %s", processor->getLastError());
    }
    
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeGetSampleRate(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle) {
    
    if (nativeHandle == 0) {
        return 0;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    return processor->getSampleRate();
}

JNIEXPORT jint JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeGetChannelCount(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle) {
    
    if (nativeHandle == 0) {
        return 0;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    return processor->getChannelCount();
}

JNIEXPORT jint JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeGetFrameSize(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle) {
    
    if (nativeHandle == 0) {
        return 0;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    return processor->getFrameSize();
}

JNIEXPORT jint JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeGetQueueSize(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle) {
    
    if (nativeHandle == 0) {
        return 0;
    }

    AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
    return static_cast<jint>(processor->getQueueSize());
}

JNIEXPORT void JNICALL
Java_com_hzexe_audio_ns_AudioProcessor_nativeDestroy(
    JNIEnv* env,
    jobject thiz,
    jlong nativeHandle) {
    
    if (nativeHandle != 0) {
        AudioProcessor* processor = reinterpret_cast<AudioProcessor*>(nativeHandle);
        delete processor;
        LOGI("AudioProcessor已销毁");
    }
}

} // extern "C"