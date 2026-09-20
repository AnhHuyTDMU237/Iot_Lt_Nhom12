package com.example.iot.service;

import com.example.iot.config.MqttGateway;
import com.example.iot.dto.CommandAckPayload;
import com.example.iot.entity.Command;
import com.example.iot.entity.Device;
import com.example.iot.repository.CommandRepository;
import com.example.iot.repository.DeviceRepository;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.ZonedDateTime;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.UUID;

@Slf4j
@Service
@RequiredArgsConstructor
public class CommandService {

    private final CommandRepository commandRepository;
    private final DeviceRepository deviceRepository;
    private final MqttGateway mqttGateway;
    private final ObjectMapper objectMapper = new ObjectMapper();

    @Transactional
    public Command sendCommand(String deviceId, String action) {
        Device device = deviceRepository.findByDeviceId(deviceId)
                .orElseThrow(() -> new RuntimeException("Device not found"));

        if (!"LED_ON".equals(action) && !"LED_OFF".equals(action)) {
            throw new IllegalArgumentException("Invalid action");
        }

        Command command = new Command();
        command.setId(UUID.randomUUID());
        command.setDeviceId(deviceId);
        command.setAction(action);
        command.setStatus("PENDING");
        command.setCreatedAt(ZonedDateTime.now());
        
        String username = SecurityContextHolder.getContext().getAuthentication() != null 
                ? SecurityContextHolder.getContext().getAuthentication().getName() : "system";
        command.setCreatedBy(username);

        try {
            Map<String, Object> payloadMap = new HashMap<>();
            payloadMap.put("commandId", command.getId().toString());
            payloadMap.put("action", action);
            payloadMap.put("timestamp", ZonedDateTime.now().toString());
            String payloadJson = objectMapper.writeValueAsString(payloadMap);
            command.setPayload(payloadJson);
            
            commandRepository.save(command);

            String topic = "device/" + deviceId + "/command";
            mqttGateway.sendToMqtt(topic, 1, payloadJson);
            
            command.setStatus("SENT");
            command.setSentAt(ZonedDateTime.now());
            commandRepository.save(command);
            
            log.info("Sent command {} to device {}", command.getId(), deviceId);
            return command;
            
        } catch (JsonProcessingException e) {
            log.error("Failed to serialize command payload", e);
            throw new RuntimeException("Failed to serialize command payload");
        } catch (Exception e) {
            command.setStatus("FAILED");
            commandRepository.save(command);
            log.error("Failed to send command via MQTT", e);
            throw new RuntimeException("Failed to send command via MQTT");
        }
    }

    @Transactional
    public void processAck(CommandAckPayload ack) {
        if (ack.getCommandId() == null) return;
        
        Optional<Command> cmdOpt = commandRepository.findById(ack.getCommandId());
        if (cmdOpt.isPresent()) {
            Command command = cmdOpt.get();
            command.setStatus("ACKNOWLEDGED");
            command.setAcknowledgedAt(ack.getTimestamp() != null ? ack.getTimestamp() : ZonedDateTime.now());
            commandRepository.save(command);
            
            // update device led state if applicable
            if (ack.getLed() != null) {
                deviceRepository.findByDeviceId(ack.getDeviceId()).ifPresent(device -> {
                    device.setLedState(ack.getLed());
                    device.setUpdatedAt(ZonedDateTime.now());
                    deviceRepository.save(device);
                });
            }
            log.info("Command {} acknowledged", ack.getCommandId());
        } else {
            log.warn("Received ACK for unknown command ID: {}", ack.getCommandId());
        }
    }

    public Page<Command> getCommandHistory(String deviceId, Pageable pageable) {
        return commandRepository.findByDeviceIdOrderByCreatedAtDesc(deviceId, pageable);
    }
}
