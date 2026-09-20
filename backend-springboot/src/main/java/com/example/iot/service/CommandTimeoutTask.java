package com.example.iot.service;

import com.example.iot.entity.Command;
import com.example.iot.repository.CommandRepository;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;

import java.time.ZonedDateTime;
import java.util.List;

@Slf4j
@Service
@RequiredArgsConstructor
public class CommandTimeoutTask {

    private final CommandRepository commandRepository;

    @Scheduled(fixedRate = 10000)
    public void checkTimeouts() {
        ZonedDateTime timeoutThreshold = ZonedDateTime.now().minusSeconds(15);
        List<Command> pendingCommands = commandRepository.findByStatusAndCreatedAtBefore("SENT", timeoutThreshold);
        for (Command cmd : pendingCommands) {
            cmd.setStatus("TIMEOUT");
            commandRepository.save(cmd);
            log.warn("Command {} timed out waiting for ACK", cmd.getId());
        }
    }
}
