<?php

declare(strict_types=1);

final class AuthController
{
    public static function login(): void
    {
        $body = Request::body();
        $username = trim($body['username'] ?? '');
        $password = $body['password'] ?? '';

        if ($username === '' || $password === '') {
            Response::error('username ve password zorunlu', 422);
        }

        $session = Auth::login($username, $password);
        if (!$session) {
            Response::error('Hatalı kullanıcı adı veya şifre', 401);
        }

        LogController::write('login', $username, ['ip' => Request::ip()]);

        Response::success($session, 'Giriş başarılı');
    }

    public static function me(): void
    {
        $user = Auth::requireUser();
        Response::success([
            'username' => $user['username'],
            'role' => $user['role'] ?? 'user',
        ]);
    }

    public static function logout(): void
    {
        $token = Request::bearerToken();
        if ($token) {
            $tokens = FileStore::readJson(STORAGE_DIR . '/tokens.json', []);
            unset($tokens[$token]);
            FileStore::writeJson(STORAGE_DIR . '/tokens.json', $tokens);
        }
        Response::success(null, 'Çıkış yapıldı');
    }
}
