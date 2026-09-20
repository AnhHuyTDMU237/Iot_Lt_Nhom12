# Hướng dẫn chạy ESP32 S3 thật và triển khai React lên Vercel

Tài liệu này áp dụng cho project `demo_chuong_5_lt` và board ESP32-S3 dùng ESP-IDF 5.5.x. Firmware trong bản này đã bỏ phụ thuộc `protocol_examples_common`, tự kết nối Wi-Fi bằng `esp_wifi`, đọc DHT22 thật, đồng bộ thời gian SNTP, xử lý JSON bằng cJSON, gửi ACK và cấu hình Last Will.

## 1. Phạm vi triển khai

- Docker Compose trên máy tính chạy PostgreSQL, EMQX, Spring Boot và website local.
- ESP32-S3 kết nối cùng Wi-Fi với máy tính và kết nối tới EMQX qua IPv4 LAN của máy tính.
- Vercel chỉ triển khai `web-react`.
- Muốn website Vercel điều khiển thiết bị từ Internet, backend, PostgreSQL và MQTT broker cũng phải có địa chỉ công khai dùng HTTPS/TLS. Backend chạy trong Docker trên máy cá nhân không tự trở thành dịch vụ Internet.

## 2. Đấu nối phần cứng

| Linh kiện | ESP32-S3 | Ghi chú |
| --- | --- | --- |
| DHT22 VCC | 3V3 | Không dùng nguồn 12 V |
| DHT22 GND | GND | Chung mass |
| DHT22 DATA | GPIO 15 | Thêm điện trở kéo lên 4.7-10 kΩ nếu cảm biến rời chưa có sẵn |
| LED anode chân dài | GPIO 2 qua điện trở 220-330 Ω | Có thể đổi trong menuconfig |
| LED cathode chân ngắn | GND | Không bỏ điện trở hạn dòng |

Nếu board có LED tích hợp ở chân khác, đổi `LED GPIO` trong menuconfig. Không nối relay 5 V hoặc hộp 8 pin AA trực tiếp vào GPIO.

## 3. Kiểm tra hệ thống bằng simulator

Mở PowerShell tại thư mục gốc project:

```powershell
docker compose up --build -d
docker compose ps
docker compose logs -f simulator
```

Mở `http://localhost`, đăng nhập `operator / Operator@123`, kiểm tra `esp32-001` ONLINE và dữ liệu thay đổi. Thử LED ON/OFF và kiểm tra lệnh chuyển sang ACKNOWLEDGED.

Sau khi simulator hoạt động đúng, dừng riêng simulator:

```powershell
docker compose stop simulator
docker compose ps
```

Không dừng `postgres`, `emqx`, `backend` hoặc `web`.

## 4. Lấy IPv4 LAN và mở firewall

```powershell
ipconfig
```

Tìm `IPv4 Address` của card Wi-Fi đang dùng, ví dụ `192.168.1.100`. ESP32 và máy tính phải kết nối cùng mạng Wi-Fi 2.4 GHz. Không dùng `localhost` trong địa chỉ MQTT.

Mở PowerShell bằng quyền Administrator nếu Windows Firewall chặn MQTT:

```powershell
New-NetFirewallRule -DisplayName "IoT MQTT 1883" -Direction Inbound -Protocol TCP -LocalPort 1883 -Action Allow
New-NetFirewallRule -DisplayName "IoT Backend 8080" -Direction Inbound -Protocol TCP -LocalPort 8080 -Action Allow
```

## 5. Cấu hình firmware ESP32-S3

Mở ESP-IDF PowerShell hoặc terminal VS Code đã kích hoạt ESP-IDF, rồi chạy:

```powershell
cd D:\Git_Vercel\demo_chuong_5_lt\esp32-firmware
Remove-Item Env:IDF_TARGET -ErrorAction SilentlyContinue
idf.py fullclean
idf.py set-target esp32s3
idf.py menuconfig
```

Trong menuconfig mở `IoT device configuration` và nhập:

- `Wi-Fi SSID`: tên Wi-Fi 2.4 GHz.
- `Wi-Fi password`: mật khẩu Wi-Fi.
- `MQTT broker URL`: ví dụ `mqtt://192.168.1.100:1883`.
- `Device ID`: giữ `esp32-001`.
- `DHT22 DATA GPIO`: `15`.
- `LED GPIO`: `2` hoặc chân thật của board.

Nhấn `S`, xác nhận lưu rồi thoát. File `sdkconfig` có mật khẩu và đã được `.gitignore`; không ép thêm file này vào Git.

## 6. Build flash và monitor

```powershell
idf.py build
idf.py -p COM3 flash monitor
```

Thay `COM3` bằng cổng hiển thị trong Device Manager. Thoát monitor bằng `Ctrl+]`.

Log đúng thường có dạng:

```text
I WIFI: Connected, IP=192.168.1.xxx
I IOT_ESP32: System clock synchronized
I IOT_ESP32: MQTT_EVENT_CONNECTED
I IOT_ESP32: Published ONLINE status
I IOT_ESP32: Subscribed device/esp32-001/command
I IOT_ESP32: Published Telemetry: T=... H=...
```

## 7. Kiểm thử thiết bị thật

1. Xác nhận `iot_simulator` đang dừng.
2. Cấp nguồn ESP32-S3 và chờ dashboard báo ONLINE.
3. Hà hơi nhẹ gần DHT22 hoặc thay đổi nhiệt độ môi trường; chờ tối đa 5 giây.
4. Nhấn LED ON, kiểm tra LED sáng và command ACKNOWLEDGED.
5. Nhấn LED OFF, kiểm tra LED tắt và ACK.
6. Rút cáp USB đột ngột; EMQX phải phát Last Will và dashboard chuyển OFFLINE.
7. Cắm lại cáp; thiết bị phải tự kết nối và chuyển ONLINE.

## 8. Các lỗi firmware thường gặp

### Không tìm thấy protocol_examples_common.h

Bản gốc dùng component ví dụ nhưng không đóng gói component đó. Bản đã sửa không còn include hay gọi `example_connect()`. Wi-Fi nằm trong `main/wifi_manager.c`, còn `main/CMakeLists.txt` khai báo `esp_wifi`, `esp_event` và `esp_netif`.

### Target esp32 không khớp esp32s3

```powershell
Remove-Item Env:IDF_TARGET -ErrorAction SilentlyContinue
idf.py fullclean
idf.py set-target esp32s3
```

### DHT22 read failed

- Kiểm tra VCC 3V3, GND, DATA GPIO 15.
- Kiểm tra điện trở kéo lên DATA-3V3 nếu dùng cảm biến DHT22 rời 4 chân.
- Không dùng dây quá dài và không đọc nhanh hơn chu kỳ 2 giây.
- Nếu module ghi `S`, `+`, `-`, nối `S -> GPIO15`, `+ -> 3V3`, `- -> GND`.

### Wi-Fi kết nối mãi không xong

- ESP32-S3 chỉ dùng Wi-Fi 2.4 GHz; bật SSID 2.4 GHz.
- Kiểm tra lại SSID/mật khẩu trong menuconfig.
- Tránh Wi-Fi trường học, captive portal hoặc client isolation.

### MQTT_EVENT_DISCONNECTED

- Kiểm tra URL dùng IPv4 LAN của máy tính, không dùng `localhost`.
- Chạy `docker compose ps` và xác nhận `iot_emqx` đang Up.
- Mở inbound TCP 1883 trong Windows Firewall.

## 9. Đưa code lên GitHub cá nhân

Repository cá nhân dự kiến là:

```text
https://github.com/AnhHuyTDMU237/Iot_Lt_Nhom12.git
```

Kiểm tra remote hiện tại:

```powershell
git remote -v
```

Nếu `origin` còn trỏ tới repository giáo viên:

```powershell
git remote rename origin upstream
git remote add origin https://github.com/AnhHuyTDMU237/Iot_Lt_Nhom12.git
```

Nếu `origin` đã tồn tại nhưng sai URL:

```powershell
git remote set-url origin https://github.com/AnhHuyTDMU237/Iot_Lt_Nhom12.git
```

Thiết lập danh tính Git nếu gặp `Author identity unknown`:

```powershell
git config --global user.name "AnhHuyTDMU237"
git config --global user.email "EMAIL_GITHUB_CUA_BAN"
```

Commit và push:

```powershell
git status
git add .
git status
git commit -m "Complete ESP32-S3 WiFi DHT22 MQTT firmware"
git branch -M main
git push -u origin main
```

Trước khi commit, bảo đảm `esp32-firmware/sdkconfig` không xuất hiện trong danh sách staged. Không dùng `git push --force`.

## 10. Triển khai React lên Vercel

### 10.1 Kiểm tra build local

```powershell
cd web-react
npm install
npm run build
```

### 10.2 Import repository

1. Đăng nhập Vercel bằng GitHub.
2. Chọn `Add New Project` rồi import `Iot_Lt_Nhom12`.
3. Đặt `Root Directory` là `web-react`.
4. Framework Preset chọn `Vite` nếu Vercel chưa tự nhận.
5. Build Command giữ `npm run build`.
6. Output Directory giữ `dist`.
7. Nếu backend đã có URL HTTPS công khai, thêm biến môi trường:
   - Name: `VITE_API_URL`
   - Value: `https://TEN-BACKEND-CONG-KHAI/api/v1`
8. Nhấn Deploy.

File `web-react/vercel.json` đã thêm rewrite về `index.html`, vì vậy tải lại các route React không bị lỗi 404.

### 10.3 Giới hạn khi backend còn ở máy cá nhân

Nếu giữ backend ở `http://localhost:8080`, website Vercel chỉ build và hiển thị giao diện nhưng người dùng khác không thể đăng nhập hoặc lấy dữ liệu. Không đặt `VITE_API_URL=http://192.168.x.x:8080/api/v1` cho production vì đây là IP riêng và trang HTTPS có thể chặn API HTTP do mixed content.

Để Vercel hoạt động end-to-end từ Internet, cần:

- backend Spring Boot có URL HTTPS công khai và cấu hình `CORS_ALLOWED_ORIGINS` bằng domain Vercel;
- PostgreSQL công khai hoặc cùng private network với backend;
- MQTT broker công khai có xác thực, ưu tiên TLS cổng 8883;
- firmware đổi MQTT URL và thông tin xác thực tương ứng.

Với bài demo tại lớp, phương án ổn định nhất là chạy toàn bộ Docker Compose và website trên máy tính trong mạng LAN; Vercel dùng để chứng minh đã triển khai frontend.

## 11. Các tệp đã sửa

- `esp32-firmware/main/wifi_manager.c`: Wi-Fi station và tự kết nối lại.
- `esp32-firmware/main/main.c`: SNTP, MQTT, cJSON, ACK, telemetry, Last Will.
- `esp32-firmware/main/dht22.c`: đọc 40 bit dữ liệu thật và kiểm tra checksum.
- `esp32-firmware/Kconfig.projbuild`: cấu hình Wi-Fi, MQTT, device ID và GPIO.
- `esp32-firmware/main/CMakeLists.txt`: khai báo component bắt buộc.
- `esp32-firmware/.gitignore`: chặn build và `sdkconfig` chứa mật khẩu.
- `web-react/vercel.json`: hỗ trợ React SPA trên Vercel.
- `web-react/.env.example`: mẫu URL backend production.

## 12. Nguồn kỹ thuật

- ESP-IDF Wi-Fi Driver ESP32-S3 v5.5.1: https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-guides/wifi.html
- Vercel Vite: https://vercel.com/docs/frameworks/frontend/vite
- Vercel Root Directory và build settings: https://vercel.com/docs/builds/configure-a-build
- GitHub thêm source local lên repository: https://docs.github.com/en/repositories/creating-and-managing-repositories/adding-locally-hosted-code-to-github

## 13. Checklist nhanh trước khi nộp

- `iot_simulator` đã dừng khi quay thiết bị thật.
- ESP32-S3 dùng Wi-Fi 2.4 GHz và MQTT URL là IPv4 LAN, không phải `localhost`.
- Dashboard hiển thị nhiệt độ và độ ẩm DHT22 thật mỗi 5 giây.
- LED ON và LED OFF điều khiển đúng LED thật.
- Command giữ nguyên `commandId` và chuyển sang ACKNOWLEDGED.
- Rút nguồn đột ngột làm thiết bị chuyển OFFLINE; cấp nguồn lại chuyển ONLINE.
- `esp32-firmware/sdkconfig` và mật khẩu Wi-Fi không nằm trong Git.
- `origin` trỏ tới repository GitHub cá nhân của nhóm.
- Vercel đặt Root Directory là `web-react` và build thành công.
- Video có log ESP32, dashboard, mobile, thao tác LED và kiểm tra OFFLINE.
