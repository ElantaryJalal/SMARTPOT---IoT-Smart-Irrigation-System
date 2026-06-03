<?php
require_once __DIR__ . '/config.php';
$user = require_login();

$stmt = db()->prepare(
    'SELECT * FROM readings WHERE pot_code = ?
     ORDER BY recorded_at DESC LIMIT 200'
);
$stmt->execute([$user['pot_code']]);
$rows = $stmt->fetchAll();
?>
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>SMARTPOT - History</title>
  <link rel="stylesheet" href="assets/style.css">
</head>
<body class="dashboard">
  <header>
    <div><h1>History</h1><p class="muted">Last 200 readings</p></div>
    <nav>
      <a class="btn ghost"   href="dashboard.php">BACK</a>
      <a class="btn primary" href="logout.php">LOG OUT</a>
    </nav>
  </header>

  <table class="history">
    <thead>
      <tr>
        <th>Humidity (%)</th>
        <th>Temperature (&deg;C)</th>
        <th>Soil (%)</th>
        <th>Tank (L)</th>
        <th>Pump</th>
        <th>Timing</th>
      </tr>
    </thead>
    <tbody>
      <?php if (!$rows): ?>
        <tr><td colspan="6" class="muted">No data yet. Power on your pot.</td></tr>
      <?php endif; ?>
      <?php foreach ($rows as $r): ?>
        <tr>
          <td><?= h(number_format((float)$r['humidity'], 1)) ?></td>
          <td><?= h(number_format((float)$r['temperature'], 1)) ?></td>
          <td><?= h(number_format((float)$r['soil'], 1)) ?></td>
          <td><?= h(number_format((float)$r['tank'], 2)) ?></td>
          <td><?= $r['pump'] ? 'ON' : 'off' ?></td>
          <td><?= h($r['recorded_at']) ?></td>
        </tr>
      <?php endforeach; ?>
    </tbody>
  </table>
</body>
</html>
