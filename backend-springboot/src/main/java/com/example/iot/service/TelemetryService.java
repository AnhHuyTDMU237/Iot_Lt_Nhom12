package com.example.iot.service;

import com.example.iot.dto.TelemetryPayload;
import com.example.iot.entity.Device;
import com.example.iot.entity.Telemetry;
import com.example.iot.repository.DeviceRepository;
import com.example.iot.repository.TelemetryRepository;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.ZonedDateTime;
import java.util.Optional;
import java.util.UUID;

@Slf4j
@Service
@RequiredArgsConstructor
public class TelemetryService {

    private final TelemetryRepository telemetryRepository;
    private final DeviceRepository deviceRepository;

    @Transactional
    public void processTelemetry(TelemetryPayload payload) {
        if (payload.getDeviceId() == null) return;
        
        Optional<Device> deviceOpt = deviceRepository.findByDeviceId(payload.getDeviceId());
        if (deviceOpt.isPresent()) {
            Device device = deviceOpt.get();
            device.setLastSeenAt(payload.getTimestamp());
            if (payload.getLed() != null) {
                device.setLedState(payload.getLed());
            }
            if ("OFFLINE".equals(device.getStatus())) {
                device.setStatus("ONLINE");
            }
            device.setUpdatedAt(ZonedDateTime.now());
            deviceRepository.save(device);

            Telemetry telemetry = new Telemetry();
            telemetry.setId(UUID.randomUUID());
            telemetry.setDeviceId(payload.getDeviceId());
            telemetry.setTemperature(payload.getTemperature());
            telemetry.setHumidity(payload.getHumidity());
            telemetry.setIlluminance(payload.getIlluminance());
            telemetry.setSoilMoisture(payload.getSoilMoisture());
            telemetry.setLedState(payload.getLed());
            telemetry.setRecordedAt(payload.getTimestamp() != null ? payload.getTimestamp() : ZonedDateTime.now());
            telemetry.setReceivedAt(ZonedDateTime.now());
            
            telemetryRepository.save(telemetry);
            log.info("Saved telemetry for device: {}", payload.getDeviceId());
        } else {
            log.warn("Telemetry received for unknown device: {}", payload.getDeviceId());
        }
    }

    public Telemetry getLatestTelemetry(String deviceId) {
        return telemetryRepository.findFirstByDeviceIdOrderByRecordedAtDesc(deviceId)
                .orElse(null);
    }

    public Page<Telemetry> getTelemetryHistory(String deviceId, ZonedDateTime from, ZonedDateTime to, Pageable pageable) {
        if (from != null && to != null) {
            return telemetryRepository.findByDeviceIdAndRecordedAtBetweenOrderByRecordedAtDesc(deviceId, from, to, pageable);
        }
        return telemetryRepository.findByDeviceIdOrderByRecordedAtDesc(deviceId, pageable);
    }
}
