<?php

declare(strict_types=1);

require_once __DIR__ . '/bootstrap.php';

header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Authorization, X-Api-Token, X-Device-Key');

$router = new Router();

// Sağlık kontrolü
$router->get('/', function () {
    $config = FileStore::readJson(STORAGE_DIR . '/config.json', []);
    Response::success([
        'name' => $config['app_name'] ?? 'Karavan API',
        'version' => $config['version'] ?? '1.0.0',
        'storage' => 'file',
        'endpoints' => [
            'POST /auth/login',
            'GET  /auth/me',
            'POST /status/push          (X-Device-Key)',
            'GET  /status/latest',
            'GET  /status/history',
            'GET  /status/online',
            'GET  /commands/poll        (X-Device-Key)',
            'POST /commands/ack         (X-Device-Key)',
            'POST /commands/send',
            'GET  /commands/list',
            'GET  /devices',
            'POST /devices',
            'GET  /logs',
        ],
    ]);
});

// Auth
$router->post('/auth/login', [AuthController::class, 'login']);
$router->get('/auth/me', [AuthController::class, 'me']);
$router->post('/auth/logout', [AuthController::class, 'logout']);

// Status (ESP → sunucu)
$router->post('/status/push', [StatusController::class, 'push']);
$router->get('/status/latest', [StatusController::class, 'latest']);
$router->get('/status/latest/{deviceId}', [StatusController::class, 'latest']);
$router->get('/status/history', [StatusController::class, 'history']);
$router->get('/status/history/{deviceId}', [StatusController::class, 'history']);
$router->get('/status/online', [StatusController::class, 'online']);
$router->get('/status/online/{deviceId}', [StatusController::class, 'online']);

// Commands (kuyruk sistemi)
$router->get('/commands/poll', [CommandController::class, 'poll']);
$router->post('/commands/ack', [CommandController::class, 'ack']);
$router->post('/commands/send', [CommandController::class, 'send']);
$router->post('/commands/send/{deviceId}', [CommandController::class, 'send']);
$router->get('/commands/list', [CommandController::class, 'list']);
$router->get('/commands/list/{deviceId}', [CommandController::class, 'list']);

// Devices
$router->get('/devices', [DeviceController::class, 'list']);
$router->get('/devices/{deviceId}', [DeviceController::class, 'show']);
$router->post('/devices', [DeviceController::class, 'create']);
$router->post('/devices/{deviceId}/regenerate-key', [DeviceController::class, 'regenerateKey']);

// Logs
$router->get('/logs', [LogController::class, 'list']);

try {
    $router->dispatch();
} catch (Throwable $e) {
    Response::error('Sunucu hatası', 500, [
        'error' => $e->getMessage(),
    ]);
}
