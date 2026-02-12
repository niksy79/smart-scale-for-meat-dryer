#ifndef WEB_PAGES_H
#define WEB_PAGES_H

const char* MONITOR_PAGE = R"rawliteral(
<!DOCTYPE html>
<html lang="bg">
<head>
    <title>Мониторинг на сушене</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .card {
            background-color: #f9f9f9;
            border: 1px solid #ddd;
            border-radius: 5px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .card h2 {
            margin-top: 0;
            color: #2196F3;
            border-bottom: 1px solid #eee;
            padding-bottom: 10px;
        }
        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }
        .parameter {
            margin-bottom: 15px;
        }
        .label {
            font-weight: bold;
            color: #555;
            margin-bottom: 5px;
        }
        .value {
            font-size: 1.2em;
            padding: 8px;
            background-color: #e1f5fe;
            border-radius: 4px;
        }
        .value.large {
            font-size: 2em;
            text-align: center;
            padding: 15px;
            background-color: #4CAF50;
            color: white;
        }
        .status-indicator {
            display: inline-block;
            width: 14px;
            height: 14px;
            border-radius: 50%;
            margin-right: 8px;
            vertical-align: middle;
        }
        .active {
            background-color: #4CAF50;
            box-shadow: 0 0 5px rgba(76, 175, 80, 0.5);
        }
        .inactive {
            background-color: #f44336;
            box-shadow: 0 0 5px rgba(244, 67, 54, 0.5);
        }
        .ready {
            background-color: #4CAF50;
            box-shadow: 0 0 5px rgba(76, 175, 80, 0.5);
        }
        .progress-container {
            background-color: #f0f0f0;
            border-radius: 10px;
            height: 30px;
            margin: 15px 0;
            overflow: hidden;
        }
        .progress-bar {
            height: 100%;
            background: linear-gradient(90deg, #4CAF50, #45a049);
            transition: width 0.5s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: bold;
        }
        .nav-buttons {
            display: flex;
            gap: 10px;
            margin: 20px 0;
        }
        .nav-button {
            padding: 10px 20px;
            background-color: #2196F3;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            text-align: center;
            flex-grow: 1;
        }
        .nav-button:hover {
            background-color: #0b7dda;
        }
        .refresh-controls {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-top: 20px;
            background-color: #f5f5f5;
            padding: 15px;
            border-radius: 5px;
        }
        .refresh-toggle {
            display: flex;
            align-items: center;
        }
        button {
            padding: 10px 20px;
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
            transition: background-color 0.3s;
        }
        button:hover {
            background-color: #0b7dda;
        }
        #toggle-refresh {
            margin-left: 10px;
            min-width: 60px;
        }
        .system-info {
            background-color: #f0f0f0;
            padding: 10px;
            border-radius: 4px;
            font-size: 0.9em;
            color: #666;
        }
        .warning-message {
            background-color: #fff3cd;
            border: 1px solid #ffc107;
            color: #856404;
            padding: 15px;
            border-radius: 5px;
            text-align: center;
            margin-bottom: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🥩 Мониторинг на сушене</h1>
        
        <div class="nav-buttons">
            <a href="/" class="nav-button">Монитор</a>
            <a href="/history" class="nav-button">История</a>
        </div>
        
        <div id="no-session-warning" class="warning-message" style="display: none;">
            ⚠️ Няма активна сесия на сушене
        </div>
        
        <div class="card">
            <h2>Статус на сесията</h2>
            <div class="parameter">
                <div class="label">Сесия активна:</div>
                <div class="value">
                    <span class="status-indicator" id="session-status"></span>
                    <span id="session-text">--</span>
                </div>
            </div>
        </div>
        
        <div id="session-data" style="display: none;">
            <div class="card">
                <h2>Текущи данни</h2>
                <div class="grid">
                    <div class="parameter">
                        <div class="label">Първоначално тегло:</div>
                        <div class="value" id="initial-weight">-- g</div>
                    </div>
                    <div class="parameter">
                        <div class="label">Текущо тегло:</div>
                        <div class="value" id="current-weight">-- g</div>
                    </div>
                </div>
                
                <div class="parameter">
                    <div class="label">Текуща загуба на тегло:</div>
                    <div class="value large" id="current-loss">--%</div>
                </div>
                
                <div class="parameter">
                    <div class="label">Целева загуба:</div>
                    <div class="value" id="target-loss">--%</div>
                </div>
                
                <div class="parameter">
                    <div class="label">Прогрес към целта:</div>
                    <div class="progress-container">
                        <div class="progress-bar" id="progress-bar" style="width: 0%;">0%</div>
                    </div>
                </div>
            </div>
            
            <div class="card">
                <h2>Информация за процеса</h2>
                <div class="grid">
                    <div class="parameter">
                        <div class="label">Текущ ден:</div>
                        <div class="value" id="current-day">--</div>
                    </div>
                    <div class="parameter">
                        <div class="label">Записи:</div>
                        <div class="value" id="record-count">--</div>
                    </div>
                    <div class="parameter">
                        <div class="label">Прогноза дни:</div>
                        <div class="value" id="days-remaining">--</div>
                    </div>
                    <div class="parameter">
                        <div class="label">Готов:</div>
                        <div class="value">
                            <span class="status-indicator" id="ready-status"></span>
                            <span id="ready-text">--</span>
                        </div>
                    </div>
                </div>
            </div>
        </div>
        
        <div class="refresh-controls">
            <div class="refresh-toggle">
                <span>Автоматично опресняване:</span>
                <button id="toggle-refresh">ON</button>
            </div>
            <button id="manual-refresh">Опресни сега</button>
        </div>
        
        <div class="system-info">
            <strong>Последно обновяване:</strong> <span id="last-update">--</span>
        </div>
    </div>
    
    <script>
        let autoRefresh = true;
        let refreshInterval;
        const refreshTime = 2000; // 2 секунди
        
        function updateStatus() {
            fetch('/status/data')
                .then(response => response.json())
                .then(data => {
                    console.log('Status data:', data);
                    
                    // Статус на сесията
                    const sessionStatus = document.getElementById('session-status');
                    const sessionText = document.getElementById('session-text');
                    const noSessionWarning = document.getElementById('no-session-warning');
                    const sessionData = document.getElementById('session-data');
                    
                    if (data.active) {
                        sessionStatus.className = 'status-indicator active';
                        sessionText.textContent = 'АКТИВНА';
                        noSessionWarning.style.display = 'none';
                        sessionData.style.display = 'block';
                        
                        // Тегла
                        document.getElementById('initial-weight').textContent = data.initialWeight.toFixed(1) + ' g';
                        document.getElementById('current-weight').textContent = data.currentWeight.toFixed(1) + ' g';
                        
                        // Загуба
                        document.getElementById('current-loss').textContent = data.currentLoss.toFixed(1) + '%';
                        document.getElementById('target-loss').textContent = data.targetLoss.toFixed(1) + '%';
                        
                        // Прогрес бар
                        const progress = Math.min((data.currentLoss / data.targetLoss) * 100, 100);
                        const progressBar = document.getElementById('progress-bar');
                        progressBar.style.width = progress.toFixed(0) + '%';
                        progressBar.textContent = progress.toFixed(0) + '%';
                        
                        // Информация
                        document.getElementById('current-day').textContent = 'Ден ' + data.currentDay;
                        document.getElementById('record-count').textContent = data.recordCount;
                        
                        if (data.daysRemaining >= 0) {
                            document.getElementById('days-remaining').textContent = '~' + data.daysRemaining + ' дни';
                        } else {
                            document.getElementById('days-remaining').textContent = 'Няма данни';
                        }
                        
                        // Готовност
                        const readyStatus = document.getElementById('ready-status');
                        const readyText = document.getElementById('ready-text');
                        if (data.isReady) {
                            readyStatus.className = 'status-indicator ready';
                            readyText.textContent = 'ДА ✓';
                        } else {
                            readyStatus.className = 'status-indicator inactive';
                            readyText.textContent = 'НЕ';
                        }
                    } else {
                        sessionStatus.className = 'status-indicator inactive';
                        sessionText.textContent = 'НЕАКТИВНА';
                        noSessionWarning.style.display = 'block';
                        sessionData.style.display = 'none';
                    }
                    
                    document.getElementById('last-update').textContent = new Date().toLocaleTimeString();
                })
                .catch(error => {
                    console.error('Error fetching status:', error);
                    document.getElementById('last-update').textContent = 'Грешка при обновяване';
                });
        }
        
        document.getElementById('toggle-refresh').addEventListener('click', function() {
            autoRefresh = !autoRefresh;
            this.textContent = autoRefresh ? 'ON' : 'OFF';
            
            if (autoRefresh) {
                refreshInterval = setInterval(updateStatus, refreshTime);
            } else {
                clearInterval(refreshInterval);
            }
        });
        
        document.getElementById('manual-refresh').addEventListener('click', updateStatus);
        
        window.onload = function() {
            updateStatus();
            if (autoRefresh) {
                refreshInterval = setInterval(updateStatus, refreshTime);
            }
        };
    </script>
</body>
</html>
)rawliteral";

const char* HISTORY_PAGE = R"rawliteral(
<!DOCTYPE html>
<html lang="bg">
<head>
    <title>История на сушене</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            max-width: 1000px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .nav-buttons {
            display: flex;
            gap: 10px;
            margin: 20px 0;
        }
        .nav-button {
            padding: 10px 20px;
            background-color: #2196F3;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            text-align: center;
            flex-grow: 1;
        }
        .nav-button:hover {
            background-color: #0b7dda;
        }
        .warning-message {
            background-color: #fff3cd;
            border: 1px solid #ffc107;
            color: #856404;
            padding: 15px;
            border-radius: 5px;
            text-align: center;
            margin-bottom: 20px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
        }
        th {
            background-color: #2196F3;
            color: white;
            padding: 12px;
            text-align: left;
        }
        td {
            padding: 10px;
            border-bottom: 1px solid #ddd;
        }
        tr:hover {
            background-color: #f5f5f5;
        }
        .loss-positive {
            color: #4CAF50;
            font-weight: bold;
        }
        .loss-negative {
            color: #f44336;
            font-weight: bold;
        }
        .system-info {
            background-color: #f0f0f0;
            padding: 10px;
            border-radius: 4px;
            font-size: 0.9em;
            color: #666;
            margin-top: 20px;
        }
        button {
            padding: 10px 20px;
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
            margin-top: 10px;
        }
        button:hover {
            background-color: #0b7dda;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>📊 История на сушене</h1>
        
        <div class="nav-buttons">
            <a href="/" class="nav-button">Монитор</a>
            <a href="/history" class="nav-button">История</a>
        </div>
        
        <div id="no-session-warning" class="warning-message" style="display: none;">
            ⚠️ Няма активна сесия на сушене
        </div>
        
        <div id="history-data">
            <table id="history-table">
                <thead>
                    <tr>
                        <th>Ден</th>
                        <th>Тегло (g)</th>
                        <th>Общо загуба (%)</th>
                        <th>Дневна промяна (g)</th>
                    </tr>
                </thead>
                <tbody id="history-body">
                    <tr><td colspan="4" style="text-align: center;">Зареждане...</td></tr>
                </tbody>
            </table>
        </div>
        
        <button id="refresh-button">Опресни данни</button>
        
        <div class="system-info">
            <strong>Последно обновяване:</strong> <span id="last-update">--</span>
        </div>
    </div>
    
    <script>
        function updateHistory() {
            fetch('/history/data')
                .then(response => response.json())
                .then(data => {
                    console.log('History data:', data);
                    
                    const noSessionWarning = document.getElementById('no-session-warning');
                    const historyBody = document.getElementById('history-body');
                    
                    if (!data.active || data.records.length === 0) {
                        noSessionWarning.style.display = 'block';
                        historyBody.innerHTML = '<tr><td colspan="4" style="text-align: center;">Няма записи</td></tr>';
                        return;
                    }
                    
                    noSessionWarning.style.display = 'none';
                    historyBody.innerHTML = '';
                    
                    data.records.forEach(record => {
                        const row = document.createElement('tr');
                        
                        const dayCell = document.createElement('td');
                        dayCell.textContent = 'Ден ' + record.day;
                        row.appendChild(dayCell);
                        
                        const weightCell = document.createElement('td');
                        weightCell.textContent = record.weight.toFixed(1) + ' g';
                        row.appendChild(weightCell);
                        
                        const lossCell = document.createElement('td');
                        lossCell.textContent = record.loss.toFixed(1) + '%';
                        lossCell.className = record.loss > 0 ? 'loss-positive' : 'loss-negative';
                        row.appendChild(lossCell);
                        
                        const changeCell = document.createElement('td');
                        if (record.change > 0) {
                            changeCell.textContent = '-' + record.change.toFixed(1) + ' g';
                            changeCell.className = 'loss-positive';
                        } else if (record.change < 0) {
                            changeCell.textContent = '+' + Math.abs(record.change).toFixed(1) + ' g';
                            changeCell.className = 'loss-negative';
                        } else {
                            changeCell.textContent = '0 g';
                        }
                        row.appendChild(changeCell);
                        
                        historyBody.appendChild(row);
                    });
                    
                    document.getElementById('last-update').textContent = new Date().toLocaleTimeString();
                })
                .catch(error => {
                    console.error('Error fetching history:', error);
                    document.getElementById('last-update').textContent = 'Грешка при обновяване';
                });
        }
        
        document.getElementById('refresh-button').addEventListener('click', updateHistory);
        
        window.onload = updateHistory;
    </script>
</body>
</html>
)rawliteral";

#endif