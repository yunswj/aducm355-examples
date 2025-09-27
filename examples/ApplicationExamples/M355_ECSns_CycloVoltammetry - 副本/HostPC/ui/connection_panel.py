#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
连接设置面板
"""

from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGroupBox,
                             QComboBox, QLabel, QPushButton, QSpinBox,
                             QMessageBox)
from PyQt5.QtCore import pyqtSignal, QTimer
from PyQt5.QtSerialPort import QSerialPortInfo

class ConnectionPanel(QWidget):
    """连接设置面板"""
    
    # 信号定义
    connect_requested = pyqtSignal(str, int)
    disconnect_requested = pyqtSignal()
    
    def __init__(self):
        super().__init__()
        self.connected = False
        self.init_ui()
        
        # 定时刷新串口列表
        self.refresh_timer = QTimer()
        self.refresh_timer.timeout.connect(self.refresh_ports)
        self.refresh_timer.start(2000)  # 每2秒刷新一次
        
        # 初始刷新
        self.refresh_ports()
    
    def init_ui(self):
        """初始化界面"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)
        
        # 串口设置组
        serial_group = QGroupBox("串口设置")
        serial_layout = QVBoxLayout(serial_group)
        
        # 串口选择
        port_layout = QHBoxLayout()
        port_layout.addWidget(QLabel("串口:"))
        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(150)
        port_layout.addWidget(self.port_combo)
        
        refresh_btn = QPushButton("刷新")
        refresh_btn.clicked.connect(self.refresh_ports)
        port_layout.addWidget(refresh_btn)
        
        serial_layout.addLayout(port_layout)
        
        # 波特率设置
        baudrate_layout = QHBoxLayout()
        baudrate_layout.addWidget(QLabel("波特率:"))
        self.baudrate_combo = QComboBox()
        self.baudrate_combo.addItems([
            "9600", "19200", "38400", "57600", 
            "115200", "230400", "460800", "921600"
        ])
        self.baudrate_combo.setCurrentText("115200")
        baudrate_layout.addWidget(self.baudrate_combo)
        
        serial_layout.addLayout(baudrate_layout)
        
        layout.addWidget(serial_group)
        
        # 连接控制组
        control_group = QGroupBox("连接控制")
        control_layout = QVBoxLayout(control_group)
        
        # 连接按钮
        self.connect_btn = QPushButton("连接")
        self.connect_btn.clicked.connect(self.on_connect_clicked)
        control_layout.addWidget(self.connect_btn)
        
        # 断开按钮
        self.disconnect_btn = QPushButton("断开")
        self.disconnect_btn.clicked.connect(self.on_disconnect_clicked)
        self.disconnect_btn.setEnabled(False)
        control_layout.addWidget(self.disconnect_btn)
        
        layout.addWidget(control_group)
        
        # 连接状态组
        status_group = QGroupBox("连接状态")
        status_layout = QVBoxLayout(status_group)
        
        self.status_label = QLabel("未连接")
        self.status_label.setStyleSheet("color: red; font-weight: bold;")
        status_layout.addWidget(self.status_label)
        
        layout.addWidget(status_group)
        
        # 添加弹性空间
        layout.addStretch()
    
    def refresh_ports(self):
        """刷新串口列表"""
        current_port = self.port_combo.currentText()
        self.port_combo.clear()
        
        # 获取可用串口
        ports = QSerialPortInfo.availablePorts()
        port_names = []
        
        for port in ports:
            port_name = port.portName()
            port_desc = port.description()
            if port_desc:
                display_name = f"{port_name} - {port_desc}"
            else:
                display_name = port_name
            
            self.port_combo.addItem(display_name, port_name)
            port_names.append(port_name)
        
        # 尝试恢复之前选择的串口
        if current_port:
            index = self.port_combo.findData(current_port)
            if index >= 0:
                self.port_combo.setCurrentIndex(index)
        
        # 如果没有可用串口，添加提示
        if not ports:
            self.port_combo.addItem("无可用串口", "")
    
    def on_connect_clicked(self):
        """连接按钮点击"""
        if self.port_combo.count() == 0 or not self.port_combo.currentData():
            QMessageBox.warning(self, "警告", "请选择有效的串口")
            return
        
        port = self.port_combo.currentData()
        baudrate = int(self.baudrate_combo.currentText())
        
        self.connect_requested.emit(port, baudrate)
    
    def on_disconnect_clicked(self):
        """断开按钮点击"""
        self.disconnect_requested.emit()
    
    def set_connected(self, connected):
        """设置连接状态"""
        self.connected = connected
        
        if connected:
            self.status_label.setText("已连接")
            self.status_label.setStyleSheet("color: green; font-weight: bold;")
            self.connect_btn.setEnabled(False)
            self.disconnect_btn.setEnabled(True)
            self.port_combo.setEnabled(False)
            self.baudrate_combo.setEnabled(False)
        else:
            self.status_label.setText("未连接")
            self.status_label.setStyleSheet("color: red; font-weight: bold;")
            self.connect_btn.setEnabled(True)
            self.disconnect_btn.setEnabled(False)
            self.port_combo.setEnabled(True)
            self.baudrate_combo.setEnabled(True)
    
    def get_config(self):
        """获取配置"""
        return {
            'port': self.port_combo.currentData() or "",
            'baudrate': int(self.baudrate_combo.currentText())
        }
    
    def load_config(self, config):
        """加载配置"""
        if 'port' in config and config['port']:
            index = self.port_combo.findData(config['port'])
            if index >= 0:
                self.port_combo.setCurrentIndex(index)
        
        if 'baudrate' in config:
            self.baudrate_combo.setCurrentText(str(config['baudrate']))