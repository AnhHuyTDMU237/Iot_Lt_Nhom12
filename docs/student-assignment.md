# Bài tập dành cho sinh viên

## Mục tiêu
Thay thế Device Simulator viết bằng Python bằng một thiết bị thật (ESP32) kết nối với cảm biến DHT22 (hoặc DHT11) và một đèn LED. Hệ thống (Backend, Web, Mobile) **phải tiếp tục hoạt động bình thường mà không cần sửa code**.

## Nhiệm vụ bắt buộc
1. **Clone repository:** Tải mã nguồn về máy.
2. **Khởi chạy hệ thống:** Chạy `docker compose up -d` và đảm bảo Simulator đang đẩy dữ liệu (thấy trên Website).
3. **Tắt Simulator:** Comment phần `simulator` trong `docker-compose.yml` hoặc chạy `docker stop iot_simulator`. Quan sát trạng thái thiết bị trên website chuyển sang `OFFLINE`.
4. **Viết Firmware ESP32:**
   - Sử dụng thư mục `esp32-firmware` hoặc tạo project ESP-IDF/Arduino IDE mới.
   - ESP32 kết nối Wi-Fi.
   - Kết nối đến MQTT Broker (Địa chỉ IP máy tính đang chạy Docker).
   - Đọc dữ liệu từ DHT22.
   - Publish JSON payload lên đúng topic được quy định trong `mqtt-contract.md`.
   - Subscribe topic command và điều khiển LED trên board dựa theo lệnh nhận được.
   - Publish ACK sau khi thực thi lệnh.
   - Thiết lập Last Will Message để báo OFFLINE khi mất điện/mất Wi-Fi.
5. **Kiểm tra:**
   - Dữ liệu thật từ DHT22 hiện trên Website & Mobile.
   - Bấm nút bật/tắt LED trên Website/Mobile làm thay đổi LED trên board.
   - Rút nguồn ESP32, kiểm tra website cập nhật sang `OFFLINE`.

## Nhiệm vụ nâng cao (Chọn ít nhất 1)
- **Thêm cảm biến ánh sáng (LDR) hoặc cảm biến đất:** Gửi thêm dữ liệu lên dashboard. (Sinh viên tự update backend/frontend để hiển thị, ghi rõ trong báo cáo).
- **Thêm thiết bị chấp hành (Relay, Quạt, Còi):** Tạo command mới như `FAN_ON`, `FAN_OFF`.
- **Cảnh báo tự động:** Nếu nhiệt độ > 35°C, tự động đẩy cảnh báo (thêm rule ở backend hoặc tự xử lý ở firmware).
- **Phân tích dữ liệu:** Vẽ thêm biểu đồ thống kê trung bình theo ngày, giờ.

## Yêu cầu nộp bài
- Link GitHub repository (Public).
- Mã nguồn firmware trong thư mục riêng biệt.
- Sơ đồ đấu nối chân.
- Video quay màn hình + thiết bị thật (5-7 phút, mô tả quá trình gửi dữ liệu, nhận command, và ngắt nguồn test offline).
- File báo cáo ngắn gọn (PDF) giải thích code firmware.
