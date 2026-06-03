<?php
// Database + session bootstrap. Override values for your local setup.

define('DB_HOST', '127.0.0.1');
define('DB_NAME', 'smartpot');
define('DB_USER', 'root');
define('DB_PASS', '');

// Threshold for the "Refill the tank" alert (litres)
define('TANK_ALERT_THRESHOLD', 0.2);

function db(): PDO {
    static $pdo = null;
    if ($pdo === null) {
        $dsn = 'mysql:host=' . DB_HOST . ';dbname=' . DB_NAME . ';charset=utf8mb4';
        $pdo = new PDO($dsn, DB_USER, DB_PASS, [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        ]);
    }
    return $pdo;
}

function start_app_session(): void {
    if (session_status() === PHP_SESSION_NONE) {
        session_start();
    }
}

function require_login(): array {
    start_app_session();
    if (empty($_SESSION['user_id'])) {
        header('Location: login.php');
        exit;
    }
    return [
        'id'        => $_SESSION['user_id'],
        'username'  => $_SESSION['username'] ?? '',
        'pot_code'  => $_SESSION['pot_code'] ?? '',
        'full_name' => $_SESSION['full_name'] ?? '',
    ];
}

function h(?string $s): string {
    return htmlspecialchars($s ?? '', ENT_QUOTES, 'UTF-8');
}
