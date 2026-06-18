<?php

declare(strict_types=1);

final class CommandController
{
    private static function queueFile(string $deviceId): string
    {
        return STORAGE_DIR . '/commands/' . $deviceId . '.json';
    }

    /** ESP cihazı bekleyen komutları çeker */
    public static function poll(): void
    {
        $device = Auth::requireDevice();
        $deviceId = $device['id'];
        Auth::touchDevice($deviceId);

        $queue = FileStore::readJson(self::queueFile($deviceId), []);
        $pending = array_values(array_filter($queue, fn ($c) => ($c['status'] ?? '') === 'pending'));

        Response::success([
            'device_id' => $deviceId,
            'commands' => $pending,
        ]);
    }

    /** ESP komut sonucunu bildirir */
    public static function ack(): void
    {
        $device = Auth::requireDevice();
        $deviceId = $device['id'];
        $body = Request::body();
        $commandId = $body['id'] ?? $body['command_id'] ?? null;

        if (!$commandId) {
            Response::error('command id gerekli', 422);
        }

        $queue = FileStore::readJson(self::queueFile($deviceId), []);
        $found = false;

        foreach ($queue as &$cmd) {
            if (($cmd['id'] ?? '') === $commandId) {
                $cmd['status'] = ($body['success'] ?? true) ? 'done' : 'failed';
                $cmd['result'] = $body['result'] ?? null;
                $cmd['acked_at'] = date('c');
                $found = true;
                break;
            }
        }
        unset($cmd);

        if (!$found) {
            Response::error('Komut bulunamadı', 404);
        }

        FileStore::writeJson(self::queueFile($deviceId), $queue);
        LogController::write('command_ack', $deviceId, ['command_id' => $commandId]);

        Response::success(null, 'Komut onaylandı');
    }

    /** Mobil/web kullanıcısı komut gönderir */
    public static function send(?string $deviceId = null): void
    {
        $user = Auth::requireUser();
        $body = Request::body();
        $deviceId = $deviceId
            ?: (string) ($body['device_id'] ?? Request::query('device_id', 'karavan-1'));

        try {
            $command = self::queueCommand(
                $deviceId,
                (string) ($body['action'] ?? ''),
                is_array($body['params'] ?? null) ? $body['params'] : [],
                (string) ($user['username'] ?? 'unknown')
            );
        } catch (InvalidArgumentException $e) {
            Response::error($e->getMessage(), 422);
        }

        Response::success($command, 'Komut kuyruğa eklendi', 201);
    }

    public static function queueCommand(string $deviceId, string $action, array $params, string $username): array
    {
        if ($action === '') {
            throw new InvalidArgumentException('action alanı zorunlu');
        }

        $devices = FileStore::readJson(STORAGE_DIR . '/devices.json', []);
        if (!isset($devices[$deviceId])) {
            throw new InvalidArgumentException('Cihaz bulunamadı: ' . $deviceId);
        }

        $allowed = [
            'toggle', 'toggleMainBat', 'toggleUPS', 'toggleWebastoPower',
            'allOff', 'setTarget', 'setWaterTarget', 'setFridgeTarget',
            'toggleAuto', 'toggleWaterAuto', 'toggleFridgeAuto',
            'manualWebasto', 'manualWaterHeater', 'manualFridge',
            'toggleGasAlarm', 'toggleRainAlarm', 'setRelayName',
        ];

        if (!in_array($action, $allowed, true)) {
            throw new InvalidArgumentException('Geçersiz action');
        }

        $command = [
            'id' => uniqid('cmd_', true),
            'action' => $action,
            'params' => $params,
            'status' => 'pending',
            'created_at' => date('c'),
            'created_by' => $username,
        ];

        $queue = FileStore::readJson(self::queueFile($deviceId), []);
        $queue[] = $command;
        $queue = array_slice($queue, -50);
        FileStore::writeJson(self::queueFile($deviceId), $queue);

        LogController::write('command_send', $deviceId, $command);

        return $command;
    }

    public static function list(?string $deviceId = null): void
    {
        Auth::requireUser();
        $deviceId = $deviceId ?: (string) Request::query('device_id', 'karavan-1');
        $queue = FileStore::readJson(self::queueFile($deviceId), []);

        Response::success([
            'device_id' => $deviceId,
            'commands' => array_reverse($queue),
        ]);
    }
}
