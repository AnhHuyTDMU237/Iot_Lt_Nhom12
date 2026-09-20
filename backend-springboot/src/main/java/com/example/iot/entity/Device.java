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
@Table(name = "devices")
public class Device {
    @Id
    private UUID id;

    @Column(name = "device_id", unique = true, nullable = false)
    private String deviceId;

    private String name;
    private String type;
    
    private String status; // ONLINE or OFFLINE

    @Column(name = "led_state")
    private boolean ledState;

    @Column(name = "last_seen_at")
    private ZonedDateTime lastSeenAt;

    @Column(name = "created_at")
    private ZonedDateTime createdAt;

    @Column(name = "updated_at")
    private ZonedDateTime updatedAt;
}
