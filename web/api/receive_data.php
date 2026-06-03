<?php
// SMARTPOT - data ingestion endpoint
// Receives JSON POSTs from the ESP8266 bridge and stores them.
// curl example:
//   curl -X POST -H 'Content-Type: application/json' \
//        -d '{"code":"DEMO-CODE-001","temperature":22.4,"humidity":55,
//             "soil":38,"light":720,"tank":0.9,"pump":0}' \
//        http://localhost/smartpot/api/receive_data.php

require_once __DIR__ . '/../config.php';

header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['ok' => false, 'error' => 'POST only']);
    exit;
}

$raw = file_get_contents('php://input');
$data = json_decode($raw, true);
if (!is_array($data) || empty($data['code'])) {
    http_response_code(400);
    echo json_encode(['ok' => false, 'error' => 'bad payload']);
    exit;
}

try {
    // Verify the pot code exists - prevents random pots from polluting the db.
    $check = db()->prepare('SELECT 1 FROM users WHERE pot_code = ?');
    $check->execute([$data['code']]);
    if (!$check->fetchColumn()) {
        http_response_code(403);
        echo json_encode(['ok' => false, 'error' => 'unknown pot']);
        exit;
    }

    $stmt = db()->prepare(
        'INSERT INTO readings (pot_code, temperature, humidity, soil, light, tank, pump)
         VALUES (?, ?, ?, ?, ?, ?, ?)'
    );
    $stmt->execute([
        $data['code'],
        $data['temperature'] ?? null,
        $data['humidity']    ?? null,
        $data['soil']        ?? null,
        $data['light']       ?? null,
        $data['tank']        ?? null,
        !empty($data['pump']) ? 1 : 0,
    ]);
    echo json_encode(['ok' => true]);
} catch (Throwable $e) {
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => 'server error']);
}
