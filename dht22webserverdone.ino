#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// Konfigurasi DHT22
#define DHTPIN 27           // GPIO 27 sesuai permintaan
#define DHTTYPE DHT22

// Konfigurasi WiFi
const char* ssid = "Radit";
const char* password = "rahmatgaming";

WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

// Variabel sensor
float temperature = 0.0;
float humidity = 0.0;
bool sensorError = false;
String errorMessage = "";
unsigned long lastReadTime = 0;
const long readInterval = 3000;  // Baca sensor setiap 3 detik

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("POLINDRA - TRK Instrumentasi dan Kontrol");
  Serial.println("Sistem Monitoring DHT22 ESP32");
  Serial.println("========================================");
  
  // Inisialisasi DHT22
  dht.begin();
  delay(2000);  // Beri waktu sensor untuk inisialisasi
  
  Serial.println("Menginisialisasi sensor DHT22...");
  Serial.println("Menggunakan GPIO: 27");
  
  // Baca sensor pertama kali untuk test
  float testTemp = dht.readTemperature();
  float testHum = dht.readHumidity();
  
  if (isnan(testTemp) || isnan(testHum)) {
    sensorError = true;
    errorMessage = "Gagal membaca sensor DHT22. Periksa koneksi.";
    Serial.println("ERROR: Sensor DHT22 tidak terdeteksi!");
    Serial.println("Periksa:");
    Serial.println("1. Koneksi kabel ke GPIO 27");
    Serial.println("2. Resistor pull-up 10kΩ (VCC ke DATA)");
    Serial.println("3. Power 3.3V (bukan 5V)");
  } else {
    Serial.println("SUKSES: Sensor DHT22 terdeteksi!");
    Serial.print("Pembacaan test - Suhu: ");
    Serial.print(testTemp);
    Serial.print("°C, Kelembaban: ");
    Serial.print(testHum);
    Serial.println("%");
  }
  
  // Koneksi WiFi
  Serial.print("\nMenghubungkan ke WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  int wifiTimeout = 0;
  
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
    delay(1000);
    Serial.print(".");
    wifiTimeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi TERHUBUNG!");
    Serial.print("Alamat IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nERROR: Gagal menghubungkan WiFi!");
  }
  
  // Setup server routes
  server.on("/", handle_OnConnect);
  server.on("/data", handle_Data);
  server.on("/sensor", handle_SensorDebug);
  server.on("/reset", handle_Reset);
  server.onNotFound(handle_NotFound);
  
  server.begin();
  Serial.println("Server HTTP dimulai");
  Serial.println("========================================\n");
}

void loop() {
  server.handleClient();
  
  // Baca sensor secara berkala
  unsigned long currentTime = millis();
  
  if (currentTime - lastReadTime >= readInterval) {
    lastReadTime = currentTime;
    
    // Baca data sensor
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    // Validasi pembacaan
    if (isnan(h) || isnan(t)) {
      sensorError = true;
      errorMessage = "Pembacaan sensor gagal!";
      
      Serial.print("[");
      Serial.print(currentTime / 1000);
      Serial.println(" detik] ERROR: Pembacaan sensor gagal");
      
      // Reset sensor dengan delay
      digitalWrite(DHTPIN, LOW);
      pinMode(DHTPIN, INPUT);
      delay(250);
    } else {
      sensorError = false;
      errorMessage = "";
      humidity = h;
      temperature = t;
      
      Serial.print("[");
      Serial.print(currentTime / 1000);
      Serial.print(" detik] Suhu: ");
      Serial.print(t);
      Serial.print("°C, Kelembaban: ");
      Serial.print(h);
      Serial.println("%");
    }
  }
}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML());
}

void handle_Data() {
  String json = "{";
  
  if (sensorError) {
    json += "\"error\":true,";
    json += "\"message\":\"" + errorMessage + "\",";
    json += "\"temperature\":0,";
    json += "\"humidity\":0";
  } else {
    json += "\"error\":false,";
    json += "\"message\":\"OK\",";
    json += "\"temperature\":" + String(temperature) + ",";
    json += "\"humidity\":" + String(humidity);
  }
  
  json += "}";
  server.send(200, "application/json", json);
}

void handle_SensorDebug() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>Debug Sensor DHT22</title>";
  html += "<style>body{font-family:Arial;padding:20px;}</style>";
  html += "</head><body>";
  html += "<h2>Debug Sensor DHT22 - POLINDRA TRK</h2>";
  html += "<p>GPIO: 27</p>";
  html += "<p>Status Sensor: " + String(sensorError ? "ERROR" : "OK") + "</p>";
  html += "<p>Pesan: " + errorMessage + "</p>";
  html += "<p>Suhu: " + String(temperature) + "°C</p>";
  html += "<p>Kelembaban: " + String(humidity) + "%</p>";
  html += "<p><a href='/'>Kembali ke Dashboard</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handle_Reset() {
  ESP.restart();
}

void handle_NotFound() {
  server.send(404, "text/plain", "Halaman tidak ditemukan");
}

String SendHTML() {
  String ptr = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>POLINDRA - Monitoring DHT22</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
        body { background: #f5f7fa; color: #333; }
        .container { max-width: 1000px; margin: 0 auto; padding: 20px; }
        
        .header {
            background: linear-gradient(135deg, #0c2340 0%, #1a3a5f 100%);
            color: white; padding: 25px; border-radius: 15px;
            margin-bottom: 25px; text-align: center;
            box-shadow: 0 5px 15px rgba(12, 35, 64, 0.2);
        }
        .header h1 { font-size: 2.2rem; margin-bottom: 10px; }
        .header h1 i { color: #d32f2f; margin-right: 15px; }
        .subtitle { font-size: 1.1rem; opacity: 0.9; }
        
        .status-bar {
            display: flex; justify-content: space-between; background: white;
            padding: 20px; border-radius: 12px; margin-bottom: 25px;
            box-shadow: 0 3px 10px rgba(0,0,0,0.08);
            flex-wrap: wrap;
        }
        .status-item { text-align: center; padding: 10px; flex: 1; }
        .status-label { font-size: 0.9rem; color: #666; margin-bottom: 8px; }
        .status-value { font-size: 1.4rem; font-weight: bold; }
        .wifi-online { color: #2ecc71; }
        .wifi-offline { color: #e74c3c; }
        
        .data-cards {
            display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 25px; margin-bottom: 30px;
        }
        .card {
            background: white; border-radius: 15px; padding: 30px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.08);
            transition: transform 0.3s ease;
            border-top: 5px solid;
        }
        .card:hover { transform: translateY(-5px); }
        .temp-card { border-color: #e74c3c; }
        .hum-card { border-color: #3498db; }
        .card-header { display: flex; align-items: center; margin-bottom: 25px; }
        .card-icon { font-size: 2.5rem; margin-right: 20px; }
        .temp-icon { color: #e74c3c; }
        .hum-icon { color: #3498db; }
        .card-title { font-size: 1.8rem; font-weight: 600; }
        
        .reading {
            font-size: 4rem; font-weight: 700; text-align: center;
            margin: 20px 0; font-family: 'Courier New', monospace;
        }
        .reading.error { color: #e74c3c; font-size: 2.5rem; }
        .unit { font-size: 2rem; color: #7f8c8d; margin-left: 5px; }
        
        .last-update {
            text-align: center; color: #95a5a6; font-size: 0.9rem;
            margin-top: 15px; padding-top: 15px;
            border-top: 1px solid #eee;
        }
        
        .controls {
            background: white; padding: 25px; border-radius: 15px;
            margin-top: 25px; box-shadow: 0 3px 10px rgba(0,0,0,0.08);
        }
        .controls h3 { margin-bottom: 20px; color: #2c3e50; }
        .button-group { display: flex; gap: 15px; flex-wrap: wrap; }
        .btn {
            padding: 12px 25px; border: none; border-radius: 8px;
            font-weight: 600; cursor: pointer; transition: all 0.3s;
            display: flex; align-items: center; gap: 10px;
        }
        .btn-primary { background: #0c2340; color: white; }
        .btn-primary:hover { background: #1a3a5f; }
        .btn-secondary { background: #ecf0f1; color: #2c3e50; }
        .btn-secondary:hover { background: #d5dbdb; }
        .btn-danger { background: #e74c3c; color: white; }
        .btn-danger:hover { background: #c0392b; }
        
        .error-alert {
            background: #ffeaea; border-left: 5px solid #e74c3c;
            padding: 20px; border-radius: 8px; margin: 20px 0;
            display: none;
        }
        .error-alert.show { display: block; }
        
        .footer {
            text-align: center; margin-top: 40px; padding-top: 20px;
            color: #7f8c8d; font-size: 0.9rem; border-top: 1px solid #ddd;
        }
        
        @media (max-width: 768px) {
            .status-bar { flex-direction: column; gap: 15px; }
            .reading { font-size: 3rem; }
            .btn { padding: 10px 20px; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1><i class="fas fa-university"></i> POLINDRA - TRK Instrumentasi</h1>
            <p class="subtitle">Sistem Monitoring Suhu & Kelembaban DHT22 | GPIO 27</p>
        </div>
        
        <div class="status-bar">
            <div class="status-item">
                <div class="status-label">Status WiFi</div>
                <div class="status-value wifi-online" id="wifiStatus">Terhubung</div>
            </div>
            <div class="status-item">
                <div class="status-label">Status Sensor</div>
                <div class="status-value" id="sensorStatus">Aktif</div>
            </div>
            <div class="status-item">
                <div class="status-label">Alamat IP</div>
                <div class="status-value" id="ipAddress">192.168.1.100</div>
            </div>
            <div class="status-item">
                <div class="status-label">Waktu Sistem</div>
                <div class="status-value" id="systemTime">--:--:--</div>
            </div>
        </div>
        
        <div class="error-alert" id="errorAlert">
            <h3><i class="fas fa-exclamation-triangle"></i> Error Sensor DHT22</h3>
            <p id="errorMessage">Gagal membaca data sensor. Periksa koneksi hardware.</p>
            <button class="btn btn-danger" onclick="resetSensor()">
                <i class="fas fa-redo"></i> Reset Sensor
            </button>
        </div>
        
        <div class="data-cards">
            <div class="card temp-card">
                <div class="card-header">
                    <i class="fas fa-thermometer-half card-icon temp-icon"></i>
                    <div>
                        <div class="card-title">Suhu Lingkungan</div>
                        <div style="color: #666; font-size: 0.95rem;">Sensor DHT22 - GPIO 27</div>
                    </div>
                </div>
                <div class="reading" id="temperatureReading">--.--<span class="unit">°C</span></div>
                <div class="last-update">
                    <i class="far fa-clock"></i> Terakhir update: <span id="tempUpdateTime">--:--:--</span>
                </div>
            </div>
            
            <div class="card hum-card">
                <div class="card-header">
                    <i class="fas fa-tint card-icon hum-icon"></i>
                    <div>
                        <div class="card-title">Kelembaban Relatif</div>
                        <div style="color: #666; font-size: 0.95rem;">Sensor DHT22 - GPIO 27</div>
                    </div>
                </div>
                <div class="reading" id="humidityReading">--.--<span class="unit">%</span></div>
                <div class="last-update">
                    <i class="far fa-clock"></i> Terakhir update: <span id="humUpdateTime">--:--:--</span>
                </div>
            </div>
        </div>
        
        <div class="controls">
            <h3><i class="fas fa-sliders-h"></i> Kontrol Sistem</h3>
            <div class="button-group">
                <button class="btn btn-primary" onclick="refreshData()">
                    <i class="fas fa-sync-alt"></i> Refresh Data
                </button>
                <button class="btn btn-secondary" onclick="openDebug()">
                    <i class="fas fa-bug"></i> Debug Sensor
                </button>
                <button class="btn btn-secondary" onclick="toggleAutoRefresh()">
                    <i class="fas fa-clock"></i> Auto Refresh: <span id="autoRefreshStatus">ON</span>
                </button>
                <button class="btn btn-secondary" onclick="exportData()">
                    <i class="fas fa-download"></i> Export Data
                </button>
            </div>
        </div>
        
        <div class="footer">
            <p><strong>POLITEKNIK NEGERI INDRAMAYU</strong> | Program Studi Teknologi Rekayasa Instrumentasi dan Kontrol</p>
            <p>Sistem Monitoring IoT - Modul Praktikum Sensor DHT22 | GPIO 27</p>
        </div>
    </div>
    
    <script>
        let autoRefresh = true;
        let refreshInterval = 3000; // 3 detik
        let lastDataTime = null;
        
        // Format waktu
        function formatTime(date) {
            return date.toLocaleTimeString('id-ID');
        }
        
        // Update waktu sistem
        function updateSystemTime() {
            const now = new Date();
            document.getElementById('systemTime').textContent = formatTime(now);
            return now;
        }
        
        // Fetch data dari ESP32
        function fetchData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    const now = updateSystemTime();
                    lastDataTime = now;
                    
                    if (data.error) {
                        // Tampilkan error
                        document.getElementById('errorMessage').textContent = data.message;
                        document.getElementById('errorAlert').classList.add('show');
                        document.getElementById('sensorStatus').textContent = 'Error';
                        document.getElementById('sensorStatus').style.color = '#e74c3c';
                        
                        document.getElementById('temperatureReading').innerHTML = 
                            '<span class="error">Sensor Error</span>';
                        document.getElementById('humidityReading').innerHTML = 
                            '<span class="error">Sensor Error</span>';
                    } else {
                        // Tampilkan data normal
                        document.getElementById('errorAlert').classList.remove('show');
                        document.getElementById('sensorStatus').textContent = 'Aktif';
                        document.getElementById('sensorStatus').style.color = '#2ecc71';
                        
                        // Update suhu
                        document.getElementById('temperatureReading').innerHTML = 
                            data.temperature.toFixed(2) + '<span class="unit">°C</span>';
                        document.getElementById('tempUpdateTime').textContent = formatTime(now);
                        
                        // Update kelembaban
                        document.getElementById('humidityReading').innerHTML = 
                            data.humidity.toFixed(2) + '<span class="unit">%</span>';
                        document.getElementById('humUpdateTime').textContent = formatTime(now);
                    }
                })
                .catch(error => {
                    console.error('Error fetching data:', error);
                    document.getElementById('errorMessage').textContent = 
                        'Gagal terhubung ke server ESP32. Periksa koneksi jaringan.';
                    document.getElementById('errorAlert').classList.add('show');
                    document.getElementById('sensorStatus').textContent = 'Offline';
                    document.getElementById('sensorStatus').style.color = '#e74c3c';
                });
        }
        
        // Manual refresh
        function refreshData() {
            fetchData();
            showNotification('Data diperbarui');
        }
        
        // Reset sensor via ESP32
        function resetSensor() {
            if (confirm('Reset sensor DHT22? Sistem akan restart sebentar.')) {
                fetch('/reset')
                    .then(() => {
                        showNotification('ESP32 sedang restart...');
                        setTimeout(() => {
                            location.reload();
                        }, 5000);
                    })
                    .catch(err => {
                        showNotification('Gagal reset sensor');
                    });
            }
        }
        
        // Toggle auto refresh
        function toggleAutoRefresh() {
            autoRefresh = !autoRefresh;
            const statusElem = document.getElementById('autoRefreshStatus');
            statusElem.textContent = autoRefresh ? 'ON' : 'OFF';
            showNotification(`Auto refresh ${autoRefresh ? 'diaktifkan' : 'dinonaktifkan'}`);
        }
        
        // Export data (simulasi)
        function exportData() {
            showNotification('Fitur export data dalam pengembangan');
        }
        
        // Open debug page
        function openDebug() {
            window.open('/sensor', '_blank');
        }
        
        // Show notification
        function showNotification(message) {
            // Buat elemen notifikasi sementara
            const notification = document.createElement('div');
            notification.style.cssText = `
                position: fixed; top: 20px; right: 20px;
                background: #0c2340; color: white; padding: 15px 25px;
                border-radius: 8px; box-shadow: 0 5px 15px rgba(0,0,0,0.2);
                z-index: 1000; font-weight: 500;
                animation: slideIn 0.3s ease;
            `;
            notification.textContent = message;
            document.body.appendChild(notification);
            
            setTimeout(() => {
                notification.style.animation = 'slideOut 0.3s ease';
                setTimeout(() => notification.remove(), 300);
            }, 3000);
        }
        
        // Inisialisasi saat halaman load
        document.addEventListener('DOMContentLoaded', function() {
            // Update alamat IP (dari ESP32)
            document.getElementById('ipAddress').textContent = 
                window.location.hostname || '192.168.4.1';
            
            // Setup auto refresh
            setInterval(() => {
                if (autoRefresh) {
                    fetchData();
                }
            }, refreshInterval);
            
            // Update waktu setiap detik
            setInterval(updateSystemTime, 1000);
            
            // Fetch data pertama kali
            setTimeout(fetchData, 1000);
            
            // Tambahkan style untuk animasi
            const style = document.createElement('style');
            style.textContent = `
                @keyframes slideIn {
                    from { transform: translateX(100%); opacity: 0; }
                    to { transform: translateX(0); opacity: 1; }
                }
                @keyframes slideOut {
                    from { transform: translateX(0); opacity: 1; }
                    to { transform: translateX(100%); opacity: 0; }
                }
            `;
            document.head.appendChild(style);
        });
    </script>
</body>
</html>
)rawliteral";
  return ptr;
}