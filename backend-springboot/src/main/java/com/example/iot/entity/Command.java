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
@Table(name = "commands")
public class Command {
    @Id
    private UUID id;

    @Column(name = "device_id", nullable = false)
    private String deviceId;

    private String action;
    
    @org.hibernate.annotations.JdbcTypeCode(org.hibernate.type.SqlTypes.JSON)
    private String payload;
    
    private String status;

    @Column(name = "created_by")
    private String createdBy;

    @Column(name = "created_at")
    private ZonedDateTime createdAt;

    @Column(name = "sent_at")
    private ZonedDateTime sentAt;

    @Column(name = "acknowledged_at")
    private ZonedDateTime acknowledgedAt;
}
