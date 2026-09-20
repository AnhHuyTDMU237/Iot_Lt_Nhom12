package com.example.iot.dto;

import lombok.Data;
import java.time.ZonedDateTime;

@Data
public class TelemetryPayload {
    private String deviceId;
    private Double temperature;
    private Double humidity;
    private Double illuminance;
    private Double soilMoisture;
    private Boolean led;
    private ZonedDateTime timestamp;
}
