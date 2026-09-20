package com.example.iot.entity;

import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.Table;
import jakarta.persistence.Column;
import lombok.Data;
import java.time.ZonedDateTime;
import java.util.UUID;

@Data
@Entity
@Table(name = "telemetry")
public class Telemetry {
    @Id
    private UUID id;

    @Column(name = "device_id", nullable = false)
    private String deviceId;

    private Double temperature;
    private Double humidity;
    
    private Double illuminance;
    
    @Column(name = "soil_moisture")
    private Double soilMoisture;

    @Column(name = "led_state")
    private Boolean ledState;

    @Column(name = "recorded_at", nullable = false)
    private ZonedDateTime recordedAt;

    @Column(name = "received_at")
    private ZonedDateTime receivedAt;
}
