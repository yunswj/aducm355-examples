#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
主窗口界面
"""

from PyQt5.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
                             QSplitter, QTabWidget, QStatusBar, QMenuBar, 
                             QAction, QMessageBox, QLabel, QFrame)
from PyQt5.QtCore import Qt, QTimer, pyqtSignal
from PyQt5.QtGui import QIcon, QPixmap

from .connection_panel import ConnectionPanel
from .parameter_panel import ParameterPanel
from .measurement_panel import MeasurementPanel
from .data_visualization import DataVisualization
import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from communication.serial_manager import SerialManager

class MainWindow(QMainWindow):
    """主窗口类"""
    
    # 信号定义
    connection_status_changed = pyqtSignal(bool)
    measurement_data_received = pyqtSignal(float, float, int)
    
    def __init__(self):
        super().__init__()
        self.serial_manager = SerialManager()
        
        # 初始化界面
        self.init_ui()
        self.init_connections()
        self.init_status_bar()
        self.init_menu_bar()
        
        # 状态更新定时器
        self.status_timer = QTimer()
        self.status_timer.timeout.connect(self.update_status)
        self.status_timer.start(1000)  # 每秒更新一次
    
    def init_ui(self):
        """初始化用户界面"""
        self.setWindowTitle("循环伏安法上位机 v1.0")
        self.setMinimumSize(1200, 800)
        self.resize(1400, 900)
        
        # 创建中央部件
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # 创建主布局
        main_layout = QHBoxLayout(central_widget)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.setSpacing(5)
        
        # 创建分割器
        main_splitter = QSplitter(Qt.Horizontal)
        main_layout.addWidget(main_splitter)
        
        # 左侧控制面板
        left_panel = self.create_left_panel()
        main_splitter.addWidget(left_panel)
        
        # 右侧数据显示面板
        right_panel = self.create_right_panel()
        main_splitter.addWidget(right_panel)
        
        # 设置分割器比例
        main_splitter.setSizes([400, 1000])
        main_splitter.setStretchFactor(0, 0)
        main_splitter.setStretchFactor(1, 1)
    
    def create_left_panel(self):
        """创建左侧控制面板"""
        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(5)
        
        # 创建选项卡
        tab_widget = QTabWidget()
        left_layout.addWidget(tab_widget)
        
        # 连接设置选项卡
        self.connection_panel = ConnectionPanel()
        tab_widget.addTab(self.connection_panel, "连接设置")
        
        # 参数配置选项卡
        self.parameter_panel = ParameterPanel()
        tab_widget.addTab(self.parameter_panel, "参数配置")
        
        # 测量控制选项卡
        self.measurement_panel = MeasurementPanel()
        tab_widget.addTab(self.measurement_panel, "测量控制")
        
        return left_widget
    
    def create_right_panel(self):
        """创建右侧数据显示面板"""
        # 数据可视化面板
        self.data_visualization = DataVisualization()
        return self.data_visualization
    
    def init_connections(self):
        """初始化信号连接"""
        # 连接面板信号
        self.connection_panel.connect_requested.connect(self.on_connect_requested)
        self.connection_panel.disconnect_requested.connect(self.on_disconnect_requested)
        
        # 参数面板信号
        self.parameter_panel.parameter_changed.connect(self.on_parameter_changed)
        
        # 测量面板信号
        self.measurement_panel.start_measurement.connect(self.on_start_measurement)
        self.measurement_panel.stop_measurement.connect(self.on_stop_measurement)
        self.measurement_panel.pause_measurement.connect(self.on_pause_measurement)
        self.measurement_panel.resume_measurement.connect(self.on_resume_measurement)
        
        # 串口管理器信号
        self.serial_manager.connection_changed.connect(self.on_connection_changed)
        self.serial_manager.data_received.connect(self.on_data_received)
        self.serial_manager.error_occurred.connect(self.on_error_occurred)
        
        # 内部信号
        self.connection_status_changed.connect(self.update_connection_status)
        self.measurement_data_received.connect(self.data_visualization.add_data_point)
    
    def init_status_bar(self):
        """初始化状态栏"""
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        
        # 连接状态标签
        self.connection_status_label = QLabel("未连接")
        self.connection_status_label.setStyleSheet("color: red; font-weight: bold;")
        self.status_bar.addPermanentWidget(self.connection_status_label)
        
        # 分隔符
        separator = QFrame()
        separator.setFrameShape(QFrame.VLine)
        separator.setFrameShadow(QFrame.Sunken)
        self.status_bar.addPermanentWidget(separator)
        
        # 测量状态标签
        self.measurement_status_label = QLabel("待机")
        self.status_bar.addPermanentWidget(self.measurement_status_label)
        
        # 数据点计数标签
        self.data_count_label = QLabel("数据点: 0")
        self.status_bar.addPermanentWidget(self.data_count_label)
    
    def init_menu_bar(self):
        """初始化菜单栏"""
        menubar = self.menuBar()
        
        # 文件菜单
        file_menu = menubar.addMenu('文件(&F)')
        
        # 导出数据
        export_action = QAction('导出数据(&E)', self)
        export_action.setShortcut('Ctrl+E')
        export_action.triggered.connect(self.export_data)
        file_menu.addAction(export_action)
        
        file_menu.addSeparator()
        
        # 退出
        exit_action = QAction('退出(&X)', self)
        exit_action.setShortcut('Ctrl+Q')
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # 工具菜单
        tools_menu = menubar.addMenu('工具(&T)')
        
        # 清除数据
        clear_action = QAction('清除数据(&C)', self)
        clear_action.triggered.connect(self.clear_data)
        tools_menu.addAction(clear_action)
        
        # 帮助菜单
        help_menu = menubar.addMenu('帮助(&H)')
        
        # 关于
        about_action = QAction('关于(&A)', self)
        about_action.triggered.connect(self.show_about)
        help_menu.addAction(about_action)
    
    def on_connect_requested(self, port, baudrate):
        """处理连接请求"""
        try:
            self.serial_manager.connect(port, baudrate)
        except Exception as e:
            QMessageBox.critical(self, "连接错误", f"无法连接到串口: {str(e)}")
    
    def on_disconnect_requested(self):
        """处理断开连接请求"""
        self.serial_manager.disconnect()
    
    def on_connection_changed(self, connected):
        """处理连接状态变化"""
        self.connection_status_changed.emit(connected)
    
    def update_connection_status(self, connected):
        """更新连接状态显示"""
        if connected:
            self.connection_status_label.setText("已连接")
            self.connection_status_label.setStyleSheet("color: green; font-weight: bold;")
            self.measurement_status_label.setText("待机")
        else:
            self.connection_status_label.setText("未连接")
            self.connection_status_label.setStyleSheet("color: red; font-weight: bold;")
            self.measurement_status_label.setText("未连接")

        # 更新各面板状态
        self.connection_panel.set_connected(connected)
        self.parameter_panel.set_connected(connected)
        self.measurement_panel.set_connected(connected)
        if not connected:
            self.measurement_panel.measurement_completed()
    
    def on_parameter_changed(self, param_type, value):
        """处理参数变化"""
        if self.serial_manager.is_connected():
            # 发送参数设置命令
            command = self.parameter_panel.get_command(param_type, value)
            if command:
                if self.serial_manager.send_command(command):
                    self.status_bar.showMessage(f"发送命令: {command}", 3000)

    def on_start_measurement(self):
        """开始测量"""
        if self.serial_manager.is_connected():
            # 获取参数并发送开始命令
            params = self.parameter_panel.get_all_parameters()
            for command in params:
                if not self.serial_manager.send_command(command):
                    self.status_bar.showMessage(f"命令发送失败: {command}", 5000)
                    return

            if self.serial_manager.send_command("$START"):
                self.measurement_status_label.setText("启动测量...")
                self.status_bar.showMessage("已发送启动命令", 3000)

    def on_stop_measurement(self):
        """停止测量"""
        if self.serial_manager.is_connected():
            if self.serial_manager.send_command("$STOP"):
                self.measurement_status_label.setText("停止命令已发送")
                self.status_bar.showMessage("已发送停止命令", 3000)

    def on_pause_measurement(self):
        """暂停测量"""
        if self.serial_manager.is_connected():
            if self.serial_manager.send_command("$PAUSE"):
                self.measurement_status_label.setText("暂停命令已发送")
                self.status_bar.showMessage("已发送暂停命令", 3000)

    def on_resume_measurement(self):
        """恢复测量"""
        if self.serial_manager.is_connected():
            if self.serial_manager.send_command("$RESUME"):
                self.measurement_status_label.setText("恢复命令已发送")
                self.status_bar.showMessage("已发送恢复命令", 3000)

    def on_data_received(self, data):
        """处理接收到的数据"""
        data = data.strip()

        if data.startswith("$DATA,"):
            try:
                parts = data.split(',')
                if len(parts) >= 4:
                    voltage = float(parts[1])
                    current = float(parts[2])
                    timestamp = int(parts[3])

                    # 发射数据信号
                    self.measurement_data_received.emit(voltage, current, timestamp)

                    # 更新数据计数
                    count = self.data_visualization.get_data_count()
                    self.data_count_label.setText(f"数据点: {count}")
                    self.measurement_panel.update_data_count(count)
                    self.measurement_panel.update_current_values(voltage, current)
            except (ValueError, IndexError) as e:
                pass  # 忽略解析错误
        elif data.startswith("$ACK"):
            parts = data.split(',')
            command = parts[1] if len(parts) > 1 else ""
            payload = ','.join(parts[1:]) if len(parts) > 1 else ""
            if payload:
                self.status_bar.showMessage(f"ACK: {payload}", 3000)
            else:
                self.status_bar.showMessage("收到设备确认", 3000)

            if command == "START":
                self.measurement_status_label.setText("测量中")
            elif command == "STOP":
                self.measurement_status_label.setText("待机")
                self.measurement_panel.measurement_completed()
            elif command == "PAUSE":
                self.measurement_status_label.setText("已暂停")
            elif command == "RESUME":
                self.measurement_status_label.setText("测量中")
        elif data.startswith("$INFO"):
            self.status_bar.showMessage(data, 4000)

    def on_error_occurred(self, error_msg):
        """处理错误"""
        QMessageBox.warning(self, "通信错误", error_msg)
    
    def update_status(self):
        """更新状态"""
        # 这里可以添加定期状态更新逻辑
        pass
    
    def export_data(self):
        """导出数据"""
        self.data_visualization.export_data()
    
    def clear_data(self):
        """清除数据"""
        reply = QMessageBox.question(self, "确认", "确定要清除所有数据吗？",
                                   QMessageBox.Yes | QMessageBox.No,
                                   QMessageBox.No)
        if reply == QMessageBox.Yes:
            self.data_visualization.clear_data()
            self.data_count_label.setText("数据点: 0")
    
    def show_about(self):
        """显示关于对话框"""
        QMessageBox.about(self, "关于", 
                         "循环伏安法上位机 v1.0\n\n"
                         "基于PyQt5开发的循环伏安法测量系统上位机软件\n"
                         "支持ADuCM355电化学传感器平台\n\n"
                         "© 2024 ADI")
    
    def closeEvent(self, event):
        """关闭事件"""
        # 断开连接
        if self.serial_manager.is_connected():
            self.serial_manager.disconnect()
        
        event.accept()