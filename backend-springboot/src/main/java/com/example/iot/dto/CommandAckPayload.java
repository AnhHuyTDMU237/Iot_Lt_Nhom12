package com.example.iot.dto;

import lombok.Data;
import java.time.ZonedDateTime;
import java.util.UUID;

@Data
public class CommandAckPayload {
    private UUID commandId;
    private String deviceId;
    private String action;
    private String status;
    private Boolean led;
    private ZonedDateTime timestamp;
}
