package com.example.iot.repository;

import com.example.iot.entity.Telemetry;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import java.time.ZonedDateTime;
import java.util.Optional;
import java.util.UUID;

public interface TelemetryRepository extends JpaRepository<Telemetry, UUID> {
    Page<Telemetry> findByDeviceIdAndRecordedAtBetweenOrderByRecordedAtDesc(
            String deviceId, ZonedDateTime from, ZonedDateTime to, Pageable pageable);
            
    Page<Telemetry> findByDeviceIdOrderByRecordedAtDesc(String deviceId, Pageable pageable);

    Optional<Telemetry> findFirstByDeviceIdOrderByRecordedAtDesc(String deviceId);
}
