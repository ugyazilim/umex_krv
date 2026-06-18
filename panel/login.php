<?php

declare(strict_types=1);

require_once __DIR__ . '/auth.php';

if (panel_is_logged_in()) {
    header('Location: /panel/');
    exit;
}

$error = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $username = $_POST['username'] ?? '';
    $password = $_POST['password'] ?? '';

    if (panel_attempt_login($username, $password)) {
        header('Location: /panel/');
        exit;
    }

    $error = 'Hatalı kullanıcı adı veya şifre';
}
?>
<!DOCTYPE html>
<html lang="tr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Giriş — Ölmezler Doğada Karavan</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Quicksand:wght@500;700&display=swap');
    * { box-sizing: border-box; }
    body {
      margin: 0; min-height: 100vh; display: flex; align-items: center; justify-content: center;
      font-family: 'Quicksand', sans-serif; font-weight: 700;
      background: radial-gradient(circle at top, #3f3f4e 0%, #2b2b36 45%, #1a1a22 100%);
      color: #ecf0f1; padding: 20px;
    }
    .card {
      width: 100%; max-width: 400px; background: #343441; border-radius: 16px;
      padding: 28px 24px; box-shadow: 0 20px 50px rgba(0,0,0,0.45);
      border: 1px solid #4a4a5a;
    }
    h1 { margin: 0 0 4px; font-size: 22px; letter-spacing: 2px; text-align: center; }
    .sub { text-align: center; color: #3498db; font-size: 11px; letter-spacing: 2px; margin-bottom: 24px; text-transform: uppercase; }
    label { display: block; font-size: 12px; color: #95a5a6; margin-bottom: 6px; }
    input {
      width: 100%; padding: 12px 14px; margin-bottom: 16px; border-radius: 10px;
      border: 1px solid #4a4a5a; background: #2b2b36; color: #fff; font-family: inherit; font-size: 15px;
    }
    input:focus { outline: none; border-color: #3498db; box-shadow: 0 0 0 2px rgba(52,152,219,0.25); }
    button {
      width: 100%; padding: 13px; border: none; border-radius: 10px; cursor: pointer;
      background: linear-gradient(135deg, #2980b9, #3498db); color: #fff;
      font-family: inherit; font-weight: 700; font-size: 15px; letter-spacing: 1px;
    }
    button:active { transform: scale(0.98); }
    .error {
      background: rgba(231,76,60,0.15); border: 1px solid #e74c3c; color: #ffb3aa;
      padding: 10px 12px; border-radius: 8px; font-size: 13px; margin-bottom: 16px; text-align: center;
    }
    .hint { margin-top: 18px; font-size: 11px; color: #7f8c8d; text-align: center; line-height: 1.5; }
  </style>
</head>
<body>
  <form class="card" method="post" autocomplete="on">
    <h1>ÖLMEZLER DOĞADA</h1>
    <div class="sub">Karavan Kontrol Paneli</div>

    <?php if ($error !== ''): ?>
      <div class="error"><?= htmlspecialchars($error, ENT_QUOTES, 'UTF-8') ?></div>
    <?php endif; ?>

    <label for="username">Kullanıcı Adı</label>
    <input type="text" id="username" name="username" required autofocus autocomplete="username">

    <label for="password">Şifre</label>
    <input type="password" id="password" name="password" required autocomplete="current-password">

    <button type="submit">GİRİŞ YAP</button>

    <div class="hint">Varsayılan: admin / admin123<br>İlk girişten sonra şifrenizi değiştirmeniz önerilir.</div>
  </form>
</body>
</html>
