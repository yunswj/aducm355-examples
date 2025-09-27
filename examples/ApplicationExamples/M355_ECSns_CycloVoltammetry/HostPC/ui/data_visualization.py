#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
数据可视化模块
"""

import numpy as np
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QTabWidget,
                             QGroupBox, QPushButton, QCheckBox, QComboBox,
                             QLabel, QSpinBox, QFileDialog, QMessageBox,
                             QSplitter, QTableWidget, QTableWidgetItem)
from PyQt5.QtCore import Qt, pyqtSignal
import pyqtgraph as pg
from pyqtgraph import PlotWidget
import csv
import json
from datetime import datetime

class DataVisualization(QWidget):
    """数据可视化组件"""
    
    def __init__(self):
        super().__init__()
        self.data_points = []  # 存储所有数据点 [(voltage, current, timestamp), ...]
        self.current_curve = None
        self.init_ui()
        self.setup_plots()
    
    def init_ui(self):
        """初始化界面"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(5, 5, 5, 5)
        layout.setSpacing(5)
        
        # 创建分割器
        splitter = QSplitter(Qt.Vertical)
        layout.addWidget(splitter)
        
        # 上部分：图表显示
        chart_widget = self.create_chart_widget()
        splitter.addWidget(chart_widget)
        
        # 下部分：数据表格
        table_widget = self.create_table_widget()
        splitter.addWidget(table_widget)
        
        # 设置分割器比例
        splitter.setSizes([600, 200])
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 0)
    
    def create_chart_widget(self):
        """创建图表组件"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(5)
        
        # 控制面板
        control_panel = self.create_control_panel()
        layout.addWidget(control_panel)
        
        # 图表选项卡
        self.chart_tabs = QTabWidget()
        layout.addWidget(self.chart_tabs)
        
        # CV曲线图
        self.cv_plot = PlotWidget(title="循环伏安曲线")
        self.cv_plot.setLabel('left', '电流', units='A')
        self.cv_plot.setLabel('bottom', '电压', units='V')
        self.cv_plot.showGrid(x=True, y=True)
        self.cv_plot.setBackground('w')
        self.chart_tabs.addTab(self.cv_plot, "CV曲线")
        
        # 时间序列图
        self.time_plot = PlotWidget(title="时间序列")
        self.time_plot.setLabel('left', '电流', units='A')
        self.time_plot.setLabel('bottom', '时间', units='s')
        self.time_plot.showGrid(x=True, y=True)
        self.time_plot.setBackground('w')
        self.chart_tabs.addTab(self.time_plot, "时间序列")
        
        # 电压时间图
        self.voltage_time_plot = PlotWidget(title="电压-时间")
        self.voltage_time_plot.setLabel('left', '电压', units='V')
        self.voltage_time_plot.setLabel('bottom', '时间', units='s')
        self.voltage_time_plot.showGrid(x=True, y=True)
        self.voltage_time_plot.setBackground('w')
        self.chart_tabs.addTab(self.voltage_time_plot, "电压-时间")
        
        return widget
    
    def create_control_panel(self):
        """创建控制面板"""
        panel = QGroupBox("显示控制")
        layout = QHBoxLayout(panel)
        
        # 自动缩放
        self.auto_scale_check = QCheckBox("自动缩放")
        self.auto_scale_check.setChecked(True)
        self.auto_scale_check.toggled.connect(self.on_auto_scale_toggled)
        layout.addWidget(self.auto_scale_check)
        
        # 显示网格
        self.grid_check = QCheckBox("显示网格")
        self.grid_check.setChecked(True)
        self.grid_check.toggled.connect(self.on_grid_toggled)
        layout.addWidget(self.grid_check)
        
        # 数据点显示
        self.points_check = QCheckBox("显示数据点")
        self.points_check.setChecked(False)
        self.points_check.toggled.connect(self.on_points_toggled)
        layout.addWidget(self.points_check)
        
        # 线条样式
        layout.addWidget(QLabel("线条样式:"))
        self.line_style_combo = QComboBox()
        self.line_style_combo.addItems(["实线", "虚线", "点线", "点划线"])
        self.line_style_combo.currentTextChanged.connect(self.on_line_style_changed)
        layout.addWidget(self.line_style_combo)
        
        # 线条宽度
        layout.addWidget(QLabel("线条宽度:"))
        self.line_width_spin = QSpinBox()
        self.line_width_spin.setRange(1, 5)
        self.line_width_spin.setValue(2)
        self.line_width_spin.valueChanged.connect(self.on_line_width_changed)
        layout.addWidget(self.line_width_spin)
        
        # 清除数据按钮
        clear_btn = QPushButton("清除数据")
        clear_btn.clicked.connect(self.clear_data)
        layout.addWidget(clear_btn)
        
        # 导出图片按钮
        export_img_btn = QPushButton("导出图片")
        export_img_btn.clicked.connect(self.export_image)
        layout.addWidget(export_img_btn)
        
        layout.addStretch()
        
        return panel
    
    def create_table_widget(self):
        """创建数据表格组件"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        
        # 表格标题
        title_layout = QHBoxLayout()
        title_layout.addWidget(QLabel("数据表格"))
        title_layout.addStretch()
        
        # 导出数据按钮
        export_btn = QPushButton("导出数据")
        export_btn.clicked.connect(self.export_data)
        title_layout.addWidget(export_btn)
        
        layout.addLayout(title_layout)
        
        # 数据表格
        self.data_table = QTableWidget()
        self.data_table.setColumnCount(4)
        self.data_table.setHorizontalHeaderLabels(["序号", "电压 (V)", "电流 (A)", "时间戳"])
        self.data_table.setAlternatingRowColors(True)
        self.data_table.setSelectionBehavior(QTableWidget.SelectRows)
        layout.addWidget(self.data_table)
        
        return widget
    
    def setup_plots(self):
        """设置图表样式"""
        # 设置CV曲线样式
        pen = pg.mkPen(color='b', width=2)
        self.cv_curve = self.cv_plot.plot([], [], pen=pen, name='CV曲线')
        
        # 设置时间序列样式
        pen_time = pg.mkPen(color='r', width=2)
        self.time_curve = self.time_plot.plot([], [], pen=pen_time, name='电流-时间')
        
        # 设置电压时间样式
        pen_voltage = pg.mkPen(color='g', width=2)
        self.voltage_curve = self.voltage_time_plot.plot([], [], pen=pen_voltage, name='电压-时间')
    
    def add_data_point(self, voltage, current, timestamp):
        """添加数据点"""
        self.data_points.append((voltage, current, timestamp))
        
        # 更新图表
        self.update_plots()
        
        # 更新表格
        self.update_table()
        
        # 自动缩放
        if self.auto_scale_check.isChecked():
            self.auto_scale_plots()
    
    def update_plots(self):
        """更新所有图表"""
        if not self.data_points:
            return
        
        # 提取数据
        voltages = [point[0] for point in self.data_points]
        currents = [point[1] for point in self.data_points]
        timestamps = [point[2] for point in self.data_points]
        
        # 计算相对时间（秒）
        if timestamps:
            start_time = timestamps[0]
            rel_times = [(t - start_time) / 1000.0 for t in timestamps]
        else:
            rel_times = []
        
        # 更新CV曲线
        self.cv_curve.setData(voltages, currents)
        
        # 更新时间序列
        if rel_times:
            self.time_curve.setData(rel_times, currents)
            self.voltage_curve.setData(rel_times, voltages)
    
    def update_table(self):
        """更新数据表格"""
        self.data_table.setRowCount(len(self.data_points))
        
        for i, (voltage, current, timestamp) in enumerate(self.data_points):
            # 序号
            self.data_table.setItem(i, 0, QTableWidgetItem(str(i + 1)))
            
            # 电压
            self.data_table.setItem(i, 1, QTableWidgetItem(f"{voltage:.6f}"))
            
            # 电流
            self.data_table.setItem(i, 2, QTableWidgetItem(f"{current:.9f}"))
            
            # 时间戳
            dt = datetime.fromtimestamp(timestamp / 1000.0)
            time_str = dt.strftime("%H:%M:%S.%f")[:-3]
            self.data_table.setItem(i, 3, QTableWidgetItem(time_str))
        
        # 滚动到最后一行
        if self.data_points:
            self.data_table.scrollToBottom()
    
    def auto_scale_plots(self):
        """自动缩放图表"""
        self.cv_plot.autoRange()
        self.time_plot.autoRange()
        self.voltage_time_plot.autoRange()
    
    def clear_data(self):
        """清除所有数据"""
        self.data_points.clear()
        
        # 清除图表
        self.cv_curve.setData([], [])
        self.time_curve.setData([], [])
        self.voltage_curve.setData([], [])
        
        # 清除表格
        self.data_table.setRowCount(0)
    
    def get_data_count(self):
        """获取数据点数量"""
        return len(self.data_points)
    
    def export_data(self):
        """导出数据到文件"""
        if not self.data_points:
            QMessageBox.information(self, "提示", "没有数据可导出")
            return
        
        # 选择文件保存路径
        file_path, _ = QFileDialog.getSaveFileName(
            self, "导出数据", 
            f"CV_Data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv",
            "CSV文件 (*.csv);;JSON文件 (*.json);;所有文件 (*.*)"
        )
        
        if not file_path:
            return
        
        try:
            if file_path.endswith('.json'):
                self.export_to_json(file_path)
            else:
                self.export_to_csv(file_path)
            
            QMessageBox.information(self, "成功", f"数据已导出到:\n{file_path}")
            
        except Exception as e:
            QMessageBox.critical(self, "错误", f"导出数据失败:\n{str(e)}")
    
    def export_to_csv(self, file_path):
        """导出到CSV文件"""
        with open(file_path, 'w', newline='', encoding='utf-8') as csvfile:
            writer = csv.writer(csvfile)
            
            # 写入标题行
            writer.writerow(['序号', '电压(V)', '电流(A)', '时间戳', '时间'])
            
            # 写入数据
            for i, (voltage, current, timestamp) in enumerate(self.data_points):
                dt = datetime.fromtimestamp(timestamp / 1000.0)
                time_str = dt.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                writer.writerow([i + 1, voltage, current, timestamp, time_str])
    
    def export_to_json(self, file_path):
        """导出到JSON文件"""
        data = {
            'metadata': {
                'export_time': datetime.now().isoformat(),
                'data_count': len(self.data_points),
                'measurement_type': 'Cyclic Voltammetry'
            },
            'data': []
        }
        
        for i, (voltage, current, timestamp) in enumerate(self.data_points):
            data['data'].append({
                'index': i + 1,
                'voltage': voltage,
                'current': current,
                'timestamp': timestamp
            })
        
        with open(file_path, 'w', encoding='utf-8') as jsonfile:
            json.dump(data, jsonfile, indent=2, ensure_ascii=False)
    
    def export_image(self):
        """导出图片"""
        current_tab = self.chart_tabs.currentIndex()
        
        if current_tab == 0:
            plot_widget = self.cv_plot
            default_name = "CV_Curve"
        elif current_tab == 1:
            plot_widget = self.time_plot
            default_name = "Time_Series"
        else:
            plot_widget = self.voltage_time_plot
            default_name = "Voltage_Time"
        
        file_path, _ = QFileDialog.getSaveFileName(
            self, "导出图片",
            f"{default_name}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.png",
            "PNG文件 (*.png);;JPG文件 (*.jpg);;所有文件 (*.*)"
        )
        
        if file_path:
            try:
                exporter = pg.exporters.ImageExporter(plot_widget.plotItem)
                exporter.export(file_path)
                QMessageBox.information(self, "成功", f"图片已导出到:\n{file_path}")
            except Exception as e:
                QMessageBox.critical(self, "错误", f"导出图片失败:\n{str(e)}")
    
    def on_auto_scale_toggled(self, checked):
        """自动缩放切换"""
        if checked:
            self.auto_scale_plots()
    
    def on_grid_toggled(self, checked):
        """网格显示切换"""
        self.cv_plot.showGrid(x=checked, y=checked)
        self.time_plot.showGrid(x=checked, y=checked)
        self.voltage_time_plot.showGrid(x=checked, y=checked)
    
    def on_points_toggled(self, checked):
        """数据点显示切换"""
        symbol = 'o' if checked else None
        
        pen = self.cv_curve.opts['pen']
        self.cv_curve.setData(self.cv_curve.xData, self.cv_curve.yData, 
                             pen=pen, symbol=symbol, symbolSize=4)
        
        pen = self.time_curve.opts['pen']
        self.time_curve.setData(self.time_curve.xData, self.time_curve.yData,
                               pen=pen, symbol=symbol, symbolSize=4)
        
        pen = self.voltage_curve.opts['pen']
        self.voltage_curve.setData(self.voltage_curve.xData, self.voltage_curve.yData,
                                  pen=pen, symbol=symbol, symbolSize=4)
    
    def on_line_style_changed(self, style):
        """线条样式改变"""
        style_map = {
            "实线": Qt.SolidLine,
            "虚线": Qt.DashLine,
            "点线": Qt.DotLine,
            "点划线": Qt.DashDotLine
        }
        
        qt_style = style_map.get(style, Qt.SolidLine)
        width = self.line_width_spin.value()
        
        # 更新所有曲线的样式
        for curve in [self.cv_curve, self.time_curve, self.voltage_curve]:
            pen = curve.opts['pen']
            if hasattr(pen, 'color'):
                color = pen.color()
            else:
                color = 'b'
            
            new_pen = pg.mkPen(color=color, width=width, style=qt_style)
            curve.setPen(new_pen)
    
    def on_line_width_changed(self, width):
        """线条宽度改变"""
        style = self.line_style_combo.currentText()
        self.on_line_style_changed(style)