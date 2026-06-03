-- SMARTPOT - database schema
-- Run with:  mysql -u root -p < db.sql

CREATE DATABASE IF NOT EXISTS smartpot CHARACTER SET utf8mb4;
USE smartpot;

DROP TABLE IF EXISTS readings;
DROP TABLE IF EXISTS pots;
DROP TABLE IF EXISTS users;

CREATE TABLE users (
  id         INT AUTO_INCREMENT PRIMARY KEY,
  full_name  VARCHAR(120) NOT NULL,
  username   VARCHAR(64)  NOT NULL UNIQUE,
  password   VARCHAR(255) NOT NULL,           -- password_hash(BCRYPT)
  pot_code   VARCHAR(64)  NOT NULL UNIQUE,    -- printed on the device
  created_at DATETIME     DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE readings (
  id           BIGINT AUTO_INCREMENT PRIMARY KEY,
  pot_code     VARCHAR(64) NOT NULL,
  temperature  DECIMAL(5,1),
  humidity     DECIMAL(5,1),
  soil         DECIMAL(5,1),
  light        INT,
  tank         DECIMAL(4,2),
  pump         TINYINT(1) DEFAULT 0,
  recorded_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_pot_time (pot_code, recorded_at)
);

-- Demo account so the dashboard works out of the box.
-- username: demo   password: demo1234   pot_code: DEMO-CODE-001
INSERT INTO users (full_name, username, password, pot_code) VALUES (
  'Demo User',
  'demo',
  '$2a$10$aSf8rQ7hC4no5ry37hQY5efmrz6p0lsHS69fG0Kkq1mVes.NAJFMG',
  'DEMO-CODE-001'
);

-- A few sample readings so /history.php has something to show
INSERT INTO readings (pot_code, temperature, humidity, soil, light, tank, pump, recorded_at) VALUES
  ('DEMO-CODE-001', 22.4, 55.0, 38.0, 720, 0.95, 0, NOW() - INTERVAL 6 HOUR),
  ('DEMO-CODE-001', 24.1, 51.0, 32.0, 810, 0.92, 1, NOW() - INTERVAL 5 HOUR),
  ('DEMO-CODE-001', 25.3, 48.0, 41.0, 760, 0.88, 0, NOW() - INTERVAL 4 HOUR),
  ('DEMO-CODE-001', 26.0, 46.0, 39.0, 680, 0.85, 0, NOW() - INTERVAL 3 HOUR),
  ('DEMO-CODE-001', 24.7, 50.0, 36.0, 540, 0.83, 0, NOW() - INTERVAL 2 HOUR),
  ('DEMO-CODE-001', 23.2, 53.0, 35.0, 220, 0.80, 0, NOW() - INTERVAL 1 HOUR),
  ('DEMO-CODE-001', 22.0, 56.0, 34.0,  90, 0.78, 0, NOW());
