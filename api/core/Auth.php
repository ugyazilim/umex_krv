<?php

declare(strict_types=1);

final class Auth
{
    private const USERS_FILE = STORAGE_DIR . '/users.json';
    private const TOKENS_FILE = STORAGE_DIR . '/tokens.json';
    private const DEVICES_FILE = STORAGE_DIR . '/devices.json';
    private const CONFIG_FILE = STORAGE_DIR . '/config.json';

    public static function login(string $username, string $password): ?array
    {
        $users = FileStore::readJson(self::USERS_FILE, []);
        foreach ($users as $user) {
            if (($user['username'] ?? '') === $username
                && password_verify($password, $user['password_hash'] ?? '')) {
                return self::issueToken($username, $user['role'] ?? 'user');
            }
        }
        return null;
    }

    public static function issueToken(string $username, string $role): array
    {
        $config = FileStore::readJson(self::CONFIG_FILE, []);
        $ttl = (int) ($config['token_ttl'] ?? 86400);
        $token = bin2hex(random_bytes(32));
        $expires = time() + $ttl;

        $tokens = FileStore::readJson(self::TOKENS_FILE, []);
        $tokens[$token] = [
            'username' => $username,
            'role' => $role,
            'expires' => $expires,
            'created_at' => date('c'),
        ];
        FileStore::writeJson(self::TOKENS_FILE, $tokens);

        return [
            'token' => $token,
            'expires_at' => date('c', $expires),
            'expires_in' => $ttl,
            'role' => $role,
        ];
    }

    public static function validateToken(?string $token): ?array
    {
        if (!$token) {
            return null;
        }

        $tokens = FileStore::readJson(self::TOKENS_FILE, []);
        $record = $tokens[$token] ?? null;
        if (!$record) {
            return null;
        }

        if (($record['expires'] ?? 0) < time()) {
            unset($tokens[$token]);
            FileStore::writeJson(self::TOKENS_FILE, $tokens);
            return null;
        }

        return $record;
    }

    public static function requireUser(): array
    {
        $token = Request::bearerToken();
        $user = self::validateToken($token);
        if (!$user) {
            Response::error('Geçersiz veya süresi dolmuş token', 401);
        }
        return $user;
    }

    public static function requireAdmin(): array
    {
        $user = self::requireUser();
        if (($user['role'] ?? '') !== 'admin') {
            Response::error('Admin yetkisi gerekli', 403);
        }
        return $user;
    }

    public static function validateDeviceKey(?string $key): ?array
    {
        if (!$key) {
            return null;
        }

        $devices = FileStore::readJson(self::DEVICES_FILE, []);
        foreach ($devices as $device) {
            if (($device['api_key'] ?? '') === $key && ($device['enabled'] ?? true)) {
                return $device;
            }
        }
        return null;
    }

    public static function requireDevice(): array
    {
        $device = self::validateDeviceKey(Request::deviceKey());
        if (!$device) {
            Response::error('Geçersiz cihaz anahtarı', 401);
        }
        return $device;
    }

    public static function touchDevice(string $deviceId): void
    {
        $devices = FileStore::readJson(self::DEVICES_FILE, []);
        if (!isset($devices[$deviceId])) {
            return;
        }
        $devices[$deviceId]['last_seen'] = date('c');
        $devices[$deviceId]['last_ip'] = Request::ip();
        FileStore::writeJson(self::DEVICES_FILE, $devices);
    }
}
