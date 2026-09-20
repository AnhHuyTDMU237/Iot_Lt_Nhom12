package com.example.iot.repository;

import com.example.iot.entity.Command;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import java.time.ZonedDateTime;
import java.util.List;
import java.util.UUID;

public interface CommandRepository extends JpaRepository<Command, UUID> {
    Page<Command> findByDeviceIdOrderByCreatedAtDesc(String deviceId, Pageable pageable);
    
    List<Command> findByStatusAndCreatedAtBefore(String status, ZonedDateTime time);
}
