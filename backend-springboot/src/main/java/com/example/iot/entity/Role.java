package com.example.iot.entity;

import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.Table;
import lombok.Data;
import java.util.UUID;

@Data
@Entity
@Table(name = "roles")
public class Role {
    @Id
    private UUID id;
    private String name;
}
