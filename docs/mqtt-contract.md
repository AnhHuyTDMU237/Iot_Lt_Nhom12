# MQTT Contract

Tất cả thiết bị (bao gồm Device Simulator và ESP32 thật) phải tuân thủ nghiêm ngặt các MQTT topics và payload JSON định nghĩa dưới đây.

## 1. Telemetry (Thiết bị gửi dữ liệu lên)
- **Topic:** `device/{deviceId}/telemetry`
- **QoS:** 0
- **Chiều:** Thiết bị -> Broker -> Backend

**Payload:**
```json
{
  "deviceId": "esp32-001",
  "temperature": 28.6,
  "humidity": 67.2,
  "illuminance": 450.5,
  "soilMoisture": 60.1,
  "led": false,
  "timestamp": "2026-09-13T08:30:00Z"
}
```

## 2. Command (Backend gửi lệnh xuống thiết bị)
- **Topic:** `device/{deviceId}/command`
- **QoS:** 1
- **Chiều:** Backend -> Broker -> Thiết bị

**Payload:**
```json
{
  "commandId": "uuid-string-here",
  "action": "LED_ON",
  "timestamp": "2026-09-13T08:31:00Z"
}
```
*Ghi chú:* `action` có thể là `LED_ON` hoặc `LED_OFF`.

## 3. Command Acknowledgement (Thiết bị phản hồi lại lệnh)
- **Topic:** `device/{deviceId}/command/ack`
- **QoS:** 1
- **Chiều:** Thiết bị -> Broker -> Backend

**Payload:**
```json
{
  "commandId": "uuid-string-here",
  "deviceId": "esp32-001",
  "action": "LED_ON",
  "status": "ACKNOWLEDGED",
  "led": true,
  "timestamp": "2026-09-13T08:31:01Z"
}
```

## 4. Status (Trạng thái thiết bị Online/Offline)
- **Topic:** `device/{deviceId}/status`
- **QoS:** 1
- **Retained:** True (bắt buộc)
- **Chiều:** Thiết bị -> Broker -> Backend

**Payload Online (Gửi khi vừa kết nối thành công):**
```json
{
  "deviceId": "esp32-001",
  "status": "ONLINE",
  "timestamp": "2026-09-13T08:30:00Z"
}
```

**Payload Offline (Được cấu hình MQTT Last Will tại thiết bị):**
```json
{
  "deviceId": "esp32-001",
  "status": "OFFLINE",
  "timestamp": "2026-09-13T08:35:00Z"
}
```
*(Broker sẽ tự động gửi message này khi thiết bị mất kết nối đột ngột).*
