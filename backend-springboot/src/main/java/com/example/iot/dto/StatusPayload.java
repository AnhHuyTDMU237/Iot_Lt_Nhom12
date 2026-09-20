package com.example.iot.dto;

import lombok.Data;
import java.time.ZonedDateTime;

@Data
public class StatusPayload {
    private String deviceId;
    private String status;
    private ZonedDateTime timestamp;
}
