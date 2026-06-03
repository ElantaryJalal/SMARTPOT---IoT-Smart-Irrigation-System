<?php
require_once __DIR__ . '/config.php';
start_app_session();

$error = '';
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $name = trim($_POST['full_name'] ?? '');
    $user = trim($_POST['username']  ?? '');
    $pwd  = $_POST['password']       ?? '';
    $code = trim($_POST['code']      ?? '');

    if ($name === '' || $user === '' || $pwd === '' || $code === '') {
        $error = 'All fields are required.';
    } else {
        try {
            $stmt = db()->prepare(
                'INSERT INTO users (full_name, username, password, pot_code)
                 VALUES (?, ?, ?, ?)'
            );
            $stmt->execute([
                $name, $user, password_hash($pwd, PASSWORD_BCRYPT), $code,
            ]);
            header('Location: login.php?registered=1');
            exit;
        } catch (PDOException $e) {
            $error = (str_contains($e->getMessage(), 'Duplicate'))
                ? 'Username or pot code already in use.'
                : 'Registration failed.';
        }
    }
}
?>
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>SMARTPOT - Register</title>
  <link rel="stylesheet" href="assets/style.css">
</head>
<body class="auth">
  <div class="card">
    <h1>Registration</h1>
    <?php if ($error): ?>
      <p class="error"><?= h($error) ?></p>
    <?php endif; ?>
    <form method="post" novalidate>
      <div class="row">
        <label>Full name
          <input type="text" name="full_name" placeholder="Enter your name" required>
        </label>
        <label>Username
          <input type="text" name="username" placeholder="Enter your username" required>
        </label>
      </div>
      <div class="row">
        <label>Password
          <input type="password" name="password" placeholder="Enter your password" required>
        </label>
        <label>Code
          <input type="text" name="code" placeholder="Enter your code" required>
        </label>
      </div>
      <button type="submit" class="btn primary">REGISTER</button>
    </form>
    <a class="btn ghost" href="login.php">LOGIN</a>
    <p class="muted">The <strong>code</strong> is printed on the pot you received.</p>
  </div>
</body>
</html>
