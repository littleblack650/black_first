#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
W25Q64批量烧录工具 - 命令行版本
用法: python w25q64_burn.py <串口> <address.txt> <picture.txt>
"""

import sys
import serial
import time
import re
from struct import pack

# 协议定义
CMD_ERASE_SECTOR = 0x01
CMD_WRITE_PAGE = 0x02
CMD_VERIFY_DATA = 0x04
CMD_GET_INFO = 0x05

RESP_OK = 0x00
RESP_ERROR = 0x01
RESP_CHECKSUM_ERROR = 0x07

W25Q64_SECTOR_SIZE = 4096
W25Q64_PAGE_SIZE = 256


def calculate_checksum(data):
    """计算校验和（XOR）"""
    checksum = 0
    for b in data:
        checksum ^= b
    return checksum


def parse_address_file(filename):
    """解析地址文件"""
    addresses = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('//'):
                # 支持十六进制地址
                if line.startswith('0x') or line.startswith('0X'):
                    addr = int(line, 16)
                else:
                    addr = int(line, 16)
                addresses.append(addr)
    return addresses


def parse_picture_file(filename):
    """解析图片文件"""
    images = []
    current_image_data = []
    current_image_name = ""
    
    with open(filename, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            
            # 检查是否是图片开始标记（注释行）
            if line.startswith('//'):
                # 如果之前有图片数据，保存它
                if current_image_data:
                    image_bytes = bytes(current_image_data)
                    images.append((current_image_name, image_bytes))
                    current_image_data = []
                
                # 提取图片名称
                match = re.search(r"'([^']+)'", line)
                if match:
                    current_image_name = match.group(1)
                else:
                    current_image_name = f"image_{len(images)}"
                continue
            
            # 解析数据行
            if line:
                # 移除行尾逗号
                line = line.rstrip(',')
                # 提取所有0x开头的十六进制数
                hex_values = re.findall(r'0x[0-9A-Fa-f]{2}', line)
                for hex_val in hex_values:
                    current_image_data.append(int(hex_val, 16))
        
        # 保存最后一个图片
        if current_image_data:
            image_bytes = bytes(current_image_data)
            images.append((current_image_name, image_bytes))
    
    return images


def send_command(ser, cmd, address, length, data=None):
    """发送命令包"""
    # 构建命令包
    cmd_packet = pack('<B', cmd)  # cmd (1 byte)
    cmd_packet += pack('<I', address)  # address (4 bytes, little-endian)
    cmd_packet += pack('<H', length)  # length (2 bytes, little-endian)
    
    # 计算校验和（不包括校验和字段本身）
    checksum = calculate_checksum(cmd_packet)
    cmd_packet += pack('<B', checksum)  # checksum (1 byte)
    
    # 发送命令包
    ser.write(cmd_packet)
    
    # 如果有数据，发送数据
    if data and length > 0:
        ser.write(data)
    
    # 读取响应
    response = ser.read(4)  # status(1) + length(2) + checksum(1)
    if len(response) < 4:
        return None, None
    
    status = response[0]
    resp_length = response[1] | (response[2] << 8)
    resp_checksum = response[3]
    
    # 验证响应校验和
    resp_header = response[:3]
    calculated_checksum = calculate_checksum(resp_header)
    if calculated_checksum != resp_checksum:
        print(f"警告: 响应校验和错误")
        return status, None
    
    # 读取响应数据（如果有）
    resp_data = None
    if resp_length > 0:
        resp_data = ser.read(resp_length)
    
    return status, resp_data


def erase_sector(ser, address):
    """擦除扇区"""
    sector_addr = address & ~(W25Q64_SECTOR_SIZE - 1)
    status, _ = send_command(ser, CMD_ERASE_SECTOR, sector_addr, 0)
    return status == RESP_OK


def write_page(ser, address, data):
    """写入一页数据（最多256字节）"""
    if len(data) > W25Q64_PAGE_SIZE:
        data = data[:W25Q64_PAGE_SIZE]
    
    status, _ = send_command(ser, CMD_WRITE_PAGE, address, len(data), data)
    return status == RESP_OK


def write_data(ser, address, data):
    """写入数据（自动分页）"""
    offset = 0
    current_addr = address
    
    while offset < len(data):
        # 计算当前页剩余空间
        page_offset = current_addr % W25Q64_PAGE_SIZE
        page_remaining = W25Q64_PAGE_SIZE - page_offset
        
        # 确定本次写入的数据长度
        write_len = min(page_remaining, len(data) - offset)
        chunk = data[offset:offset + write_len]
        
        # 写入数据
        if not write_page(ser, current_addr, chunk):
            return False
        
        offset += write_len
        current_addr += write_len
        time.sleep(0.01)  # 短暂延迟
    
    return True


def verify_data(ser, address, data):
    """验证数据"""
    status, read_data = send_command(ser, CMD_VERIFY_DATA, address, len(data), data)
    return status == RESP_OK


def burn_images(ser, addresses, images):
    """批量烧录图片"""
    if len(addresses) != len(images):
        print(f"错误: 地址数量({len(addresses)})与图片数量({len(images)})不匹配")
        return False
    
    # 获取需要擦除的唯一扇区
    sectors_to_erase = set()
    for addr in addresses:
        sector_addr = addr & ~(W25Q64_SECTOR_SIZE - 1)
        sectors_to_erase.add(sector_addr)
    
    # 擦除扇区
    print(f"正在擦除 {len(sectors_to_erase)} 个扇区...")
    for i, sector in enumerate(sorted(sectors_to_erase)):
        print(f"  擦除扇区 {i+1}/{len(sectors_to_erase)}: 0x{sector:06X}")
        if not erase_sector(ser, sector):
            print(f"错误: 擦除扇区失败: 0x{sector:06X}")
            return False
        time.sleep(0.1)
    
    # 烧录图片
    print(f"\n开始烧录 {len(images)} 张图片...")
    for i, (address, (name, data)) in enumerate(zip(addresses, images)):
        print(f"[{i+1}/{len(images)}] {name}")
        print(f"  地址: 0x{address:06X}, 大小: {len(data)} 字节")
        
        # 写入数据
        if not write_data(ser, address, data):
            print(f"错误: 写入图片失败: {name}")
            return False
        
        # 验证数据
        if not verify_data(ser, address, data):
            print(f"错误: 验证图片失败: {name}")
            return False
        
        print(f"  ✓ 烧录成功")
        time.sleep(0.05)
    
    print(f"\n所有图片烧录完成!")
    return True


def main():
    if len(sys.argv) < 4:
        print("用法: python w25q64_burn.py <串口> <address.txt> <picture.txt> [波特率]")
        print("示例: python w25q64_burn.py COM3 address.txt picture.txt 115200")
        sys.exit(1)
    
    port = sys.argv[1]
    address_file = sys.argv[2]
    picture_file = sys.argv[3]
    baudrate = int(sys.argv[4]) if len(sys.argv) > 4 else 115200
    
    # 解析文件
    print("解析地址文件...")
    addresses = parse_address_file(address_file)
    print(f"找到 {len(addresses)} 个地址")
    
    print("解析图片文件...")
    images = parse_picture_file(picture_file)
    print(f"找到 {len(images)} 张图片")
    
    if len(addresses) != len(images):
        print(f"警告: 地址数量({len(addresses)})与图片数量({len(images)})不匹配")
    
    # 连接串口
    print(f"\n连接串口 {port} (波特率: {baudrate})...")
    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)  # 等待串口稳定
        
        # 清空接收缓冲区
        ser.reset_input_buffer()
        
        # 烧录图片
        success = burn_images(ser, addresses, images)
        
        ser.close()
        
        if success:
            print("\n✓ 烧录完成!")
            sys.exit(0)
        else:
            print("\n✗ 烧录失败!")
            sys.exit(1)
            
    except serial.SerialException as e:
        print(f"错误: 无法打开串口: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n\n用户中断")
        if ser:
            ser.close()
        sys.exit(1)
    except Exception as e:
        print(f"错误: {e}")
        import traceback
        traceback.print_exc()
        if ser:
            ser.close()
        sys.exit(1)


if __name__ == "__main__":
    main()

