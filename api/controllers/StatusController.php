<?php

declare(strict_types=1);

final class StatusController
{
    private static function statusFile(string $deviceId): string
    {
        return STORAGE_DIR . '/status/' . $deviceId . '.json';
    }

    private static function historyFile(string $deviceId): string
    {
        return STORAGE_DIR . '/logs/history_' . $deviceId . '.txt';
    }

    public static function push(): void
    {
        $device = Auth::requireDevice();
        $deviceId = $device['id'];
        $body = Request::body();

        if (empty($body)) {
            Response::error('Boş veri gönderildi', 422);
        }

        $payload = [
            'device_id' => $deviceId,
            'received_at' => date('c'),
            'ip' => Request::ip(),
            'data' => $body,
        ];

        FileStore::writeJson(self::statusFile($deviceId), $payload);
        FileStore::appendLine(self::historyFile($deviceId), [
            'at' => date('c'),
            'data' => $body,
        ]);

        $config = FileStore::readJson(STORAGE_DIR . '/config.json', []);
        $maxHistory = (int) ($config['max_history'] ?? 200);
        self::trimHistory(self::historyFile($deviceId), $maxHistory);

        Auth::touchDevice($deviceId);

        Response::success([
            'device_id' => $deviceId,
            'online' => true,
        ], 'Durum kaydedildi');
    }

    public static function latest(?string $deviceId = null): void
    {
        Auth::requireUser();
        $deviceId = $deviceId ?: (string) Request::query('device_id', 'karavan-1');
        $status = self::readForDevice($deviceId);

        if ($status === null) {
            Response::error('Bu cihaz için henüz veri yok', 404);
        }

        Response::success($status);
    }

    public static function readForDevice(string $deviceId): ?array
    {
        $status = FileStore::readJson(self::statusFile($deviceId));
        return empty($status) ? null : $status;
    }

    public static function history(?string $deviceId = null): void
    {
        Auth::requireUser();
        $deviceId = $deviceId ?: (string) Request::query('device_id', 'karavan-1');
        $limit = min(500, max(1, (int) Request::query('limit', 50)));

        $rows = FileStore::readLines(self::historyFile($deviceId), $limit, true);
        Response::success([
            'device_id' => $deviceId,
            'count' => count($rows),
            'history' => $rows,
        ]);
    }

    public static function online(?string $deviceId = null): void
    {
        Auth::requireUser();
        $deviceId = $deviceId ?: (string) Request::query('device_id', 'karavan-1');
        $devices = FileStore::readJson(STORAGE_DIR . '/devices.json', []);
        $device = $devices[$deviceId] ?? null;

        if (!$device) {
            Response::error('Cihaz bulunamadı', 404);
        }

        $lastSeen = $device['last_seen'] ?? null;
        $online = false;
        $secondsAgo = null;

        if ($lastSeen) {
            $secondsAgo = time() - strtotime($lastSeen);
            $online = $secondsAgo <= 120;
        }

        Response::success([
            'device_id' => $deviceId,
            'online' => $online,
            'last_seen' => $lastSeen,
            'seconds_ago' => $secondsAgo,
        ]);
    }

    private static function trimHistory(string $path, int $maxLines): void
    {
        if (!file_exists($path)) {
            return;
        }

        $lines = file($path, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES) ?: [];
        if (count($lines) <= $maxLines) {
            return;
        }

        $keep = array_slice($lines, -$maxLines);
        file_put_contents($path, implode(PHP_EOL, $keep) . PHP_EOL, LOCK_EX);
    }
}
