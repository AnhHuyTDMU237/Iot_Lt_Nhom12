package com.example.iot.controller;

import com.example.iot.entity.Command;
import com.example.iot.entity.Device;
import com.example.iot.entity.Telemetry;
import com.example.iot.service.CommandService;
import com.example.iot.service.DeviceService;
import com.example.iot.service.TelemetryService;
import lombok.Data;
import lombok.RequiredArgsConstructor;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.PageRequest;
import org.springframework.format.annotation.DateTimeFormat;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.*;

import java.time.ZonedDateTime;
import java.util.List;

@RestController
@RequestMapping("/api/v1")
@RequiredArgsConstructor
public class DeviceController {

    private final DeviceService deviceService;
    private final TelemetryService telemetryService;
    private final CommandService commandService;

    @GetMapping("/devices")
    public ResponseEntity<List<Device>> getAllDevices() {
        return ResponseEntity.ok(deviceService.getAllDevices());
    }

    @GetMapping("/devices/{deviceId}")
    public ResponseEntity<Device> getDevice(@PathVariable String deviceId) {
        return ResponseEntity.ok(deviceService.getDeviceByDeviceId(deviceId));
    }

    @GetMapping("/devices/{deviceId}/telemetry/latest")
    public ResponseEntity<Telemetry> getLatestTelemetry(@PathVariable String deviceId) {
        Telemetry telemetry = telemetryService.getLatestTelemetry(deviceId);
        if (telemetry == null) {
            return ResponseEntity.notFound().build();
        }
        return ResponseEntity.ok(telemetry);
    }

    @GetMapping("/devices/{deviceId}/telemetry")
    public ResponseEntity<Page<Telemetry>> getTelemetryHistory(
            @PathVariable String deviceId,
            @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) ZonedDateTime from,
            @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) ZonedDateTime to,
            @RequestParam(defaultValue = "0") int page,
            @RequestParam(defaultValue = "20") int size) {
        return ResponseEntity.ok(telemetryService.getTelemetryHistory(deviceId, from, to, PageRequest.of(page, size)));
    }

    @GetMapping("/devices/{deviceId}/commands")
    public ResponseEntity<Page<Command>> getCommandHistory(
            @PathVariable String deviceId,
            @RequestParam(defaultValue = "0") int page,
            @RequestParam(defaultValue = "10") int size) {
        return ResponseEntity.ok(commandService.getCommandHistory(deviceId, PageRequest.of(page, size)));
    }

    @PostMapping("/devices/{deviceId}/commands")
    @PreAuthorize("hasAnyRole('ADMIN', 'OPERATOR')")
    public ResponseEntity<Command> sendCommand(@PathVariable String deviceId, @RequestBody CommandRequest request) {
        Command command = commandService.sendCommand(deviceId, request.getAction());
        return ResponseEntity.ok(command);
    }
}

@Data
class CommandRequest {
    private String action;
}
