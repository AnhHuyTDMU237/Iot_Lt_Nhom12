# Database Schema

## Bảng `users`
- `id` (UUID, PK)
- `username` (VARCHAR, Unique)
- `password_hash` (VARCHAR)
- `full_name` (VARCHAR)
- `enabled` (BOOLEAN)

## Bảng `roles`
- `id` (UUID, PK)
- `name` (VARCHAR) (ROLE_ADMIN, ROLE_OPERATOR, ROLE_VIEWER)

## Bảng `user_roles`
- `user_id` (UUID, FK)
- `role_id` (UUID, FK)

## Bảng `devices`
- `id` (UUID, PK)
- `device_id` (VARCHAR, Unique)
- `name` (VARCHAR)
- `type` (VARCHAR)
- `status` (VARCHAR) (ONLINE / OFFLINE)
- `led_state` (BOOLEAN)
- `last_seen_at` (TIMESTAMP WITH TIME ZONE)
- `created_at` (TIMESTAMP)
- `updated_at` (TIMESTAMP)

## Bảng `telemetry`
- `id` (UUID, PK)
- `device_id` (VARCHAR, FK or index for search)
- `temperature` (DOUBLE)
- `humidity` (DOUBLE)
- `illuminance` (DOUBLE)
- `soil_moisture` (DOUBLE)
- `led_state` (BOOLEAN)
- `recorded_at` (TIMESTAMP WITH TIME ZONE, from device payload)
- `received_at` (TIMESTAMP WITH TIME ZONE, server time)
- *Index on (device_id, recorded_at)*

## Bảng `commands`
- `id` (UUID, PK, dùng làm commandId)
- `device_id` (VARCHAR)
- `action` (VARCHAR)
- `payload` (JSONB / VARCHAR)
- `status` (VARCHAR) (PENDING, SENT, ACKNOWLEDGED, FAILED, TIMEOUT)
- `created_by` (VARCHAR, username của người gửi)
- `created_at` (TIMESTAMP)
- `sent_at` (TIMESTAMP)
- `acknowledged_at` (TIMESTAMP)
- *Index on (status, created_at)*
