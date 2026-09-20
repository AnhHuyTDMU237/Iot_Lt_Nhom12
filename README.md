# IoT Smart Environment Monitoring & Control

Dự án mẫu phục vụ giảng dạy môn **Chuyên đề Internet of Things**. 
Hệ thống giúp sinh viên hiểu rõ kiến trúc IoT End-to-End: từ thiết bị (ESP32/Simulator), MQTT Broker (EMQX), Backend (Spring Boot), Cơ sở dữ liệu (PostgreSQL) cho đến Frontend (ReactJS/Flutter).

## 1. Sơ đồ kiến trúc (Architecture)
Xem chi tiết và biểu đồ thành phần tại: [docs/architecture.md](docs/architecture.md)

Cơ chế giao tiếp qua MQTT: [docs/mqtt-contract.md](docs/mqtt-contract.md)
Cấu trúc Database: [docs/database-schema.md](docs/database-schema.md)

## 2. Hướng dẫn cài đặt bằng Docker Compose (Dành cho Giảng viên / Sinh viên)

Toàn bộ hệ thống đã được cấu hình sẵn trong Docker, bạn chỉ cần 1 lệnh duy nhất để chạy tất cả (Database, Broker, Backend, Web, Simulator).

### Bước 1: Yêu cầu hệ thống
- Yêu cầu cài đặt **Docker Desktop** (trên Windows/Mac) hoặc Docker Engine & Docker Compose (trên Linux).

### Bước 2: Khởi động hệ thống
Mở Terminal / Command Prompt tại thư mục chứa mã nguồn dự án:

```bash
docker compose up --build -d
```

Lệnh này sẽ tải các image cần thiết và khởi tạo 5 container:
1. `iot_postgres`: Cơ sở dữ liệu PostgreSQL (Lưu trữ user, thiết bị, lịch sử đo lường).
2. `iot_emqx`: Máy chủ EMQX MQTT Broker (Trung chuyển tin nhắn MQTT).
3. `iot_backend`: Ứng dụng Spring Boot cung cấp REST API.
4. `iot_web`: Giao diện Web Dashboard (ReactJS).
5. `iot_simulator`: Script Python đóng vai trò như 1 thiết bị ESP32 ảo để gửi dữ liệu nhiệt độ, độ ẩm liên tục.

### Bước 3: Truy cập hệ thống
Sau khi các dịch vụ báo trạng thái **Up** (bạn có thể kiểm tra bằng lệnh `docker compose ps`), hãy truy cập:

- **Web Dashboard:** [http://localhost](http://localhost)
- **API Swagger Docs (Nếu cần):** [http://localhost:8080/swagger-ui/index.html](http://localhost:8080/swagger-ui/index.html)
- **EMQX Dashboard (Quản trị MQTT):** [http://localhost:18083](http://localhost:18083) (Tài khoản: `admin` / `public`)

### Bước 4: Tài khoản đăng nhập
Database đã được tự động chèn (seed) 3 tài khoản phân quyền khác nhau:
- **Tài khoản 1 (Quản trị):** `admin` / `Admin@123`
- **Tài khoản 2 (Vận hành):** `operator` / `Operator@123` 
- **Tài khoản 3 (Khách):** `viewer` / `Viewer@123` (Tài khoản này chỉ có quyền xem, bị khóa chức năng bật/tắt LED)

---

## 3. Dừng hệ thống
Để tắt hệ thống nhưng vẫn giữ lại dữ liệu lịch sử (Database):
```bash
docker compose down
```

Để tắt và **XÓA SẠCH** dữ liệu Database (Khởi tạo lại từ đầu ở lần chạy sau):
```bash
docker compose down -v
```

## 4. Bài tập dành cho sinh viên
Mục tiêu cuối cùng là sinh viên sẽ code firmware C/C++ cho chip **ESP32** thật (đọc cảm biến DHT22, điều khiển LED thật), sau đó thay thế cho Simulator ảo. 

Chi tiết bài tập xem tại: [docs/student-assignment.md](docs/student-assignment.md)

## 5. ESP32-S3 thật và Vercel

Bản firmware đã được hoàn thiện để tự kết nối Wi-Fi mà không phụ thuộc
`protocol_examples_common`, đọc DHT22 thật, xử lý command bằng cJSON, gửi ACK,
đồng bộ thời gian và dùng MQTT Last Will.

Xem hướng dẫn build, đấu nối, kiểm thử, push GitHub và triển khai React tại:
[HUONG_DAN_ESP32_S3_VA_VERCEL.md](HUONG_DAN_ESP32_S3_VA_VERCEL.md).
