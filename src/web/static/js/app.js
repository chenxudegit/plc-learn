// 工业设备数据模拟系统 - 前端JavaScript应用

// 全局变量
let refreshInterval = 5000; // 5秒刷新间隔
let autoRefresh = true;
let chartPoints = 100;
let currentCharts = {};

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', function() {
    console.log('工业设备数据模拟系统前端初始化...');
    
    // 初始化导航
    initNavigation();
    
    // 初始化页面
    initDashboard();
    
    // 加载设备列表
    loadDevices();
    
    // 加载系统统计
    loadSystemStats();
    
    // 启动实时数据更新
    startRealtimeUpdates();
    
    // 启动自动刷新
    if (autoRefresh) {
        startAutoRefresh();
    }
    
    // 加载设置
    loadSettings();
});

// 初始化导航
function initNavigation() {
    const navLinks = document.querySelectorAll('nav a');
    const sections = document.querySelectorAll('main section');
    
    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            
            // 移除所有活动状态
            navLinks.forEach(l => l.classList.remove('active'));
            sections.forEach(s => s.classList.remove('active'));
            
            // 添加活动状态
            this.classList.add('active');
            const targetId = this.getAttribute('href').substring(1);
            document.getElementById(targetId).classList.add('active');
        });
    });
}

// 初始化仪表板
function initDashboard() {
    console.log('初始化仪表板...');
    
    // 设置当前时间
    updateCurrentTime();
    
    // 每秒更新时间
    setInterval(updateCurrentTime, 1000);
}

// 更新当前时间
function updateCurrentTime() {
    const now = new Date();
    const timeString = now.toLocaleString('zh-CN');
    document.getElementById('last-update').textContent = `最后更新: ${timeString}`;
}

// 加载设备列表
async function loadDevices() {
    try {
        const response = await fetch('/api/devices');
        const devices = await response.json();
        
        const deviceList = document.getElementById('device-list');
        deviceList.innerHTML = '';
        
        devices.forEach(device => {
            const deviceCard = createDeviceCard(device);
            deviceList.appendChild(deviceCard);
        });
        
        // 填充分析设备选择器
        const analysisDevice = document.getElementById('analysis-device');
        analysisDevice.innerHTML = '<option value="">选择设备</option>';
        devices.forEach(device => {
            const option = document.createElement('option');
            option.value = device.name;
            option.textContent = device.name;
            analysisDevice.appendChild(option);
        });
        
    } catch (error) {
        console.error('加载设备列表失败:', error);
        document.getElementById('device-list').innerHTML = '<div class="error">加载设备失败</div>';
    }
}

// 创建设备卡片
function createDeviceCard(device) {
    const card = document.createElement('div');
    card.className = 'device-card';
    card.innerHTML = `
        <div class="device-header">
            <h3>${device.name}</h3>
            <span class="device-type ${device.type}">${device.type.toUpperCase()}</span>
        </div>
        <div class="device-info">
            <div class="info-item">
                <span class="label">地址:</span>
                <span class="value">${device.address}:${device.port}</span>
            </div>
            <div class="info-item">
                <span class="label">状态:</span>
                <span class="status ${device.enabled ? 'online' : 'offline'}">
                    ${device.enabled ? '在线' : '离线'}
                </span>
            </div>
        </div>
        <div class="device-controls">
            <button onclick="toggleDevice('${device.name}')" class="btn btn-small">
                ${device.enabled ? '停止' : '启动'}
            </button>
            <button onclick="viewDeviceData('${device.name}')" class="btn btn-small btn-secondary">
                查看数据
            </button>
        </div>
    `;
    return card;
}

// 加载系统统计
async function loadSystemStats() {
    try {
        const response = await fetch('/api/stats');
        const stats = await response.json();
        
        // 更新设备状态
        updateDeviceStatus(stats);
        
        // 更新数据统计
        updateDataStats(stats);
        
        // 更新系统信息
        updateSystemInfo(stats);
        
    } catch (error) {
        console.error('加载系统统计失败:', error);
    }
}

// 更新设备状态
function updateDeviceStatus(stats) {
    const deviceStatus = document.getElementById('device-status');
    
    let statusHtml = '';
    if (stats.status_stats) {
        Object.entries(stats.status_stats).forEach(([device, statuses]) => {
            statusHtml += `<div class="status-item">
                <span class="device-name">${device}:</span>
                <span class="status-value">${Object.keys(statuses).join(', ')}</span>
            </div>`;
        });
    }
    
    deviceStatus.innerHTML = statusHtml || '<div class="no-data">暂无状态信息</div>';
}

// 更新数据统计
function updateDataStats(stats) {
    const dataStats = document.getElementById('data-stats');
    
    // 数据类型中文映射
    const dataTypeLabels = {
        'temperature': '温度',
        'pressure': '压力', 
        'flow': '流量',
        'status': '状态',
        'counter': '计数器'
    };
    
    // 数据单位映射
    const dataTypeUnits = {
        'temperature': '°C',
        'pressure': 'MPa',
        'flow': 'L/min',
        'status': '',
        'counter': ''
    };
    
    let statsHtml = '';
    if (stats.data_type_stats) {
        Object.entries(stats.data_type_stats).forEach(([type, data]) => {
            const label = dataTypeLabels[type] || type;
            const unit = dataTypeUnits[type] || '';
            const unitDisplay = unit ? ` ${unit}` : '';
            
            statsHtml += `<div class="stat-item">
                <div class="stat-header">
                    <span class="stat-label">${label}:</span>
                    <span class="stat-count">${data.count} 个数据点</span>
                </div>
                <div class="stat-details">
                    <span class="stat-detail">最新: ${data.latest_value}${unitDisplay}</span>
                    <span class="stat-detail">平均: ${data.avg_value}${unitDisplay}</span>
                    <span class="stat-detail">范围: ${data.min_value} - ${data.max_value}${unitDisplay}</span>
                </div>
            </div>`;
        });
    }
    
    dataStats.innerHTML = statsHtml || '<div class="no-data">暂无统计数据</div>';
}

// 更新系统信息
function updateSystemInfo(stats) {
    const systemInfo = document.getElementById('system-info');
    
    systemInfo.innerHTML = `
        <div class="info-item">
            <span class="label">总数据点:</span>
            <span class="value">${stats.total_data_points || 0}</span>
        </div>
        <div class="info-item">
            <span class="label">设备数量:</span>
            <span class="value">${stats.device_count || 0}</span>
        </div>
        <div class="info-item">
            <span class="label">最新数据:</span>
            <span class="value">${stats.latest_timestamp || '--'}</span>
        </div>
        <div class="info-item">
            <span class="label">系统状态:</span>
            <span class="value">${stats.system_uptime || '运行中'}</span>
        </div>
    `;
}

// 更新分析图表
async function updateAnalysis() {
    const device = document.getElementById('analysis-device').value;
    const dataType = document.getElementById('analysis-type').value;
    const timeRange = document.getElementById('analysis-time').value;
    
    if (!device) {
        document.getElementById('analysis-results').innerHTML = '请选择设备进行分析';
        return;
    }
    
    try {
        const response = await fetch(`/api/data/${encodeURIComponent(device)}?hours=${timeRange}&type=${dataType}`);
        const data = await response.json();
        
        if (data.error) {
            document.getElementById('analysis-results').innerHTML = `错误: ${data.error}`;
            return;
        }
        
        // 绘制图表
        drawAnalysisChart(data, device, dataType);
        
        // 显示分析摘要
        showAnalysisSummary(data, device, dataType);
        
    } catch (error) {
        console.error('更新分析失败:', error);
        document.getElementById('analysis-results').innerHTML = '分析失败，请重试';
    }
}

// 绘制分析图表
function drawAnalysisChart(data, device, dataType) {
    const ctx = document.getElementById('data-chart').getContext('2d');
    
    // 销毁现有图表
    if (currentCharts.analysis) {
        currentCharts.analysis.destroy();
    }
    
    // 准备数据
    const chartData = prepareChartData(data, dataType);
    
    // 创建新图表
    currentCharts.analysis = new Chart(ctx, {
        type: 'line',
        data: chartData,
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                x: {
                    type: 'time',
                    time: {
                        displayFormats: {
                            hour: 'HH:mm',
                            minute: 'HH:mm:ss'
                        }
                    },
                    title: {
                        display: true,
                        text: '时间'
                    }
                },
                y: {
                    title: {
                        display: true,
                        text: '数值'
                    }
                }
            },
            plugins: {
                title: {
                    display: true,
                    text: `${device} - ${dataType === 'all' ? '所有数据' : dataType}`
                },
                legend: {
                    display: true
                }
            }
        }
    });
}

// 准备图表数据
function prepareChartData(data, dataType) {
    if (dataType === 'all') {
        // 按数据类型分组
        const groupedData = {};
        data.forEach(item => {
            if (!groupedData[item.type]) {
                groupedData[item.type] = [];
            }
            groupedData[item.type].push({
                x: new Date(item.timestamp),
                y: item.value
            });
        });
        
        const datasets = Object.entries(groupedData).map(([type, points]) => ({
            label: type,
            data: points,
            borderColor: getRandomColor(),
            backgroundColor: 'rgba(0,0,0,0.1)',
            tension: 0.1
        }));
        
        return { datasets };
    } else {
        // 单一数据类型
        const points = data.map(item => ({
            x: new Date(item.timestamp),
            y: item.value
        }));
        
        return {
            datasets: [{
                label: dataType,
                data: points,
                borderColor: '#007bff',
                backgroundColor: 'rgba(0,123,255,0.1)',
                tension: 0.1
            }]
        };
    }
}

// 显示分析摘要
function showAnalysisSummary(data, device, dataType) {
    if (data.length === 0) {
        document.getElementById('analysis-results').innerHTML = '没有数据进行分析';
        return;
    }
    
    const values = data.map(item => item.value);
    const summary = {
        count: values.length,
        min: Math.min(...values),
        max: Math.max(...values),
        mean: values.reduce((a, b) => a + b, 0) / values.length,
        latest: values[values.length - 1]
    };
    
    document.getElementById('analysis-results').innerHTML = `
        <div class="summary-grid">
            <div class="summary-item">
                <span class="label">数据点数:</span>
                <span class="value">${summary.count}</span>
            </div>
            <div class="summary-item">
                <span class="label">最小值:</span>
                <span class="value">${summary.min.toFixed(2)}</span>
            </div>
            <div class="summary-item">
                <span class="label">最大值:</span>
                <span class="value">${summary.max.toFixed(2)}</span>
            </div>
            <div class="summary-item">
                <span class="label">平均值:</span>
                <span class="value">${summary.mean.toFixed(2)}</span>
            </div>
            <div class="summary-item">
                <span class="label">最新值:</span>
                <span class="value">${summary.latest.toFixed(2)}</span>
            </div>
        </div>
    `;
}

// 显示实时监控
function showRealtime() {
    // 切换到实时监控页面
    document.querySelector('nav a[href="#realtime"]').click();
    
    // 开始实时数据更新
    startRealtimeUpdates();
}

// 启动实时数据更新
function startRealtimeUpdates() {
    // 清除现有定时器
    if (window.realtimeTimer) {
        clearInterval(window.realtimeTimer);
    }
    
    // 立即更新一次
    updateRealtimeData();
    
    // 设置定时更新
    window.realtimeTimer = setInterval(updateRealtimeData, 2000);
}

// 更新实时数据
async function updateRealtimeData() {
    try {
        // 更新Modbus设备数据（端子定位检测机）
        const modbusResponse = await fetch('/api/realtime/端子定位检测机');
        if (modbusResponse.ok) {
            const modbusData = await modbusResponse.json();
            updateModbusRealtime(modbusData);
        }
        
        // 更新Socket设备数据（D-1夹具机器手臂）
        const socketResponse = await fetch('/api/realtime/D-1夹具机器手臂');
        if (socketResponse.ok) {
            const socketData = await socketResponse.json();
            updateSocketRealtime(socketData);
        }
        
    } catch (error) {
        console.error('更新实时数据失败:', error);
    }
}

// 更新Modbus实时数据
function updateModbusRealtime(data) {
    if (data.latest) {
        if (data.latest.temperature) {
            document.getElementById('modbus-temp').textContent = data.latest.temperature.value.toFixed(2);
        }
        if (data.latest.pressure) {
            document.getElementById('modbus-pressure').textContent = data.latest.pressure.value.toFixed(3);
        }
        if (data.latest.flow) {
            document.getElementById('modbus-flow').textContent = data.latest.flow.value.toFixed(1);
        }
        if (data.latest.status) {
            document.getElementById('modbus-status').textContent = data.latest.status.value;
        }
    }
}

// 更新Socket实时数据
function updateSocketRealtime(data) {
    if (data.latest) {
        if (data.latest.custom) {
            document.getElementById('socket-counter').textContent = data.latest.custom.value;
        }
        if (data.latest.status) {
            document.getElementById('socket-status').textContent = data.latest.status.value;
        }
        if (data.latest.custom) {
            document.getElementById('socket-time').textContent = new Date(data.latest.custom.timestamp).toLocaleTimeString();
        }
    }
}

// 刷新数据
function refreshData() {
    loadSystemStats();
    loadDevices();
    
    // 如果当前在分析页面，更新分析
    if (document.querySelector('#analysis.active')) {
        updateAnalysis();
    }
    
    // 如果当前在实时监控页面，更新实时数据
    if (document.querySelector('#realtime.active')) {
        updateRealtimeData();
    }
}

// 导出数据
function exportData() {
    // 实现数据导出功能
    alert('数据导出功能开发中...');
}

// 切换设备状态
function toggleDevice(deviceName) {
    // 实现设备开关功能
    alert(`切换设备 ${deviceName} 状态功能开发中...`);
}

// 查看设备数据
function viewDeviceData(deviceName) {
    // 切换到分析页面并选择设备
    document.querySelector('nav a[href="#analysis"]').click();
    document.getElementById('analysis-device').value = deviceName;
    updateAnalysis();
}

// 启动自动刷新
function startAutoRefresh() {
    setInterval(() => {
        if (autoRefresh) {
            refreshData();
        }
    }, refreshInterval);
}

// 加载设置
function loadSettings() {
    const savedSettings = localStorage.getItem('plc-settings');
    if (savedSettings) {
        const settings = JSON.parse(savedSettings);
        refreshInterval = settings.refreshInterval || 5000;
        autoRefresh = settings.autoRefresh !== undefined ? settings.autoRefresh : true;
        chartPoints = settings.chartPoints || 100;
        
        // 更新UI
        document.getElementById('refresh-interval').value = refreshInterval / 1000;
        document.getElementById('auto-refresh').checked = autoRefresh;
        document.getElementById('chart-points').value = chartPoints;
    }
}

// 保存设置
function saveSettings() {
    const settings = {
        refreshInterval: parseInt(document.getElementById('refresh-interval').value) * 1000,
        autoRefresh: document.getElementById('auto-refresh').checked,
        chartPoints: parseInt(document.getElementById('chart-points').value)
    };
    
    localStorage.setItem('plc-settings', JSON.stringify(settings));
    
    // 更新全局变量
    refreshInterval = settings.refreshInterval;
    autoRefresh = settings.autoRefresh;
    chartPoints = settings.chartPoints;
    
    // 重启自动刷新
    if (autoRefresh) {
        startAutoRefresh();
    }
    
    alert('设置已保存');
}

// 工具函数：生成随机颜色
function getRandomColor() {
    const colors = [
        '#007bff', '#28a745', '#dc3545', '#ffc107', '#17a2b8',
        '#6f42c1', '#fd7e14', '#20c997', '#e83e8c', '#6c757d'
    ];
    return colors[Math.floor(Math.random() * colors.length)];
}

// ==================== 调试功能 ====================

// 更新调试状态
function updateDebugStatus(status) {
    const debugStatus = document.getElementById('debug-status');
    if (debugStatus) {
        debugStatus.textContent = `状态: ${status}`;
    }
}

// 更新调试错误信息
function updateDebugError(error) {
    const debugErrors = document.getElementById('debug-errors');
    if (debugErrors) {
        debugErrors.textContent = `错误信息: ${error}`;
        debugErrors.style.color = 'red';
    }
}

// 更新调试最后更新时间
function updateDebugLastUpdate() {
    const debugLastUpdate = document.getElementById('debug-last-update');
    if (debugLastUpdate) {
        const now = new Date();
        debugLastUpdate.textContent = `最后更新: ${now.toLocaleTimeString()}`;
    }
}

// 测试实时API
async function testRealtimeAPI() {
    try {
        updateDebugStatus('正在测试实时API...');
        
        // 测试Modbus API
        const modbusResponse = await fetch('/api/realtime/端子定位检测机');
        if (modbusResponse.ok) {
            const modbusData = await modbusResponse.json();
            updateDebugStatus(`Modbus API测试成功: ${JSON.stringify(modbusData.latest, null, 2)}`);
        } else {
            updateDebugError(`Modbus API测试失败: ${modbusResponse.status}`);
        }
        
        // 测试Socket API
        const socketResponse = await fetch('/api/realtime/D-1夹具机器手臂');
        if (socketResponse.ok) {
            const socketData = await socketResponse.json();
            updateDebugStatus(`Socket API测试成功: ${JSON.stringify(socketData.latest, null, 2)}`);
        } else {
            updateDebugError(`Socket API测试失败: ${socketResponse.status}`);
        }
        
    } catch (error) {
        updateDebugError(`API测试失败: ${error.message}`);
    }
}

// 页面卸载时清理
window.addEventListener('beforeunload', function() {
    if (window.realtimeTimer) {
        clearInterval(window.realtimeTimer);
    }
});
