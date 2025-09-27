#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
参数配置面板
"""

from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGroupBox,
                             QLabel, QDoubleSpinBox, QSpinBox, QComboBox,
                             QPushButton, QSlider, QCheckBox, QFormLayout)
from PyQt5.QtCore import pyqtSignal, Qt

class ParameterPanel(QWidget):
    """参数配置面板"""
    
    # 信号定义
    parameter_changed = pyqtSignal(str, object)
    
    def __init__(self):
        super().__init__()
        self.connected = False
        self.init_ui()
        self.init_connections()
    
    def init_ui(self):
        """初始化界面"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)
        
        # 电压范围设置组
        voltage_group = QGroupBox("电压范围设置")
        voltage_layout = QFormLayout(voltage_group)
        
        # 起始电压
        self.start_voltage_spin = QDoubleSpinBox()
        self.start_voltage_spin.setRange(-2.0, 2.0)
        self.start_voltage_spin.setSingleStep(0.01)
        self.start_voltage_spin.setDecimals(3)
        self.start_voltage_spin.setValue(-0.5)
        self.start_voltage_spin.setSuffix(" V")
        voltage_layout.addRow("起始电压:", self.start_voltage_spin)
        
        # 峰值电压
        self.peak_voltage_spin = QDoubleSpinBox()
        self.peak_voltage_spin.setRange(-2.0, 2.0)
        self.peak_voltage_spin.setSingleStep(0.01)
        self.peak_voltage_spin.setDecimals(3)
        self.peak_voltage_spin.setValue(0.5)
        self.peak_voltage_spin.setSuffix(" V")
        voltage_layout.addRow("峰值电压:", self.peak_voltage_spin)
        
        layout.addWidget(voltage_group)
        
        # 电流范围设置组
        current_group = QGroupBox("电流范围设置")
        current_layout = QFormLayout(current_group)
        
        # RTIA索引选择
        self.rtia_combo = QComboBox()
        rtia_values = [
            ("0 - 200Ω", 0),
            ("1 - 1kΩ", 1),
            ("2 - 5kΩ", 2),
            ("3 - 10kΩ", 3),
            ("4 - 20kΩ", 4),
            ("5 - 40kΩ", 5),
            ("6 - 80kΩ", 6),
            ("7 - 160kΩ", 7),
            ("8 - 320kΩ", 8),
            ("9 - 640kΩ", 9),
            ("10 - 1.28MΩ", 10),
            ("11 - 2.56MΩ", 11)
        ]
        
        for text, value in rtia_values:
            self.rtia_combo.addItem(text, value)
        
        self.rtia_combo.setCurrentIndex(3)  # 默认10kΩ
        current_layout.addRow("RTIA阻值:", self.rtia_combo)
        
        layout.addWidget(current_group)
        
        # 扫描参数设置组
        scan_group = QGroupBox("扫描参数设置")
        scan_layout = QFormLayout(scan_group)
        
        # 扫描速率
        self.scan_rate_spin = QDoubleSpinBox()
        self.scan_rate_spin.setRange(0.001, 10.0)
        self.scan_rate_spin.setSingleStep(0.001)
        self.scan_rate_spin.setDecimals(3)
        self.scan_rate_spin.setValue(0.1)
        self.scan_rate_spin.setSuffix(" V/s")
        scan_layout.addRow("扫描速率:", self.scan_rate_spin)
        
        # 扫描步数
        self.steps_spin = QSpinBox()
        self.steps_spin.setRange(10, 1000)
        self.steps_spin.setSingleStep(1)
        self.steps_spin.setValue(100)
        scan_layout.addRow("扫描步数:", self.steps_spin)
        
        # 每步持续时间
        self.duration_spin = QSpinBox()
        self.duration_spin.setRange(1, 10000)
        self.duration_spin.setSingleStep(1)
        self.duration_spin.setValue(100)
        self.duration_spin.setSuffix(" ms")
        scan_layout.addRow("每步时间:", self.duration_spin)
        
        layout.addWidget(scan_group)
        
        # 测量模式设置组
        mode_group = QGroupBox("测量模式设置")
        mode_layout = QFormLayout(mode_group)
        
        # 测量模式
        self.mode_combo = QComboBox()
        self.mode_combo.addItem("单次扫描", 0)
        self.mode_combo.addItem("连续扫描", 1)
        self.mode_combo.addItem("差分脉冲", 2)
        mode_layout.addRow("测量模式:", self.mode_combo)
        
        # 循环次数
        self.cycles_spin = QSpinBox()
        self.cycles_spin.setRange(1, 100)
        self.cycles_spin.setValue(1)
        mode_layout.addRow("循环次数:", self.cycles_spin)
        
        layout.addWidget(mode_group)
        
        # 高级设置组
        advanced_group = QGroupBox("高级设置")
        advanced_layout = QFormLayout(advanced_group)
        
        # 自动量程
        self.auto_range_check = QCheckBox("启用自动量程")
        advanced_layout.addRow(self.auto_range_check)
        
        # 数据平均
        self.averaging_check = QCheckBox("启用数据平均")
        advanced_layout.addRow(self.averaging_check)
        
        # 平均次数
        self.avg_count_spin = QSpinBox()
        self.avg_count_spin.setRange(1, 16)
        self.avg_count_spin.setValue(4)
        self.avg_count_spin.setEnabled(False)
        advanced_layout.addRow("平均次数:", self.avg_count_spin)
        
        layout.addWidget(advanced_group)
        
        # 控制按钮组
        button_group = QGroupBox("参数控制")
        button_layout = QVBoxLayout(button_group)
        
        # 应用参数按钮
        self.apply_btn = QPushButton("应用参数")
        self.apply_btn.setEnabled(False)
        button_layout.addWidget(self.apply_btn)
        
        # 查询参数按钮
        self.query_btn = QPushButton("查询参数")
        self.query_btn.setEnabled(False)
        button_layout.addWidget(self.query_btn)
        
        # 重置参数按钮
        self.reset_btn = QPushButton("重置参数")
        button_layout.addWidget(self.reset_btn)
        
        layout.addWidget(button_group)
        
        # 添加弹性空间
        layout.addStretch()
    
    def init_connections(self):
        """初始化信号连接"""
        # 参数变化信号
        self.start_voltage_spin.valueChanged.connect(
            lambda v: self.parameter_changed.emit("start_voltage", v))
        self.peak_voltage_spin.valueChanged.connect(
            lambda v: self.parameter_changed.emit("peak_voltage", v))
        self.rtia_combo.currentIndexChanged.connect(
            lambda i: self.parameter_changed.emit("rtia_index", self.rtia_combo.currentData()))
        self.scan_rate_spin.valueChanged.connect(
            lambda v: self.parameter_changed.emit("scan_rate", v))
        self.steps_spin.valueChanged.connect(
            lambda v: self.parameter_changed.emit("steps", v))
        self.duration_spin.valueChanged.connect(
            lambda v: self.parameter_changed.emit("duration", v))
        self.mode_combo.currentIndexChanged.connect(
            lambda i: self.parameter_changed.emit("mode", self.mode_combo.currentData()))
        
        # 按钮信号
        self.apply_btn.clicked.connect(self.apply_parameters)
        self.query_btn.clicked.connect(self.query_parameters)
        self.reset_btn.clicked.connect(self.reset_parameters)
        
        # 高级设置信号
        self.averaging_check.toggled.connect(self.avg_count_spin.setEnabled)
        self.averaging_check.toggled.connect(
            lambda checked: self.parameter_changed.emit("averaging", checked))
        self.avg_count_spin.valueChanged.connect(
            lambda v: self.parameter_changed.emit("avg_count", v))
        self.auto_range_check.toggled.connect(
            lambda checked: self.parameter_changed.emit("auto_range", checked))
    
    def set_connected(self, connected):
        """设置连接状态"""
        self.connected = connected
        self.apply_btn.setEnabled(connected)
        self.query_btn.setEnabled(connected)
    
    def apply_parameters(self):
        """应用所有参数"""
        if not self.connected:
            return
        
        # 发送电压范围设置命令
        start_voltage = self.start_voltage_spin.value()
        peak_voltage = self.peak_voltage_spin.value()
        self.parameter_changed.emit("voltage_range", (start_voltage, peak_voltage))
        
        # 发送电流范围设置命令
        rtia_index = self.rtia_combo.currentData()
        self.parameter_changed.emit("current_range", rtia_index)
        
        # 发送扫描速率命令
        scan_rate = self.scan_rate_spin.value()
        self.parameter_changed.emit("scan_rate", scan_rate)
        
        # 发送扫描参数命令
        steps = self.steps_spin.value()
        duration = self.duration_spin.value()
        self.parameter_changed.emit("scan_params", (steps, duration))
        
        # 发送测量模式命令
        mode = self.mode_combo.currentData()
        self.parameter_changed.emit("measurement_mode", mode)
    
    def query_parameters(self):
        """查询所有参数"""
        if not self.connected:
            return
        
        # 发送查询命令
        self.parameter_changed.emit("query_all", None)
    
    def reset_parameters(self):
        """重置参数到默认值"""
        self.start_voltage_spin.setValue(-0.5)
        self.peak_voltage_spin.setValue(0.5)
        self.rtia_combo.setCurrentIndex(3)
        self.scan_rate_spin.setValue(0.1)
        self.steps_spin.setValue(100)
        self.duration_spin.setValue(100)
        self.mode_combo.setCurrentIndex(0)
        self.cycles_spin.setValue(1)
        self.auto_range_check.setChecked(False)
        self.averaging_check.setChecked(False)
        self.avg_count_spin.setValue(4)
    
    def get_command(self, param_type, value):
        """根据参数类型和值生成命令"""
        commands = {
            "voltage_range": lambda v: f"$SVR,{v[0]:.3f},{v[1]:.3f}*XX",
            "current_range": lambda v: f"$SCA,{v}*XX",
            "scan_rate": lambda v: f"$SSR,{v:.3f}*XX",
            "scan_params": lambda v: f"$SSP,{v[0]},{v[1]}*XX",
            "measurement_mode": lambda v: f"$SMD,{v}*XX",
            "query_all": lambda v: "$QALL*XX"
        }
        
        if param_type in commands:
            return commands[param_type](value)
        return None
    
    def get_config(self):
        """获取当前配置"""
        return {
            'start_voltage': self.start_voltage_spin.value(),
            'peak_voltage': self.peak_voltage_spin.value(),
            'rtia_index': self.rtia_combo.currentData(),
            'scan_rate': self.scan_rate_spin.value(),
            'steps': self.steps_spin.value(),
            'duration': self.duration_spin.value(),
            'mode': self.mode_combo.currentData(),
            'cycles': self.cycles_spin.value(),
            'auto_range': self.auto_range_check.isChecked(),
            'averaging': self.averaging_check.isChecked(),
            'avg_count': self.avg_count_spin.value()
        }
    
    def load_config(self, config):
        """加载配置"""
        if 'start_voltage' in config:
            self.start_voltage_spin.setValue(config['start_voltage'])
        if 'peak_voltage' in config:
            self.peak_voltage_spin.setValue(config['peak_voltage'])
        if 'rtia_index' in config:
            index = self.rtia_combo.findData(config['rtia_index'])
            if index >= 0:
                self.rtia_combo.setCurrentIndex(index)
        if 'scan_rate' in config:
            self.scan_rate_spin.setValue(config['scan_rate'])
        if 'steps' in config:
            self.steps_spin.setValue(config['steps'])
        if 'duration' in config:
            self.duration_spin.setValue(config['duration'])
        if 'mode' in config:
            index = self.mode_combo.findData(config['mode'])
            if index >= 0:
                self.mode_combo.setCurrentIndex(index)
        if 'cycles' in config:
            self.cycles_spin.setValue(config['cycles'])
        if 'auto_range' in config:
            self.auto_range_check.setChecked(config['auto_range'])
        if 'averaging' in config:
            self.averaging_check.setChecked(config['averaging'])
        if 'avg_count' in config:
            self.avg_count_spin.setValue(config['avg_count'])
    
    def update_parameter_from_response(self, response):
        """根据设备响应更新参数显示"""
        # 解析设备响应并更新界面
        # 这里可以根据具体的响应格式来实现
        pass