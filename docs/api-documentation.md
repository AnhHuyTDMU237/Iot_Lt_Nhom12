# API Documentation

Base path: `/api/v1`

## Authentication

### 1. Login
- **URL:** `/auth/login`
- **Method:** `POST`
- **Body:**
```json
{
  "username": "admin",
  "password": "Admin@123"
}
```
- **Response:** (200 OK)
```json
{
  "accessToken": "eyJhb...",
  "tokenType": "Bearer",
  "expiresIn": 86400,
  "role": "ADMIN",
  "username": "admin",
  "fullName": "Administrator"
}
```

## Devices

### 2. Get All Devices
- **URL:** `/devices`
- **Method:** `GET`
- **Headers:** `Authorization: Bearer <token>`
- **Response:** List of devices

### 3. Get Device By ID
- **URL:** `/devices/{deviceId}`
- **Method:** `GET`
- **Headers:** `Authorization: Bearer <token>`
- **Response:** Device details

## Telemetry

### 4. Get Latest Telemetry
- **URL:** `/devices/{deviceId}/telemetry/latest`
- **Method:** `GET`
- **Headers:** `Authorization: Bearer <token>`

### 5. Get Telemetry History (Paginated)
- **URL:** `/devices/{deviceId}/telemetry?page=0&size=20&from=ISO8601&to=ISO8601`
- **Method:** `GET`
- **Headers:** `Authorization: Bearer <token>`

## Commands

### 6. Send Command
- **URL:** `/devices/{deviceId}/commands`
- **Method:** `POST`
- **Headers:** `Authorization: Bearer <token>`
- **Body:**
```json
{
  "action": "LED_ON"
}
```
- **Response:** Command status (PENDING/SENT) with `commandId`.

### 7. Get Command History
- **URL:** `/devices/{deviceId}/commands?page=0&size=10`
- **Method:** `GET`
- **Headers:** `Authorization: Bearer <token>`

Lưu ý: API dùng chuẩn Swagger/OpenAPI, có thể xem trực tiếp tại UI khi Backend chạy: `http://localhost:8080/swagger-ui.html`
