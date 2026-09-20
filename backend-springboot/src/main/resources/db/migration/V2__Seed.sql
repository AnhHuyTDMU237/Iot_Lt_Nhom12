-- Roles
INSERT INTO roles (id, name) VALUES ('00000000-0000-0000-0000-000000000001', 'ROLE_ADMIN');
INSERT INTO roles (id, name) VALUES ('00000000-0000-0000-0000-000000000002', 'ROLE_OPERATOR');
INSERT INTO roles (id, name) VALUES ('00000000-0000-0000-0000-000000000003', 'ROLE_VIEWER');

-- Admin / Admin@123
INSERT INTO users (id, username, password_hash, full_name, enabled, role_id) 
VALUES ('11111111-1111-1111-1111-111111111111', 'admin', '$2a$10$rdbM6Ysk7yFhmr6Q53bLbuhy4e9IfYrP8Or4QiVHHWWkZ7tjBEOEy', 'Administrator', true, '00000000-0000-0000-0000-000000000001');

-- Operator / Operator@123
INSERT INTO users (id, username, password_hash, full_name, enabled, role_id) 
VALUES ('22222222-2222-2222-2222-222222222222', 'operator', '$2a$10$txfVYIrd.WmUObqrhvIXn.g5CykRaoaZlM1jx4EbRMlnHb4OceAwa', 'System Operator', true, '00000000-0000-0000-0000-000000000002');

-- Viewer / Viewer@123
INSERT INTO users (id, username, password_hash, full_name, enabled, role_id) 
VALUES ('33333333-3333-3333-3333-333333333333', 'viewer', '$2a$10$oRNSXu.ObKNiXbO3rqTUJumwuCXlnkomsI.5k1Bu6GVrCchvv0ZSG', 'Guest Viewer', true, '00000000-0000-0000-0000-000000000003');

-- Demo Device
INSERT INTO devices (id, device_id, name, type, status, led_state)
VALUES ('aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa', 'esp32-001', 'Smart Environment Sensor 1', 'ESP32', 'OFFLINE', false);
