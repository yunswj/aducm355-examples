#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
串口通信管理器
"""

import time
import threading
from PyQt5.QtCore import QObject, pyqtSignal, QTimer
from PyQt5.QtSerialPort import QSerialPort, QSerialPortInfo

class SerialManager(QObject):
    """串口通信管理器"""
    
    # 信号定义
    connection_changed = pyqtSignal(bool)
    data_received = pyqtSignal(str)
    error_occurred = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()
        self.serial_port = None
        self.connected = False
        self.rx_buffer = ""
        
        # 命令队列和发送锁
        self.command_queue = []
        self.send_lock = threading.Lock()
        
        # 响应超时定时器
        self.response_timer = QTimer()
        self.response_timer.timeout.connect(self.on_response_timeout)
        self.response_timer.setSingleShot(True)
        
        # 当前等待响应的命令
        self.pending_command = None
        self.response_timeout = 5000  # 5秒超时
    
    def connect(self, port_name, baudrate):
        """连接串口"""
        try:
            if self.connected:
                self.disconnect()
            
            # 创建串口对象
            self.serial_port = QSerialPort()
            self.serial_port.setPortName(port_name)
            self.serial_port.setBaudRate(baudrate)
            self.serial_port.setDataBits(QSerialPort.Data8)
            self.serial_port.setParity(QSerialPort.NoParity)
            self.serial_port.setStopBits(QSerialPort.OneStop)
            self.serial_port.setFlowControl(QSerialPort.NoFlowControl)
            
            # 连接信号
            self.serial_port.readyRead.connect(self.on_data_ready)
            self.serial_port.errorOccurred.connect(self.on_error_occurred)
            
            # 打开串口
            if self.serial_port.open(QSerialPort.ReadWrite):
                self.connected = True
                self.connection_changed.emit(True)
                
                # 清空缓冲区
                self.serial_port.clear()
                self.rx_buffer = ""
                
                return True
            else:
                error_msg = f"无法打开串口: {self.serial_port.errorString()}"
                self.error_occurred.emit(error_msg)
                return False
                
        except Exception as e:
            error_msg = f"连接串口时发生错误: {str(e)}"
            self.error_occurred.emit(error_msg)
            return False
    
    def disconnect(self):
        """断开串口连接"""
        if self.serial_port and self.connected:
            self.serial_port.close()
            self.connected = False
            self.connection_changed.emit(False)
            
            # 停止响应定时器
            self.response_timer.stop()
            self.pending_command = None
            
            # 清空命令队列
            self.command_queue.clear()
    
    def is_connected(self):
        """检查是否已连接"""
        return self.connected and self.serial_port and self.serial_port.isOpen()
    
    def send_command(self, command):
        """发送命令"""
        if not self.is_connected():
            self.error_occurred.emit("串口未连接")
            return False
        
        try:
            with self.send_lock:
                # 确保命令格式正确
                if not command.startswith('$'):
                    command = '$' + command
                
                if not command.endswith('\r\n'):
                    if not command.endswith('*XX'):
                        if '*' not in command:
                            command += '*XX'
                    command += '\r\n'
                
                # 发送命令
                data = command.encode('utf-8')
                bytes_written = self.serial_port.write(data)
                
                if bytes_written == len(data):
                    # 设置等待响应
                    self.pending_command = command.strip()
                    self.response_timer.start(self.response_timeout)
                    return True
                else:
                    self.error_occurred.emit("命令发送不完整")
                    return False
                    
        except Exception as e:
            error_msg = f"发送命令时发生错误: {str(e)}"
            self.error_occurred.emit(error_msg)
            return False
    
    def send_raw_data(self, data):
        """发送原始数据"""
        if not self.is_connected():
            return False
        
        try:
            if isinstance(data, str):
                data = data.encode('utf-8')
            
            bytes_written = self.serial_port.write(data)
            return bytes_written == len(data)
            
        except Exception as e:
            error_msg = f"发送数据时发生错误: {str(e)}"
            self.error_occurred.emit(error_msg)
            return False
    
    def on_data_ready(self):
        """处理接收到的数据"""
        if not self.serial_port:
            return
        
        try:
            # 读取所有可用数据
            data = self.serial_port.readAll()
            text = data.data().decode('utf-8', errors='ignore')
            
            # 添加到接收缓冲区
            self.rx_buffer += text
            
            # 处理完整的行
            while '\n' in self.rx_buffer:
                line, self.rx_buffer = self.rx_buffer.split('\n', 1)
                line = line.strip()
                
                if line:
                    self.process_received_line(line)
                    
        except Exception as e:
            error_msg = f"处理接收数据时发生错误: {str(e)}"
            self.error_occurred.emit(error_msg)
    
    def process_received_line(self, line):
        """处理接收到的完整行"""
        # 发射数据接收信号
        self.data_received.emit(line)
        
        # 如果有等待响应的命令，停止超时定时器
        if self.pending_command:
            self.response_timer.stop()
            self.pending_command = None
        
        # 解析特定类型的响应
        if line.startswith('$ACK'):
            # 确认响应
            pass
        elif line.startswith('$NAK'):
            # 否定响应
            self.error_occurred.emit(f"命令执行失败: {line}")
        elif line.startswith('$DATA'):
            # 数据响应
            pass
        elif line.startswith('$INFO'):
            # 信息响应
            pass
    
    def on_response_timeout(self):
        """响应超时处理"""
        if self.pending_command:
            error_msg = f"命令响应超时: {self.pending_command}"
            self.error_occurred.emit(error_msg)
            self.pending_command = None
    
    def on_error_occurred(self, error):
        """处理串口错误"""
        error_msg = f"串口错误: {self.serial_port.errorString()}"
        self.error_occurred.emit(error_msg)
        
        # 如果是严重错误，断开连接
        if error in [QSerialPort.ResourceError, QSerialPort.DeviceNotFoundError]:
            self.disconnect()
    
    def get_available_ports(self):
        """获取可用串口列表"""
        ports = []
        for port_info in QSerialPortInfo.availablePorts():
            ports.append({
                'name': port_info.portName(),
                'description': port_info.description(),
                'manufacturer': port_info.manufacturer(),
                'serial_number': port_info.serialNumber()
            })
        return ports
    
    def calculate_checksum(self, command):
        """计算命令校验和"""
        # 简单的XOR校验和
        checksum = 0
        for char in command:
            checksum ^= ord(char)
        return f"{checksum:02X}"
    
    def verify_checksum(self, line):
        """验证接收数据的校验和"""
        if '*' not in line:
            return True  # 没有校验和，认为有效
        
        try:
            data_part, checksum_part = line.rsplit('*', 1)
            expected_checksum = self.calculate_checksum(data_part)
            return checksum_part.upper() == expected_checksum.upper()
        except:
            return False
    
    def set_response_timeout(self, timeout_ms):
        """设置响应超时时间"""
        self.response_timeout = timeout_ms
    
    def clear_buffers(self):
        """清空缓冲区"""
        if self.serial_port:
            self.serial_port.clear()
        self.rx_buffer = ""
        self.command_queue.clear()
        
        if self.response_timer.isActive():
            self.response_timer.stop()
        self.pending_command = None