// API端点（部署在 /kv/ 路径下）
const API_BASE = '/kv/api';
const KV_ENDPOINT = `${API_BASE}/kv`;
const STATS_ENDPOINT = `${API_BASE}/stats`;

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', function () {
    // 初始加载统计信息
    loadStats();

    // 监听回车键提交
    document.getElementById('command-input').addEventListener('keypress', function (e) {
        if (e.key === 'Enter') {
            submitCommand();
        }
    });
});

// 加载统计信息
async function loadStats() {
    try {
        const response = await fetch(STATS_ENDPOINT);
        const data = await response.json();

        if (data.status === 'OK' && data.data) {
            updateStats(data.data);
        }
    } catch (error) {
        console.error('Failed to load stats:', error);
    }
}

// 更新统计信息显示
function updateStats(stats) {
    // 更新Array统计
    if (stats.array) {
        document.getElementById('array-count').textContent = stats.array.count;
        document.getElementById('array-remaining').textContent = stats.array.remaining;
        const arrayPercent = (stats.array.count / stats.array.max) * 100;
        document.getElementById('array-progress').style.width = arrayPercent + '%';
    }

    // 更新Hash统计
    if (stats.hash) {
        document.getElementById('hash-count').textContent = stats.hash.count;
        document.getElementById('hash-remaining').textContent = stats.hash.remaining;
        const hashPercent = (stats.hash.count / stats.hash.max) * 100;
        document.getElementById('hash-progress').style.width = hashPercent + '%';
    }

    // 更新RBTree统计
    if (stats.rbtree) {
        document.getElementById('rbtree-count').textContent = stats.rbtree.count;
        document.getElementById('rbtree-remaining').textContent = stats.rbtree.remaining;
        const rbtreePercent = (stats.rbtree.count / stats.rbtree.max) * 100;
        document.getElementById('rbtree-progress').style.width = rbtreePercent + '%';
    }
}

// 提交命令
async function submitCommand() {
    const input = document.getElementById('command-input');
    const commandStr = input.value.trim();

    if (!commandStr) {
        showResponse('请输入命令', 'warning');
        return;
    }

    // 解析命令
    const parts = commandStr.split(/\s+/);
    if (parts.length < 2) {
        showResponse('命令格式错误,至少需要命令和键', 'error');
        return;
    }

    const cmd = parts[0].toUpperCase();
    const key = parts[1];
    const value = parts.slice(2).join(' ');

    // 构建请求数据
    const requestData = {
        cmd: cmd,
        key: key
    };

    // 如果有value,添加到请求中
    if (value) {
        requestData.value = value;
    }

    try {
        // 显示加载状态
        const submitBtn = document.getElementById('submit-btn');
        submitBtn.disabled = true;
        submitBtn.style.opacity = '0.5';

        // 发送POST请求
        const response = await fetch(KV_ENDPOINT, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(requestData)
        });

        const data = await response.json();

        // 恢复按钮状态
        submitBtn.disabled = false;
        submitBtn.style.opacity = '1';

        // 显示响应
        handleResponse(data, commandStr);

        // 清空输入框
        input.value = '';

        // 命令执行后刷新统计信息
        loadStats();

    } catch (error) {
        console.error('Request failed:', error);
        showResponse('请求失败: ' + error.message, 'error');

        // 恢复按钮状态
        const submitBtn = document.getElementById('submit-btn');
        submitBtn.disabled = false;
        submitBtn.style.opacity = '1';
    }
}

// 处理响应
function handleResponse(data, commandStr) {
    let message = '';
    let type = 'info';

    switch (data.status) {
        case 'OK':
            message = data.message || '操作成功';
            type = 'success';
            break;
        case 'EXIST':
            message = data.message || '键已存在';
            type = 'warning';
            break;
        case 'NO_EXIST':
            message = data.message || '键不存在';
            type = 'warning';
            break;
        case 'FULL':
            message = data.message || '存储已满';
            type = 'error';
            break;
        case 'ERROR':
            message = data.message || '操作失败';
            type = 'error';
            break;
        default:
            message = data.message || '未知状态';
            type = 'info';
    }

    // 显示响应
    showResponse(message, type);

    // 添加到历史记录
    addHistory(commandStr, message, type);
}

// 显示响应消息
function showResponse(message, type = 'info') {
    const responseArea = document.getElementById('response-area');

    const messageDiv = document.createElement('div');
    messageDiv.className = `response-message response-${type}`;
    messageDiv.textContent = message;

    // 清空之前的消息
    responseArea.innerHTML = '';
    responseArea.appendChild(messageDiv);

    // 3秒后淡出
    setTimeout(() => {
        messageDiv.style.opacity = '0';
        setTimeout(() => {
            if (messageDiv.parentNode) {
                messageDiv.parentNode.removeChild(messageDiv);
            }
        }, 300);
    }, 3000);
}

// 添加历史记录
function addHistory(command, result, type) {
    const historyList = document.getElementById('history-list');

    const historyItem = document.createElement('div');
    historyItem.className = 'history-item';

    const time = new Date().toLocaleTimeString('zh-CN');
    historyItem.innerHTML = `
        <div class="time">${time}</div>
        <div class="command">${escapeHtml(command)}</div>
        <div class="result" style="color: ${getTypeColor(type)}">${escapeHtml(result)}</div>
    `;

    // 插入到列表开头
    historyList.insertBefore(historyItem, historyList.firstChild);

    // 限制历史记录数量
    while (historyList.children.length > 50) {
        historyList.removeChild(historyList.lastChild);
    }
}

// 根据类型获取颜色
function getTypeColor(type) {
    switch (type) {
        case 'success':
            return '#28a745';
        case 'error':
            return '#dc3545';
        case 'warning':
            return '#ffc107';
        default:
            return '#17a2b8';
    }
}

// HTML转义
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// 切换帮助弹窗显示/隐藏
function toggleHelp() {
    const modal = document.getElementById('help-modal');
    if (modal.classList.contains('show')) {
        modal.classList.remove('show');
    } else {
        modal.classList.add('show');
    }
}

// 点击弹窗外部区域关闭弹窗
document.addEventListener('DOMContentLoaded', function () {
    const modal = document.getElementById('help-modal');
    if (modal) {
        modal.addEventListener('click', function (e) {
            if (e.target === modal) {
                toggleHelp();
            }
        });
    }
});

// ESC键关闭弹窗
document.addEventListener('keydown', function (e) {
    if (e.key === 'Escape') {
        const helpModal = document.getElementById('help-modal');
        if (helpModal && helpModal.classList.contains('show')) {
            toggleHelp();
        }
        const projectModal = document.getElementById('project-modal');
        if (projectModal && projectModal.classList.contains('show')) {
            toggleProjectInfo();
        }
    }
});

// 显示命令帮助（从导航菜单调用）
function showCommandHelp() {
    toggleHelp();
}

// 切换项目介绍弹窗显示/隐藏
function toggleProjectInfo() {
    const modal = document.getElementById('project-modal');
    if (modal.classList.contains('show')) {
        modal.classList.remove('show');
    } else {
        modal.classList.add('show');
    }
}

// 显示项目介绍
function showProjectInfo() {
    toggleProjectInfo();
}

// 查看简历（在当前标签页打开，支持浏览器返回按钮）
function viewResume() {
    window.location.href = '/kv/resources/resume.html';
}

// 点击弹窗外部区域关闭项目介绍弹窗
document.addEventListener('DOMContentLoaded', function () {
    const projectModal = document.getElementById('project-modal');
    if (projectModal) {
        projectModal.addEventListener('click', function (e) {
            if (e.target === projectModal) {
                toggleProjectInfo();
            }
        });
    }
});

