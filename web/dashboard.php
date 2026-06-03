<?php
require_once __DIR__ . '/config.php';
$user = require_login();

// Latest reading
$stmt = db()->prepare(
    'SELECT * FROM readings WHERE pot_code = ?
     ORDER BY recorded_at DESC LIMIT 1'
);
$stmt->execute([$user['pot_code']]);
$latest = $stmt->fetch() ?: null;

$temp = $latest['temperature'] ?? null;
$hum  = $latest['humidity']    ?? null;
$tank = $latest['tank']        ?? null;
$pump = $latest['pump']        ?? 0;
$lowTank = ($tank !== null && $tank < TANK_ALERT_THRESHOLD);
?>
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>SMARTPOT - Dashboard</title>
  <link rel="stylesheet" href="assets/style.css">
  <meta http-equiv="refresh" content="30">
</head>
<body class="dashboard">
  <header>
    <div>
      <h1>SMARTPOT</h1>
      <p class="muted">Hi <?= h($user['full_name']) ?> - pot <code><?= h($user['pot_code']) ?></code></p>
    </div>
    <nav>
      <a class="btn ghost" href="history.php">Historique</a>
      <a class="btn primary" href="logout.php">LOG OUT</a>
    </nav>
  </header>

  <?php if ($lowTank): ?>
    <div class="alert">
      <strong>Water level low.</strong>
      Tank at <?= number_format((float)$tank, 2) ?> L - please refill.
      <button onclick="this.parentNode.remove()" class="btn primary small">OK</button>
    </div>
  <?php endif; ?>

  <section class="gauges">
    <div class="gauge">
      <div class="ring"
           style="--value: <?= $hum  !== null ? (float)$hum  : 0 ?>"
           data-label="Humidity">
        <span><?= $hum  !== null ? number_format((float)$hum,  1) : '--' ?>%</span>
      </div>
      <p>Humidity</p>
    </div>
    <div class="gauge">
      <div class="ring temp"
           style="--value: <?= $temp !== null ? min(100,(float)$temp*2) : 0 ?>"
           data-label="Temperature">
        <span><?= $temp !== null ? number_format((float)$temp, 1) : '--' ?>&deg;C</span>
      </div>
      <p>Temperature</p>
    </div>
    <div class="gauge">
      <div class="ring tank"
           style="--value: <?= $tank !== null ? min(100,(float)$tank*100) : 0 ?>"
           data-label="Tank">
        <span><?= $tank !== null ? number_format((float)$tank, 2) : '--' ?>L</span>
      </div>
      <p>Water tank</p>
    </div>
  </section>

  <p class="status">
    Pump is currently <strong><?= $pump ? 'ON' : 'OFF' ?></strong>.
    Page refreshes every 30 seconds.
  </p>
</body>
</html>
