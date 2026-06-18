<?php

declare(strict_types=1);

session_start();

require_once dirname(__DIR__) . '/api/bootstrap.php';

function panel_is_logged_in(): bool
{
    if (empty($_SESSION['api_token'])) {
        return false;
    }

    $user = Auth::validateToken($_SESSION['api_token']);
    if (!$user) {
        panel_logout();
        return false;
    }

    return true;
}

function panel_require_login(): void
{
    if (!panel_is_logged_in()) {
        header('Location: /panel/login.php');
        exit;
    }
}

function panel_attempt_login(string $username, string $password): bool
{
    $session = Auth::login(trim($username), $password);
    if (!$session) {
        return false;
    }

    $_SESSION['api_token'] = $session['token'];
    $_SESSION['username'] = trim($username);
    $_SESSION['role'] = $session['role'] ?? 'user';

    LogController::write('panel_login', $_SESSION['username'], ['ip' => $_SERVER['REMOTE_ADDR'] ?? '']);

    return true;
}

function panel_logout(): void
{
    if (!empty($_SESSION['api_token'])) {
        $tokens = FileStore::readJson(STORAGE_DIR . '/tokens.json', []);
        unset($tokens[$_SESSION['api_token']]);
        FileStore::writeJson(STORAGE_DIR . '/tokens.json', $tokens);
    }

    $_SESSION = [];
    if (ini_get('session.use_cookies')) {
        $p = session_get_cookie_params();
        setcookie(session_name(), '', time() - 42000, $p['path'], $p['domain'], $p['secure'], $p['httponly']);
    }
    session_destroy();
}

function panel_token(): string
{
    return $_SESSION['api_token'] ?? '';
}

function panel_username(): string
{
    return $_SESSION['username'] ?? '';
}
