package com.example.iot.service;

import com.example.iot.dto.CommandAckPayload;
import com.example.iot.dto.StatusPayload;
import com.example.iot.dto.TelemetryPayload;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.context.annotation.Bean;
import org.springframework.integration.annotation.ServiceActivator;
import org.springframework.messaging.MessageHandler;
import org.springframework.stereotype.Service;

@Slf4j
@Service
@RequiredArgsConstructor
public class MqttMessageListener {

    private final TelemetryService telemetryService;
    private final CommandService commandService;
    private final DeviceService deviceService;
    private final ObjectMapper objectMapper = new ObjectMapper().registerModule(new JavaTimeModule());

    @Bean
    @ServiceActivator(inputChannel = "mqttInputChannel")
    public MessageHandler handler() {
        return message -> {
            String topic = (String) message.getHeaders().get("mqtt_receivedTopic");
            String payload = (String) message.getPayload();
            log.debug("Received topic: {}, payload: {}", topic, payload);

            try {
                if (topic != null) {
                    if (topic.endsWith("/telemetry")) {
                        TelemetryPayload telemetryPayload = objectMapper.readValue(payload, TelemetryPayload.class);
                        telemetryService.processTelemetry(telemetryPayload);
                    } else if (topic.endsWith("/command/ack")) {
                        CommandAckPayload ackPayload = objectMapper.readValue(payload, CommandAckPayload.class);
                        commandService.processAck(ackPayload);
                    } else if (topic.endsWith("/status")) {
                        StatusPayload statusPayload = objectMapper.readValue(payload, StatusPayload.class);
                        deviceService.processStatus(statusPayload);
                    }
                }
            } catch (Exception e) {
                log.error("Error processing MQTT message on topic: {}", topic, e);
            }
        };
    }
}
