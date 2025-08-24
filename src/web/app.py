#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
工业设备数据模拟系统 - Web界面

@file app.py
@brief Flask Web应用主文件
@details 提供工业设备数据模拟系统的Web界面和API接口
@author 工业设备数据模拟系统开发团队
@date 2025
@version 1.0.0

主要功能：
- 提供Web仪表板界面
- 设备数据查询和展示
- 系统状态监控
- 数据分析和统计
- RESTful API接口
"""

from flask import Flask, render_template, jsonify, request
from flask_cors import CORS
import json
import sqlite3
import os
from datetime import datetime, timedelta
import pandas as pd

# 创建Flask应用实例
app = Flask(__name__)
# 启用CORS支持，允许跨域请求
CORS(app)

# ==================== 配置常量 ====================

# 配置文件路径
CONFIG_FILE = '../../config/config.json'
# 数据库文件路径
DATABASE_PATH = '../../data/plc_data.db'

# ==================== 工具函数 ====================

def load_config():
    """
    加载系统配置文件
    
    @return dict 配置信息字典，如果文件不存在则返回空字典
    
    配置文件包含：
    - 系统基本信息
    - 数据库配置
    - Modbus设备配置
    - Socket设备配置
    - Web服务配置
    """
    try:
        with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        # 配置文件不存在时返回空字典
        return {}

def get_db_connection():
    """
    获取SQLite数据库连接
    
    @return sqlite3.Connection 数据库连接对象
    
    功能说明：
    1. 确保数据目录存在
    2. 创建数据库连接
    3. 设置行工厂为sqlite3.Row，支持列名访问
    """
    # 确保数据目录存在
    os.makedirs(os.path.dirname(DATABASE_PATH), exist_ok=True)
    
    # 创建数据库连接
    conn = sqlite3.connect(DATABASE_PATH)
    # 设置行工厂，支持通过列名访问数据
    conn.row_factory = sqlite3.Row
    return conn

def init_database():
    """
    初始化数据库表结构
    
    功能说明：
    1. 创建设备数据表（device_data）
    2. 创建设备状态表（device_status）
    
    表结构说明：
    - device_data: 存储设备产生的数据点
    - device_status: 存储设备状态变化记录
    """
    conn = get_db_connection()
    cursor = conn.cursor()
    
    # 第一步：创建设备数据表
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS device_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 主键ID
            timestamp TEXT NOT NULL,               -- 时间戳
            device_name TEXT NOT NULL,             -- 设备名称
            data_type TEXT NOT NULL,               -- 数据类型
            value REAL NOT NULL,                   -- 数据值
            unit TEXT,                             -- 数据单位
            source TEXT                            -- 数据源
        )
    ''')
    
    # 第二步：创建设备状态表
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS device_status (
            id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 主键ID
            timestamp TEXT NOT NULL,               -- 时间戳
            device_name TEXT NOT NULL,             -- 设备名称
            status TEXT NOT NULL                   -- 设备状态
        )
    ''')
    
    # 提交事务并关闭连接
    conn.commit()
    conn.close()

# ==================== 页面路由 ====================

@app.route('/')
def index():
    """
    主页路由
    
    @return str 渲染后的HTML页面
    
    功能说明：
    返回系统的主仪表板页面，包含设备状态、数据统计等信息
    """
    return render_template('index.html')

# ==================== API接口 ====================

@app.route('/api/devices')
def get_devices():
    """
    获取设备列表API
    
    @return JSON 设备列表信息
    
    返回数据格式：
    [
        {
            "name": "设备名称",
            "type": "设备类型",
            "address": "设备地址",
            "port": "设备端口",
            "enabled": true/false
        }
    ]
    
    功能说明：
    1. 读取配置文件中的设备信息
    2. 根据配置生成设备列表
    3. 支持Modbus和Socket两种设备类型
    """
    config = load_config()
    devices = []
    
    # 检查Modbus设备是否启用
    if config.get('modbus', {}).get('enabled', False):
        devices.append({
            'name': '端子定位检测机',           # 设备名称
            'type': '定位检测',                 # 设备类型
            'address': config['modbus']['host'], # 设备地址
            'port': config['modbus']['port'],    # 设备端口
            'enabled': True                      # 启用状态
        })
    
    # 检查Socket设备是否启用
    if config.get('socket', {}).get('enabled', False):
        devices.append({
            'name': 'D-1夹具机器手臂',           # 设备名称
            'type': '夹具',                      # 设备类型
            'address': config['socket']['host'], # 设备地址
            'port': config['socket']['port'],    # 设备端口
            'enabled': True                      # 启用状态
        })
    
    return jsonify(devices)

@app.route('/api/data/<device_name>')
def get_device_data(device_name):
    """获取设备数据"""
    try:
        hours = request.args.get('hours', 24, type=int)
        data_type = request.args.get('type', 'all')
        limit = request.args.get('limit', 1000, type=int)
        
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # 构建查询条件
        where_clause = "WHERE device_name = ?"
        params = [device_name]
        
        if data_type != 'all':
            where_clause += " AND data_type = ?"
            params.append(data_type)
        
        # 查询指定时间范围内的数据
        time_limit = datetime.now() - timedelta(hours=hours)
        where_clause += " AND timestamp >= ?"
        params.append(time_limit.isoformat())
        
        query = f'''
            SELECT timestamp, data_type, value, unit, source
            FROM device_data
            {where_clause}
            ORDER BY timestamp DESC
            LIMIT ?
        '''
        params.append(limit)
        
        cursor.execute(query, params)
        rows = cursor.fetchall()
        
        data = []
        for row in rows:
            data.append({
                'timestamp': row['timestamp'],
                'type': row['data_type'],
                'value': row['value'],
                'unit': row['unit'],
                'source': row['source']
            })
        
        conn.close()
        return jsonify(data)
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/realtime/<device_name>')
def get_realtime_data(device_name):
    """获取设备实时数据"""
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # 设备名称到传感器类型的映射
        device_sensor_mapping = {
            '端子定位检测机': ['temperature', 'pressure', 'flow', 'status'],  # Modbus设备
            'D-1夹具机器手臂': ['custom', 'status']  # Socket设备
        }
        
        # 获取设备对应的传感器类型
        sensor_types = device_sensor_mapping.get(device_name, [])
        if not sensor_types:
            return jsonify({'error': f'未知设备: {device_name}'}), 404
        
        # 获取最新的数据点（按传感器类型查询）
        latest_values = {}
        for sensor_type in sensor_types:
            # 查询该传感器类型的最新数据
            cursor.execute('''
                SELECT timestamp, data_type, value, unit, source
                FROM device_data
                WHERE data_type = ?
                ORDER BY timestamp DESC
                LIMIT 1
            ''', [sensor_type])
            
            row = cursor.fetchone()
            if row:
                latest_values[sensor_type] = {
                    'value': row['value'],
                    'unit': row['unit'],
                    'timestamp': row['timestamp']
                }
        
        # 获取最近的数据点用于图表显示
        recent_data = {}
        for sensor_type in sensor_types:
            cursor.execute('''
                SELECT timestamp, data_type, value, unit, source
                FROM device_data
                WHERE data_type = ?
                ORDER BY timestamp DESC
                LIMIT 10
            ''', [sensor_type])
            
            rows = cursor.fetchall()
            recent_data[sensor_type] = []
            for row in rows:
                recent_data[sensor_type].append({
                    'timestamp': row['timestamp'],
                    'value': row['value'],
                    'unit': row['unit']
                })
        
        conn.close()
        return jsonify({
            'device': device_name,
            'latest': latest_values,
            'recent': recent_data
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/status/<device_name>')
def get_device_status(device_name):
    """获取设备状态"""
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT status, timestamp
            FROM device_status
            WHERE device_name = ?
            ORDER BY timestamp DESC
            LIMIT 1
        ''', [device_name])
        
        row = cursor.fetchone()
        conn.close()
        
        if row:
            return jsonify({
                'status': row['status'],
                'timestamp': row['timestamp']
            })
        else:
            return jsonify({'status': 'unknown', 'timestamp': None})
            
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/analysis/<device_name>')
def get_analysis(device_name):
    """获取数据分析结果"""
    try:
        hours = request.args.get('hours', 24, type=int)
        
        conn = get_db_connection()
        
        # 读取数据到DataFrame
        query = '''
            SELECT timestamp, data_type, value
            FROM device_data
            WHERE device_name = ? AND timestamp >= ?
            ORDER BY timestamp
        '''
        
        time_limit = datetime.now() - timedelta(hours=hours)
        df = pd.read_sql_query(query, conn, params=[device_name, time_limit.isoformat()])
        conn.close()
        
        if df.empty:
            return jsonify({'error': '没有数据'})
        
        # 转换时间戳
        df['timestamp'] = pd.to_datetime(df['timestamp'])
        
        # 按数据类型分组分析
        analysis = {}
        for data_type in df['data_type'].unique():
            type_data = df[df['data_type'] == data_type]
            
            analysis[data_type] = {
                'count': len(type_data),
                'min': float(type_data['value'].min()),
                'max': float(type_data['value'].max()),
                'mean': float(type_data['value'].mean()),
                'std': float(type_data['value'].std()),
                'latest': float(type_data['value'].iloc[-1]) if not type_data.empty else 0
            }
        
        return jsonify(analysis)
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/config')
def get_config():
    """获取系统配置"""
    config = load_config()
    return jsonify(config)

@app.route('/api/stats')
def get_system_stats():
    """获取系统统计信息"""
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # 获取总数据点数量
        cursor.execute('SELECT COUNT(*) FROM device_data')
        total_data_points = cursor.fetchone()[0]
        
        # 获取设备数量
        cursor.execute('SELECT COUNT(DISTINCT device_name) FROM device_data')
        device_count = cursor.fetchone()[0]
        
        # 获取数据类型统计 - 改进为更有意义的信息
        cursor.execute('''
            SELECT 
                data_type,
                COUNT(*) as count,
                AVG(value) as avg_value,
                MIN(value) as min_value,
                MAX(value) as max_value,
                (SELECT value FROM device_data 
                 WHERE data_type = d.data_type 
                 ORDER BY timestamp DESC LIMIT 1) as latest_value
            FROM device_data d
            GROUP BY data_type
        ''')
        
        # 数据类型名称映射（中文 -> 英文）
        data_type_mapping = {
            '流量': 'flow',
            '压力': 'pressure', 
            '温度': 'temperature',
            '状态': 'status'
        }
        
        data_type_stats = {}
        for row in cursor.fetchall():
            data_type = row[0]
            # 如果是中文名称，映射到英文名称
            if data_type in data_type_mapping:
                data_type = data_type_mapping[data_type]
            
            # 如果该数据类型已存在，合并统计数据
            if data_type in data_type_stats:
                existing = data_type_stats[data_type]
                # 合并计数
                existing['count'] += row[1]
                # 重新计算平均值（加权平均）
                total_count = existing['count']
                existing['avg_value'] = round(
                    (existing['avg_value'] * (total_count - row[1]) + row[2] * row[1]) / total_count, 2
                )
                # 更新最小值和最大值
                existing['min_value'] = min(existing['min_value'], row[3])
                existing['max_value'] = max(existing['max_value'], row[4])
                # 使用最新的值
                existing['latest_value'] = row[5]
            else:
                # 新的数据类型，直接添加
                data_type_stats[data_type] = {
                    'count': row[1],
                    'avg_value': round(row[2], 2) if row[2] else 0,
                    'min_value': round(row[3], 2) if row[3] else 0,
                    'max_value': round(row[4], 2) if row[4] else 0,
                    'latest_value': round(row[5], 2) if row[5] else 0
                }
        
        # 获取最新数据时间
        cursor.execute('''
            SELECT MAX(timestamp) as latest_timestamp
            FROM device_data
        ''')
        latest_timestamp = cursor.fetchone()[0]
        
        # 获取设备状态统计
        cursor.execute('''
            SELECT device_name, status, COUNT(*) as count
            FROM device_status
            WHERE timestamp >= datetime('now', '-1 hour')
            GROUP BY device_name, status
        ''')
        status_stats = {}
        for row in cursor.fetchall():
            device = row[0]
            status = row[1]
            count = row[2]
            if device not in status_stats:
                status_stats[device] = {}
            status_stats[device][status] = count
        
        conn.close()
        
        return jsonify({
            'total_data_points': total_data_points,
            'device_count': device_count,
            'data_type_stats': data_type_stats,
            'latest_timestamp': latest_timestamp,
            'status_stats': status_stats,
            'system_uptime': '运行中'
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    # 初始化数据库
    init_database()
    
    # 启动应用
    config = load_config()
    host = config.get('web', {}).get('host', '0.0.0.0')  # 允许外部访问
    port = config.get('web', {}).get('port', 5000)
    debug = config.get('web', {}).get('debug', True)
    
    print(f"启动Web服务: http://{host}:{port}")
    print(f"本地访问: http://127.0.0.1:{port}")
    print(f"网络访问: http://0.0.0.0:{port}")
    app.run(host=host, port=port, debug=debug)
