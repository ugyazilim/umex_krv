<?php

declare(strict_types=1);

require_once __DIR__ . '/auth.php';
panel_require_login();

header('Content-Type: application/json; charset=utf-8');

$deviceId = (string) ($_GET['device_id'] ?? $_POST['device_id'] ?? 'karavan-1');
$method = strtoupper($_SERVER['REQUEST_METHOD'] ?? 'GET');

try {
    if ($method === 'GET' && ($_GET['action'] ?? '') === 'status') {
        $status = StatusController::readForDevice($deviceId);
        if ($status === null) {
            http_response_code(404);
            echo json_encode([
                'success' => false,
                'message' => 'Bu cihaz için henüz veri yok',
                'timestamp' => date('c'),
            ], JSON_UNESCAPED_UNICODE);
            exit;
        }

        echo json_encode([
            'success' => true,
            'message' => 'OK',
            'data' => $status,
            'timestamp' => date('c'),
        ], JSON_UNESCAPED_UNICODE);
        exit;
    }

    if ($method === 'GET' && ($_GET['action'] ?? '') === 'online') {
        $devices = FileStore::readJson(STORAGE_DIR . '/devices.json', []);
        $device = $devices[$deviceId] ?? null;
        if (!$device) {
            http_response_code(404);
            echo json_encode(['success' => false, 'message' => 'Cihaz bulunamadı'], JSON_UNESCAPED_UNICODE);
            exit;
        }

        $lastSeen = $device['last_seen'] ?? null;
        $secondsAgo = $lastSeen ? (time() - strtotime($lastSeen)) : null;
        $online = $secondsAgo !== null && $secondsAgo <= 120;

        echo json_encode([
            'success' => true,
            'data' => [
                'device_id' => $deviceId,
                'online' => $online,
                'last_seen' => $lastSeen,
                'seconds_ago' => $secondsAgo,
            ],
        ], JSON_UNESCAPED_UNICODE);
        exit;
    }

    if ($method === 'POST') {
        $body = json_decode(file_get_contents('php://input') ?: '', true);
        if (!is_array($body)) {
            $body = $_POST;
        }

        $action = (string) ($body['action'] ?? '');
        $params = is_array($body['params'] ?? null) ? $body['params'] : [];
        $targetDevice = (string) ($body['device_id'] ?? $deviceId);

        $command = CommandController::queueCommand(
            $targetDevice,
            $action,
            $params,
            panel_username()
        );

        http_response_code(201);
        echo json_encode([
            'success' => true,
            'message' => 'Komut kuyruğa eklendi',
            'data' => $command,
            'timestamp' => date('c'),
        ], JSON_UNESCAPED_UNICODE);
        exit;
    }

    http_response_code(405);
    echo json_encode(['success' => false, 'message' => 'Geçersiz istek'], JSON_UNESCAPED_UNICODE);
} catch (InvalidArgumentException $e) {
    http_response_code(422);
    echo json_encode(['success' => false, 'message' => $e->getMessage()], JSON_UNESCAPED_UNICODE);
} catch (Throwable $e) {
    http_response_code(500);
    echo json_encode(['success' => false, 'message' => 'Sunucu hatası'], JSON_UNESCAPED_UNICODE);
}
