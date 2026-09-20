CREATE TABLE roles (
    id UUID PRIMARY KEY,
    name VARCHAR(50) NOT NULL UNIQUE
);

CREATE TABLE users (
    id UUID PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    full_name VARCHAR(100),
    enabled BOOLEAN NOT NULL DEFAULT TRUE,
    role_id UUID NOT NULL,
    CONSTRAINT fk_users_roles FOREIGN KEY (role_id) REFERENCES roles(id)
);

CREATE TABLE devices (
    id UUID PRIMARY KEY,
    device_id VARCHAR(100) NOT NULL UNIQUE,
    name VARCHAR(255),
    type VARCHAR(100),
    status VARCHAR(50) NOT NULL DEFAULT 'OFFLINE',
    led_state BOOLEAN NOT NULL DEFAULT FALSE,
    last_seen_at TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE telemetry (
    id UUID PRIMARY KEY,
    device_id VARCHAR(100) NOT NULL,
    temperature DOUBLE PRECISION,
    humidity DOUBLE PRECISION,
    led_state BOOLEAN,
    recorded_at TIMESTAMP WITH TIME ZONE NOT NULL,
    received_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_telemetry_device FOREIGN KEY (device_id) REFERENCES devices(device_id) ON DELETE CASCADE
);

CREATE INDEX idx_telemetry_device_recorded ON telemetry(device_id, recorded_at);

CREATE TABLE commands (
    id UUID PRIMARY KEY,
    device_id VARCHAR(100) NOT NULL,
    action VARCHAR(50) NOT NULL,
    payload JSONB,
    status VARCHAR(50) NOT NULL,
    created_by VARCHAR(50),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    sent_at TIMESTAMP WITH TIME ZONE,
    acknowledged_at TIMESTAMP WITH TIME ZONE,
    CONSTRAINT fk_commands_device FOREIGN KEY (device_id) REFERENCES devices(device_id) ON DELETE CASCADE
);

CREATE INDEX idx_commands_status_created ON commands(status, created_at);
