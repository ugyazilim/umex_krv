<?php

declare(strict_types=1);

final class DeviceController
{
    public static function list(): void
    {
        Auth::requireUser();
        $devices = FileStore::readJson(STORAGE_DIR . '/devices.json', []);

        $safe = [];
        foreach ($devices as $id => $device) {
            $safe[$id] = [
                'id' => $device['id'] ?? $id,
                'name' => $device['name'] ?? $id,
                'enabled' => $device['enabled'] ?? true,
                'last_seen' => $device['last_seen'] ?? null,
                'last_ip' => $device['last_ip'] ?? null,
                'online' => self::isOnline($device['last_seen'] ?? null),
            ];
        }

        Response::success(array_values($safe));
    }

    public static function show(string $deviceId): void
    {
        Auth::requireUser();
        $devices = FileStore::readJson(STORAGE_DIR . '/devices.json', []);
        $device = $devices[$deviceId] ?? null;

        if (!$device) {
            Response::error('Cihaz bulunamadı', 404);
        }

        Response::success([
            'id' => $device['id'] ?? $deviceId,
            'name' => $device['name'] ?? $deviceId,
            'enabled' => $device['enabled'] ?? true,
            'last_seen' => $device['last_seen'] ?? null,
            'last_ip' => $device['last_ip'] ?? null,
            'online' => self::isOnline($device['last_seen'] ?? null),
        ]);
    }

    public static function create(): void
    {
        Auth::requireAdmin();
        $body = Request::body();

        $id = preg_replace('/[^a-zA-Z0-9\-_]/', '', $body['id'] ?? '');
        $name = trim($body['name'] ?? '');

        if ($id === '' || $name === '') {
            Response::error('id ve name zorunlu', 422);
        }

        $devices = FileStore::readJson(STORAGE_DIR . '/devices.json', []);
        if (isset($devices[$id])) {
            Response::error('Bu id zaten var', 409);
        }

        $apiKey = bin2hex(random_bytes(16));
        $devices[$id] = [
            'id' => $id,
            'name' => $name,
            'api_key' => $apiKey,
            'enabled' => true,
            'created_at' => date('c'),
        ];

        FileStore::writeJson(STORAGE_DIR . '/devices.json', $devices);
        LogController::write('device_create', $id, ['name' => $name]);

        Response::success([
            'id' => $id,
            'name' => $name,
            'api_key' => $apiKey,
        ], 'Cihaz oluşturuldu', 201);
    }

    public static function regenerateKey(string $deviceId): void
    {
        Auth::requireAdmin();
        $devices = FileStore::readJson(STORAGE_DIR . '/devices.json', []);

        if (!isset($devices[$deviceId])) {
            Response::error('Cihaz bulunamadı', 404);
        }

        $newKey = bin2hex(random_bytes(16));
        $devices[$deviceId]['api_key'] = $newKey;
        FileStore::writeJson(STORAGE_DIR . '/devices.json', $devices);

        LogController::write('device_key_regen', $deviceId);

        Response::success([
            'device_id' => $deviceId,
            'api_key' => $newKey,
        ], 'Yeni anahtar oluşturuldu');
    }

    private static function isOnline(?string $lastSeen): bool
    {
        if (!$lastSeen) {
            return false;
        }
        return (time() - strtotime($lastSeen)) <= 120;
    }
}
