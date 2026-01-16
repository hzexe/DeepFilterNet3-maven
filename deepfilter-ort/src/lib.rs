// DeepFilterNet3 集成 - 基于 tract 框架
// 直接封装 tract.rs 的接口，避免重复实现逻辑

use std::io::Cursor;

use jni::JNIEnv;
use jni::objects::{JClass, JByteBuffer};
use jni::sys::{jint, jlong, jbyteArray};
use ndarray::prelude::*;

use df::tract::{DfParams, DfTract, RuntimeParams, ReduceMask};

// DeepFilterNet 状态包装器（线程安全）
pub struct DeepFilterNetState {
    df: DfTract,
}

// 实现线程安全
unsafe impl Send for DeepFilterNetState {}
unsafe impl Sync for DeepFilterNetState {}

// 创建 DeepFilterNet 实例
#[no_mangle]
pub extern "C" fn df_create(
    tar_buf: *const u8,
    tar_size: usize,
    post_filter_beta: f32,
    atten_lim_db: f32,
) -> *mut DeepFilterNetState {
    unsafe {
        if tar_buf.is_null() {
            eprintln!("错误: tar_buf 为空");
            return std::ptr::null_mut();
        }

        let tar_slice = std::slice::from_raw_parts(tar_buf, tar_size);
        let cursor = Cursor::new(tar_slice);

        let df_params = match DfParams::from_targz(cursor) {
            Ok(params) => params,
            Err(e) => {
                eprintln!("加载模型失败: {:?}", e);
                return std::ptr::null_mut();
            }
        };

        let runtime_params = RuntimeParams::new(
            1,                     // n_ch: 音频通道数（1=单声道）
            post_filter_beta,      // post_filter_beta: 后滤波器 beta 参数（控制降噪强度，>0 启用后滤波）
            atten_lim_db,          // atten_lim_db: 衰减限制（dB），控制最大降噪幅度
            -10.,                  // min_db_thresh: 最小 dB 阈值，用于噪声检测
            30.,                   // max_db_erb_thresh: ERB 解码器最大 dB 阈值
            20.,                   // max_db_df_thresh: 深度滤波器最大 dB 阈值
            ReduceMask::MEAN,      // reduce_mask: 掩码缩减方式（MEAN=平均值）
        );

        let df = match DfTract::new(df_params, &runtime_params) {
            Ok(d) => d,
            Err(e) => {
                eprintln!("初始化 DfTract 失败: {:?}", e);
                return std::ptr::null_mut();
            }
        };

        let state = Box::new(DeepFilterNetState { df });
        Box::into_raw(state) as *mut DeepFilterNetState
    }
}

// 销毁 DeepFilterNet 实例
#[no_mangle]
pub extern "C" fn df_destroy(state: *mut DeepFilterNetState) {
    if !state.is_null() {
        unsafe {
            let _ = Box::from_raw(state);
        }
    }
}

// 处理音频帧
#[no_mangle]
pub extern "C" fn df_process_frame(
    state: *mut DeepFilterNetState,
    input: *const f32,
    output: *mut f32,
    frame_size: usize,
) -> f32 {
    unsafe {
        if state.is_null() || input.is_null() || output.is_null() {
            eprintln!("错误: 空指针参数");
            return -1.0;
        }

        let state = &mut *state;
        let input_slice = std::slice::from_raw_parts(input, frame_size);
        let output_slice = std::slice::from_raw_parts_mut(output, frame_size);

        let input_array = ArrayView2::from_shape((1, frame_size), input_slice).unwrap();
        let output_array = ArrayViewMut2::from_shape((1, frame_size), output_slice).unwrap();

        match state.df.process(input_array, output_array) {
            Ok(lsnr) => lsnr,
            Err(e) => {
                eprintln!("处理帧失败: {:?}", e);
                -1.0
            }
        }
    }
}

// 设置后滤波器beta参数
#[no_mangle]
pub extern "C" fn df_set_post_filter_beta(state: *mut DeepFilterNetState, beta: f32) {
    unsafe {
        if !state.is_null() {
            eprintln!("错误: state指针为空");
            return;
        }
        
        let state = &mut *state;
        state.df.set_pf_beta(beta);
    }
}

// 设置衰减限制
#[no_mangle]
pub extern "C" fn df_set_atten_lim(state: *mut DeepFilterNetState, lim_db: f32) {
    unsafe {
        if !state.is_null() {
            eprintln!("错误: state指针为空");
            return;
        }
        
        let state = &mut *state;
        state.df.set_atten_lim(lim_db);
    }
}

// 获取帧大小
#[no_mangle]
pub extern "C" fn df_get_frame_size(state: *mut DeepFilterNetState) -> usize {
    unsafe {
        if state.is_null() {
            eprintln!("错误: state指针为空");
            return 0;
        }
        
        let state = &*state;
        state.df.hop_size
    }
}

// JNI: 创建实例
#[no_mangle]
pub extern "system" fn Java_com_hzexe_audio_ns_DeepFilterNet_nativeCreate(
    env: JNIEnv,
    _class: JClass,
    tar_bytes: jbyteArray,
    post_filter_beta: f32,
    atten_lim_db: f32,
) -> jlong {
    let tar_buf = match env.convert_byte_array(tar_bytes) {
        Ok(buf) => buf,
        Err(e) => {
            eprintln!("转换字节数组失败: {:?}", e);
            return 0;
        }
    };

    let state_ptr = df_create(tar_buf.as_ptr(), tar_buf.len(), post_filter_beta, atten_lim_db);

    if state_ptr.is_null() {
        0
    } else {
        state_ptr as jlong
    }
}

// JNI: 销毁实例
#[no_mangle]
pub extern "system" fn Java_com_hzexe_audio_ns_DeepFilterNet_nativeDestroy(
    _env: JNIEnv,
    _class: JClass,
    state_ptr: jlong,
) {
    if state_ptr != 0 {
        df_destroy(state_ptr as *mut DeepFilterNetState);
    }
}

// JNI: 处理音频帧（优化版本 - 减少内存分配和复制）
// input_offset: 输入数组起始偏移量（字节）
// input_length: 输入数组有效长度（字节）
// output_offset: 输出数组起始偏移量（字节）
#[no_mangle]
pub extern "system" fn Java_com_hzexe_audio_ns_DeepFilterNet_nativeProcess(
    env: JNIEnv,
    _class: JClass,
    state_ptr: jlong,
    input: JByteBuffer,
    input_offset: jint,
    input_length: jint,
    output: JByteBuffer,
    output_offset: jint,
    output_length: jint,
) -> f32 {
    if state_ptr == 0 {
        eprintln!("错误: 状态指针为空");
        return -1.0;
    }

    if input_length != output_length {
        eprintln!("错误: 输入和输出长度不匹配");
        return -1.0;
    }

    if input_length % 4 != 0 {
        eprintln!("错误: 长度必须是 4 的倍数");
        return -1.0;
    }

    let frame_size = (input_length / 4) as usize;

    let input_ptr = match env.get_direct_buffer_address(input) {
        Ok(ptr) => ptr,
        Err(e) => {
            eprintln!("获取输入缓冲区地址失败: {}", e);
            return -1.0;
        }
    };

    let output_ptr = match env.get_direct_buffer_address(output) {
        Ok(ptr) => ptr,
        Err(e) => {
            eprintln!("获取输出缓冲区地址失败: {}", e);
            return -1.0;
        }
    };

    let input_bytes = unsafe {
        std::slice::from_raw_parts(
            input_ptr.offset(input_offset as isize),
            input_length as usize,
        )
    };

    let output_bytes = unsafe {
        std::slice::from_raw_parts_mut(
            output_ptr.offset(output_offset as isize),
            output_length as usize,
        )
    };

    let mut input_f32: [f32; 512] = [0.0; 512];
    let mut output_f32: [f32; 512] = [0.0; 512];

    for i in 0..frame_size {
        let bytes: [u8; 4] = [
            input_bytes[i * 4] as u8,
            input_bytes[i * 4 + 1] as u8,
            input_bytes[i * 4 + 2] as u8,
            input_bytes[i * 4 + 3] as u8,
        ];
        input_f32[i] = f32::from_le_bytes(bytes);
    }

    let lsnr = df_process_frame(
        state_ptr as *mut DeepFilterNetState,
        input_f32.as_ptr(),
        output_f32.as_mut_ptr(),
        frame_size,
    );

    for i in 0..frame_size {
        let bytes = output_f32[i].to_le_bytes();
        output_bytes[i * 4] = bytes[0];
        output_bytes[i * 4 + 1] = bytes[1];
        output_bytes[i * 4 + 2] = bytes[2];
        output_bytes[i * 4 + 3] = bytes[3];
    }

    lsnr
}
