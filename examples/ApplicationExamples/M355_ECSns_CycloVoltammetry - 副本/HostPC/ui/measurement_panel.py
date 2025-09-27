#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测量控制面板
"""

from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGroupBox,
                             QPushButton, QLabel, QProgressBar, QLCDNumber,
                             QTextEdit, QCheckBox)
from PyQt5.QtCore import pyqtSignal, QTimer, Qt
from PyQt5.QtGui import QFont

class MeasurementPanel(QWidget):
    """测量控制面板"""
    
    # 信号定义
    start_measurement = pyqtSignal()
    stop_measurement = pyqtSignal()
    pause_measurement = pyqtSignal()
    resume_measurement = pyqtSignal()
    
    def __init__(self):
        super().__init__()
        self.connected = False
        self.measuring = False
        self.paused = False
        self.measurement_time = 0
        
        self.init_ui()
        self.init_timer()
    
    def init_ui(self):
        """初始化界面"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)
        
        # 测量控制组
        control_group = QGroupBox("测量控制")
        control_layout = QVBoxLayout(control_group)
        
        # 开始按钮
        self.start_btn = QPushButton("开始测量")
        self.start_btn.setMinimumHeight(40)
        self.start_btn.setStyleSheet("""
            QPushButton {
                background-color: #4CAF50;
                color: white;
                font-weight: bold;
                border: none;
                border-radius: 5px;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:disabled {
                background-color: #cccccc;
                color: #666666;
            }
        """)
        self.start_btn.clicked.connect(self.on_start_clicked)
        self.start_btn.setEnabled(False)
        control_layout.addWidget(self.start_btn)
        
        # 停止按钮
        self.stop_btn = QPushButton("停止测量")
        self.stop_btn.setMinimumHeight(40)
        self.stop_btn.setStyleSheet("""
            QPushButton {
                background-color: #f44336;
                color: white;
                font-weight: bold;
                border: none;
                border-radius: 5px;
            }
            QPushButton:hover {
                background-color: #da190b;
            }
            QPushButton:disabled {
                background-color: #cccccc;
                color: #666666;
            }
        """)
        self.stop_btn.clicked.connect(self.on_stop_clicked)
        self.stop_btn.setEnabled(False)
        control_layout.addWidget(self.stop_btn)
        
        # 暂停/恢复按钮
        self.pause_btn = QPushButton("暂停测量")
        self.pause_btn.setMinimumHeight(40)
        self.pause_btn.setStyleSheet("""
            QPushButton {
                background-color: #ff9800;
                color: white;
                font-weight: bold;
                border: none;
                border-radius: 5px;
            }
            QPushButton:hover {
                background-color: #e68900;
            }
            QPushButton:disabled {
                background-color: #cccccc;
                color: #666666;
            }
        """)
        self.pause_btn.clicked.connect(self.on_pause_clicked)
        self.pause_btn.setEnabled(False)
        control_layout.addWidget(self.pause_btn)
        
        layout.addWidget(control_group)
        
        # 测量状态组
        status_group = QGroupBox("测量状态")
        status_layout = QVBoxLayout(status_group)
        
        # 状态标签
        self.status_label = QLabel("待机")
        self.status_label.setAlignment(Qt.AlignCenter)
        self.status_label.setStyleSheet("""
            QLabel {
                font-size: 14px;
                font-weight: bold;
                padding: 10px;
                border: 2px solid #cccccc;
                border-radius: 5px;
                background-color: #f0f0f0;
            }
        """)
        status_layout.addWidget(self.status_label)
        
        # 进度条
        self.progress_bar = QProgressBar()
        self.progress_bar.setRange(0, 100)
        self.progress_bar.setValue(0)
        self.progress_bar.setTextVisible(True)
        status_layout.addWidget(self.progress_bar)
        
        layout.addWidget(status_group)
        
        # 测量信息组
        info_group = QGroupBox("测量信息")
        info_layout = QVBoxLayout(info_group)
        
        # 测量时间显示
        time_layout = QHBoxLayout()
        time_layout.addWidget(QLabel("测量时间:"))
        
        self.time_lcd = QLCDNumber(8)
        self.time_lcd.setDigitCount(8)
        self.time_lcd.setSegmentStyle(QLCDNumber.Flat)
        self.time_lcd.display("00:00:00")
        time_layout.addWidget(self.time_lcd)
        
        info_layout.addLayout(time_layout)
        
        # 数据点计数
        count_layout = QHBoxLayout()
        count_layout.addWidget(QLabel("数据点数:"))
        
        self.count_lcd = QLCDNumber(6)
        self.count_lcd.setDigitCount(6)
        self.count_lcd.setSegmentStyle(QLCDNumber.Flat)
        self.count_lcd.display("0")
        count_layout.addWidget(self.count_lcd)
        
        info_layout.addLayout(count_layout)
        
        # 当前电压显示
        voltage_layout = QHBoxLayout()
        voltage_layout.addWidget(QLabel("当前电压:"))
        
        self.voltage_label = QLabel("0.000 V")
        self.voltage_label.setStyleSheet("font-weight: bold; color: blue;")
        voltage_layout.addWidget(self.voltage_label)
        
        info_layout.addLayout(voltage_layout)
        
        # 当前电流显示
        current_layout = QHBoxLayout()
        current_layout.addWidget(QLabel("当前电流:"))
        
        self.current_label = QLabel("0.000 μA")
        self.current_label.setStyleSheet("font-weight: bold; color: red;")
        current_layout.addWidget(self.current_label)
        
        info_layout.addLayout(current_layout)
        
        layout.addWidget(info_group)
        
        # 测量选项组
        options_group = QGroupBox("测量选项")
        options_layout = QVBoxLayout(options_group)
        
        # 自动保存数据
        self.auto_save_check = QCheckBox("自动保存数据")
        self.auto_save_check.setChecked(True)
        options_layout.addWidget(self.auto_save_check)
        
        # 实时显示
        self.realtime_display_check = QCheckBox("实时显示曲线")
        self.realtime_display_check.setChecked(True)
        options_layout.addWidget(self.realtime_display_check)
        
        # 声音提示
        self.sound_alert_check = QCheckBox("完成时声音提示")
        self.sound_alert_check.setChecked(False)
        options_layout.addWidget(self.sound_alert_check)
        
        layout.addWidget(options_group)
        
        # 添加弹性空间
        layout.addStretch()
    
    def init_timer(self):
        """初始化定时器"""
        self.measurement_timer = QTimer()
        self.measurement_timer.timeout.connect(self.update_measurement_time)
        self.measurement_timer.setInterval(1000)  # 每秒更新一次
    
    def set_connected(self, connected):
        """设置连接状态"""
        self.connected = connected
        self.start_btn.setEnabled(connected and not self.measuring)

    def _apply_idle_state(self):
        """更新界面为待机状态"""
        self.start_btn.setEnabled(self.connected)
        self.stop_btn.setEnabled(False)
        self.pause_btn.setEnabled(False)
        self.pause_btn.setText("暂停测量")

        self.status_label.setText("待机")
        self.status_label.setStyleSheet("""
            QLabel {
                font-size: 14px;
                font-weight: bold;
                padding: 10px;
                border: 2px solid #cccccc;
                border-radius: 5px;
                background-color: #f0f0f0;
            }
        """)

        self.measurement_timer.stop()

    def on_start_clicked(self):
        """开始按钮点击"""
        if not self.connected:
            return

        self.measuring = True
        self.paused = False
        self.measurement_time = 0
        
        # 更新界面状态
        self.start_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)
        self.pause_btn.setEnabled(True)
        self.pause_btn.setText("暂停测量")
        
        self.status_label.setText("测量中")
        self.status_label.setStyleSheet("""
            QLabel {
                font-size: 14px;
                font-weight: bold;
                padding: 10px;
                border: 2px solid #4CAF50;
                border-radius: 5px;
                background-color: #e8f5e8;
                color: #2e7d32;
            }
        """)
        
        # 重置显示
        self.progress_bar.setValue(0)
        self.count_lcd.display("0")
        self.voltage_label.setText("0.000 V")
        self.current_label.setText("0.000 μA")
        
        # 启动计时器
        self.measurement_timer.start()
        
        # 发射开始信号
        self.start_measurement.emit()
    
    def on_stop_clicked(self):
        """停止按钮点击"""
        self.measuring = False
        self.paused = False

        # 更新界面状态
        self._apply_idle_state()

        # 发射停止信号
        self.stop_measurement.emit()
    
    def on_pause_clicked(self):
        """暂停/恢复按钮点击"""
        if self.paused:
            # 恢复测量
            self.paused = False
            self.pause_btn.setText("暂停测量")
            
            self.status_label.setText("测量中")
            self.status_label.setStyleSheet("""
                QLabel {
                    font-size: 14px;
                    font-weight: bold;
                    padding: 10px;
                    border: 2px solid #4CAF50;
                    border-radius: 5px;
                    background-color: #e8f5e8;
                    color: #2e7d32;
                }
            """)
            
            # 恢复计时器
            self.measurement_timer.start()
            
            # 发射恢复信号
            self.resume_measurement.emit()
        else:
            # 暂停测量
            self.paused = True
            self.pause_btn.setText("恢复测量")
            
            self.status_label.setText("已暂停")
            self.status_label.setStyleSheet("""
                QLabel {
                    font-size: 14px;
                    font-weight: bold;
                    padding: 10px;
                    border: 2px solid #ff9800;
                    border-radius: 5px;
                    background-color: #fff3e0;
                    color: #f57c00;
                }
            """)
            
            # 暂停计时器
            self.measurement_timer.stop()
            
            # 发射暂停信号
            self.pause_measurement.emit()
    
    def update_measurement_time(self):
        """更新测量时间显示"""
        if self.measuring and not self.paused:
            self.measurement_time += 1
            
            hours = self.measurement_time // 3600
            minutes = (self.measurement_time % 3600) // 60
            seconds = self.measurement_time % 60
            
            time_str = f"{hours:02d}:{minutes:02d}:{seconds:02d}"
            self.time_lcd.display(time_str)
    
    def update_data_count(self, count):
        """更新数据点计数"""
        self.count_lcd.display(str(count))
    
    def update_current_values(self, voltage, current):
        """更新当前电压电流值"""
        self.voltage_label.setText(f"{voltage:.3f} V")
        
        # 自动选择合适的电流单位
        if abs(current) >= 1e-3:
            self.current_label.setText(f"{current*1000:.3f} mA")
        elif abs(current) >= 1e-6:
            self.current_label.setText(f"{current*1e6:.3f} μA")
        elif abs(current) >= 1e-9:
            self.current_label.setText(f"{current*1e9:.3f} nA")
        else:
            self.current_label.setText(f"{current*1e12:.3f} pA")
    
    def update_progress(self, progress):
        """更新进度条"""
        self.progress_bar.setValue(int(progress))
    
    def measurement_completed(self):
        """测量完成"""
        self.measuring = False
        self.paused = False
        self._apply_idle_state()
        
        # 如果启用了声音提示
        if self.sound_alert_check.isChecked():
            # 这里可以添加声音提示代码
            pass
    
    def get_measurement_options(self):
        """获取测量选项"""
        return {
            'auto_save': self.auto_save_check.isChecked(),
            'realtime_display': self.realtime_display_check.isChecked(),
            'sound_alert': self.sound_alert_check.isChecked()
        }
    
    def is_measuring(self):
        """检查是否正在测量"""
        return self.measuring
    
    def is_paused(self):
        """检查是否已暂停"""
        return self.paused