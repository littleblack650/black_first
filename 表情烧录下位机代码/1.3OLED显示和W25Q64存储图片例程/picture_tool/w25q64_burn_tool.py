#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
W25Q64烧录工具 - 核心模块
"""

import serial
import time
import re
from struct import pack, unpack
from dataclasses import dataclass
from typing import List, Tuple, Optional

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


@dataclass
class FlashInfo:
    """Flash信息"""
    capacity: int
    sector_size: int
    page_size: int
    sector_count: int


@dataclass
class ImageInfo:
    """图片信息"""
    address: int
    data: bytes
    name: str = ""


class W25Q64Burner:
    """W25Q64烧录器"""
    
    def __init__(self, port: str, baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.ser: Optional[serial.Serial] = None
    
    def connect(self) -> bool:
        """连接设备"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=2)
            time.sleep(2)  # 等待串口稳定
            self.ser.reset_input_buffer()
            return True
        except serial.SerialException:
            return False
    
    def disconnect(self):
        """断开连接"""
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.ser = None
    
    def send_command(self, cmd: int, address: int, length: int, data: bytes = None) -> Tuple[int, Optional[bytes]]:
        """发送命令包"""
        # #region agent log
        log_path = r"c:\Users\black\Desktop\Table_pet_robot\picture_load\1.3OLED显示和W25Q64存储图片例程\.cursor\debug.log"
        import json
        try:
            with open(log_path, 'a', encoding='utf-8') as f:
                f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Enter", "data": {"cmd": cmd, "address": hex(address), "length": length, "has_data": data is not None}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "A"}) + "\n")
        except: pass
        # #endregion
        
        if not self.ser or not self.ser.is_open:
            # #region agent log
            try:
                with open(log_path, 'a', encoding='utf-8') as f:
                    f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Serial not open", "data": {}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "A"}) + "\n")
            except: pass
            # #endregion
            return None, None
        
        # 构建命令包
        cmd_packet = pack('<B', cmd)  # cmd (1 byte)
        cmd_packet += pack('<I', address)  # address (4 bytes, little-endian)
        cmd_packet += pack('<H', length)  # length (2 bytes, little-endian)
        
        # 计算校验和
        checksum = calculate_checksum(cmd_packet)
        cmd_packet += pack('<B', checksum)  # checksum (1 byte)
        
        # #region agent log
        try:
            with open(log_path, 'a', encoding='utf-8') as f:
                f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Before send", "data": {"cmd_packet_len": len(cmd_packet), "checksum": hex(checksum), "data_len": len(data) if data else 0}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "B"}) + "\n")
        except: pass
        # #endregion
        
        # 发送命令包
        self.ser.write(cmd_packet)
        
        # 如果有数据，发送数据
        if data and length > 0:
            self.ser.write(data)
            # #region agent log
            try:
                with open(log_path, 'a', encoding='utf-8') as f:
                    f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Data sent", "data": {"data_bytes": len(data)}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "B"}) + "\n")
            except: pass
            # #endregion
            # 根据数据量计算等待时间，确保STM32有足够时间处理
            wait_time = max(0.05, length / 10000.0)  # 至少50ms，或根据数据量计算
            time.sleep(wait_time)
        
        # 读取响应（增加超时和重试）
        # #region agent log
        try:
            with open(log_path, 'a', encoding='utf-8') as f:
                f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Before read response", "data": {"in_waiting": self.ser.in_waiting}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "E"}) + "\n")
        except: pass
        # #endregion
        
        # 等待响应数据到达（最多等待1秒）
        timeout_count = 0
        while self.ser.in_waiting < 4 and timeout_count < 100:
            time.sleep(0.01)
            timeout_count += 1
        
        # #region agent log
        try:
            with open(log_path, 'a', encoding='utf-8') as f:
                f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Wait for response", "data": {"in_waiting": self.ser.in_waiting, "timeout_count": timeout_count}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "E"}) + "\n")
        except: pass
        # #endregion
        
        response = self.ser.read(4)  # status(1) + length(2) + checksum(1)
        
        # #region agent log
        try:
            with open(log_path, 'a', encoding='utf-8') as f:
                f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Response received", "data": {"response_len": len(response), "response_hex": response.hex() if response else "None"}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "E"}) + "\n")
        except: pass
        # #endregion
        
        if len(response) < 4:
            # #region agent log
            try:
                with open(log_path, 'a', encoding='utf-8') as f:
                    f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Incomplete response", "data": {"got": len(response)}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "E"}) + "\n")
            except: pass
            # #endregion
            return None, None
        
        status = response[0]
        resp_length = response[1] | (response[2] << 8)
        resp_checksum = response[3]
        
        # 验证响应校验和
        resp_header = response[:3]
        calculated_checksum = calculate_checksum(resp_header)
        if calculated_checksum != resp_checksum:
            # #region agent log
            try:
                with open(log_path, 'a', encoding='utf-8') as f:
                    f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Checksum error", "data": {"calculated": hex(calculated_checksum), "received": hex(resp_checksum)}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "E"}) + "\n")
            except: pass
            # #endregion
            return RESP_CHECKSUM_ERROR, None
        
        # 读取响应数据（如果有）
        resp_data = None
        if resp_length > 0:
            resp_data = self.ser.read(resp_length)
            # #region agent log
            try:
                with open(log_path, 'a', encoding='utf-8') as f:
                    f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Response data read", "data": {"expected": resp_length, "got": len(resp_data) if resp_data else 0}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "E"}) + "\n")
            except: pass
            # #endregion
        
        # #region agent log
        try:
            with open(log_path, 'a', encoding='utf-8') as f:
                f.write(json.dumps({"timestamp": time.time(), "location": "send_command", "message": "Exit", "data": {"status": status, "status_hex": hex(status)}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "A"}) + "\n")
        except: pass
        # #endregion
        
        return status, resp_data
    
    def erase_sector(self, address: int) -> bool:
        """擦除扇区"""
        sector_addr = address & ~(W25Q64_SECTOR_SIZE - 1)
        status, _ = self.send_command(CMD_ERASE_SECTOR, sector_addr, 0)
        if status == RESP_OK:
            time.sleep(0.1)  # 等待擦除完成
            return True
        return False
    
    def write_page(self, address: int, data: bytes) -> bool:
        """写入一页或数据块（自动分页）"""
        # #region agent log
        log_path = r"c:\Users\black\Desktop\Table_pet_robot\picture_load\1.3OLED显示和W25Q64存储图片例程\.cursor\debug.log"
        import json
        try:
            with open(log_path, 'a', encoding='utf-8') as f:
                f.write(json.dumps({"timestamp": time.time(), "location": "write_page", "message": "Enter", "data": {"address": hex(address), "data_len": len(data)}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "D"}) + "\n")
        except: pass
        # #endregion
        
        offset = 0
        current_addr = address
        data_len = len(data)
        
        while offset < data_len:
            # 计算当前页剩余空间
            page_offset = current_addr % W25Q64_PAGE_SIZE
            page_remaining = W25Q64_PAGE_SIZE - page_offset
            
            # 确定本次写入的数据长度
            write_len = min(page_remaining, data_len - offset)
            chunk = data[offset:offset + write_len]
            
            # #region agent log
            try:
                with open(log_path, 'a', encoding='utf-8') as f:
                    f.write(json.dumps({"timestamp": time.time(), "location": "write_page", "message": "Before write chunk", "data": {"offset": offset, "current_addr": hex(current_addr), "write_len": write_len}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "D"}) + "\n")
            except: pass
            # #endregion
            
            # 写入数据
            status, _ = self.send_command(CMD_WRITE_PAGE, current_addr, write_len, chunk)
            
            # #region agent log
            try:
                with open(log_path, 'a', encoding='utf-8') as f:
                    f.write(json.dumps({"timestamp": time.time(), "location": "write_page", "message": "After write chunk", "data": {"status": status, "status_hex": hex(status) if status is not None else "None"}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "D"}) + "\n")
            except: pass
            # #endregion
            
            if status != RESP_OK:
                # #region agent log
                try:
                    with open(log_path, 'a', encoding='utf-8') as f:
                        f.write(json.dumps({"timestamp": time.time(), "location": "write_page", "message": "Write failed", "data": {"status": status}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "D"}) + "\n")
                except: pass
                # #endregion
                return False
            
            offset += write_len
            current_addr += write_len
            time.sleep(0.01)  # 短暂延迟
        
        # #region agent log
        try:
            with open(log_path, 'a', encoding='utf-8') as f:
                f.write(json.dumps({"timestamp": time.time(), "location": "write_page", "message": "Exit success", "data": {}, "sessionId": "debug-session", "runId": "run1", "hypothesisId": "D"}) + "\n")
        except: pass
        # #endregion
        
        return True
    
    def verify_data(self, address: int, data: bytes) -> bool:
        """验证数据"""
        status, _ = self.send_command(CMD_VERIFY_DATA, address, len(data), data)
        return status == RESP_OK
    
    def get_flash_info(self) -> Optional[FlashInfo]:
        """获取Flash信息"""
        status, data = self.send_command(CMD_GET_INFO, 0, 0)
        if status == RESP_OK and data and len(data) >= 16:
            capacity, sector_size, page_size, sector_count = unpack('<IIII', data[:16])
            return FlashInfo(capacity, sector_size, page_size, sector_count)
        return None


class ImageParser:
    """图片解析器"""
    
    @staticmethod
    def parse_address_file(filename: str) -> List[int]:
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
    
    @staticmethod
    def parse_picture_file(filename: str) -> List[Tuple[str, bytes]]:
        """解析图片文件，返回 (name, data) 列表"""
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
    
    @staticmethod
    def validate_image_size(data: bytes, min_size: int = 1, max_size: int = 1024 * 1024) -> bool:
        """验证图片大小"""
        return min_size <= len(data) <= max_size


class BatchBurner:
    """批量烧录器（兼容GUI接口）"""
    
    def __init__(self, burner: W25Q64Burner):
        self.burner = burner

