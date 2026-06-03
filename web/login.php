<?php
require_once __DIR__ . '/config.php';
start_app_session();

$error = '';
$justRegistered = isset($_GET['registered']);

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $user = trim($_POST['username'] ?? '');
    $pwd  = $_POST['password']     ?? '';
    $code = trim($_POST['code']    ?? '');

    $stmt = db()->prepare(
        'SELECT * FROM users WHERE username = ? AND pot_code = ?'
    );
    $stmt->execute([$user, $code]);
    $row = $stmt->fetch();

    if ($row && password_verify($pwd, $row['password'])) {
        $_SESSION['user_id']   = (int)$row['id'];
        $_SESSION['username']  = $row['username'];
        $_SESSION['pot_code']  = $row['pot_code'];
        $_SESSION['full_name'] = $row['full_name'];
        header('Location: dashboard.php');
        exit;
    }
    $error = 'Login, password or code incorrect.';
}
?>
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>SMARTPOT - Login</title>
  <link rel="stylesheet" href="assets/style.css">
</head>
<body class="auth">
  <div class="card">
    <h1>Login</h1>
    <?php if ($justRegistered): ?>
      <p class="success">Account created. You can sign in.</p>
    <?php endif; ?>
    <?php if ($error): ?>
      <p class="error"><?= h($error) ?></p>
    <?php endif; ?>
    <form method="post" novalidate>
      <div class="row">
        <label>Username
          <input type="text" name="username" placeholder="Enter your username" required>
        </label>
        <label>Password
          <input type="password" name="password" placeholder="Enter your password" required>
        </label>
      </div>
      <label>Code
        <input type="text" name="code" placeholder="Enter your code" required>
      </label>
      <button type="submit" class="btn primary">DONE</button>
    </form>
    <a class="btn ghost" href="register.php">SIGN UP</a>
    <p class="muted">Demo account: <code>demo</code> / <code>demo1234</code> / <code>DEMO-CODE-001</code></p>
  </div>
</body>
</html>
