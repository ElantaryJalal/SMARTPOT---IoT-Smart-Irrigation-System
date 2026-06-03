<?php
require_once __DIR__ . '/config.php';
start_app_session();
header('Location: ' . (empty($_SESSION['user_id']) ? 'login.php' : 'dashboard.php'));
exit;
