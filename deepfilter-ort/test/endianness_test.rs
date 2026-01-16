/**
 * 字节序测试工具（Rust版本）
 * 
 * 用于验证Rust和C++之间的字节序是否一致
 */

use std::mem;

/**
 * 打印浮点数的字节表示
 * 
 * @param value 浮点数值
 * @param name 名称
 */
fn print_float_bytes(value: f32, name: &str) {
    let bytes: &[u8] = unsafe {
        std::slice::from_raw_parts(&value as *const f32 as *const u8, 4)
    };
    
    println!("{} = {:.6}", name, value);
    print!("  Bytes: ");
    for byte in bytes {
        print!("{:02X} ", byte);
    }
    println!();
    
    // 判断字节序
    if bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x80 && bytes[3] == 0x3F {
        println!("  字节序: Little-Endian (小端序)");
    } else if bytes[0] == 0x3F && bytes[1] == 0x80 && bytes[2] == 0x00 && bytes[3] == 0x00 {
        println!("  字节序: Big-Endian (大端序)");
    } else {
        println!("  字节序: 未知");
    }
    println!();
}

/**
 * 测试IEEE 754浮点数表示
 */
fn test_ieee754() {
    println!("========================================");
    println!("IEEE 754浮点数测试");
    println!("========================================");
    println!();
    
    // 测试1.0f
    print_float_bytes(1.0, "1.0f");
    
    // 测试-1.0f
    print_float_bytes(-1.0, "-1.0f");
    
    // 测试0.0f
    print_float_bytes(0.0, "0.0f");
    
    // 测试3.1415926535f
    print_float_bytes(3.1415927, "3.1415926535f");
    
    // 测试123.456f
    print_float_bytes(123.456, "123.456f");
}

/**
 * 测试音频数据字节序
 */
fn test_audio_data() {
    println!("========================================");
    println!("音频数据字节序测试");
    println!("========================================");
    println!();
    
    // 模拟音频数据（正弦波）
    let num_samples = 10;
    let mut audio_data: Vec<f32> = Vec::with_capacity(num_samples);
    
    println!("生成正弦波音频数据：");
    for i in 0..num_samples {
        let sample = (2.0 * std::f32::consts::PI * i as f32 / num_samples as f32).sin();
        audio_data.push(sample);
    }
    
    println!("前5个采样点的字节表示：");
    for i in 0..5 {
        let bytes: &[u8] = unsafe {
            std::slice::from_raw_parts(&audio_data[i] as *const f32 as *const u8, 4)
        };
        print!("  Sample[{}] = {:.6} -> ", i, audio_data[i]);
        for byte in bytes {
            print!("{:02X} ", byte);
        }
        println!();
    }
    println!();
}

/**
 * 测试字节序一致性
 */
fn test_endianness_consistency() {
    println!("========================================");
    println!("字节序一致性测试");
    println!("========================================");
    println!();
    
    // 测试多个值，确保字节序一致
    let test_values: Vec<f32> = vec![1.0, 2.0, 3.0, 4.0, 5.0];
    
    println!("检查字节序一致性：");
    for value in &test_values {
        let bytes: &[u8] = unsafe {
            std::slice::from_raw_parts(value as *const f32 as *const u8, 4)
        };
        print!("  {}f -> ", value);
        for byte in bytes {
            print!("{:02X} ", byte);
        }
        println!();
    }
    
    println!();
    println!("结论：");
    println!("  如果所有值的字节序都是 Little-Endian，");
    println!("  则Rust和C++之间不需要字节序转换。");
    println!();
}

/**
 * 测试从C++传递的指针
 */
fn test_pointer_from_cpp(input_ptr: *const f32, frame_size: usize) {
    println!("========================================");
    println!("从C++传递的指针测试");
    println!("========================================");
    println!();
    
    if input_ptr.is_null() {
        println!("错误: 输入指针为空");
        return;
    }
    
    unsafe {
        let input_slice = std::slice::from_raw_parts(input_ptr, frame_size);
        
        println!("帧大小: {}", frame_size);
        println!("前5个采样点的字节表示：");
        
        for i in 0..std::cmp::min(5, frame_size) {
            let bytes: &[u8] = std::slice::from_raw_parts(
                &input_slice[i] as *const f32 as *const u8, 
                4
            );
            print!("  Sample[{}] = {:.6} -> ", i, input_slice[i]);
            for byte in bytes {
                print!("{:02X} ", byte);
            }
            println!();
        }
    }
    
    println!();
    println!("结论：");
    println!("  如果字节序与Rust本地生成的数据一致，");
    println!("  则说明C++和Rust之间的字节序兼容。");
    println!();
}

/**
 * 主函数
 */
fn main() {
    println!();
    println!("========================================");
    println!("  字节序测试工具 (Rust版本)");
    println!("========================================");
    println!();
    
    test_ieee754();
    test_audio_data();
    test_endianness_consistency();
    
    // 如果需要测试从C++传递的指针，可以取消下面的注释
    // 并在C++代码中调用此函数
    // let test_data: Vec<f32> = vec![1.0, 2.0, 3.0, 4.0, 5.0];
    // test_pointer_from_cpp(test_data.as_ptr(), test_data.len());
    
    println!("========================================");
    println!("  测试完成");
    println!("========================================");
    println!();
}

/**
 * 导出函数供C++调用测试
 */
#[no_mangle]
pub extern "C" fn test_endianness_from_cpp(input: *const f32, frame_size: usize) {
    test_pointer_from_cpp(input, frame_size);
}
