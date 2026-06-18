<?php

declare(strict_types=1);

define('API_ROOT', __DIR__);
define('STORAGE_DIR', API_ROOT . '/storage');
define('DEFAULT_DEVICE_API_KEY', 'a7f3c9e2b18d4f650e9a1c3d5e7b9f12');

require_once API_ROOT . '/core/Response.php';
require_once API_ROOT . '/core/Request.php';
require_once API_ROOT . '/core/FileStore.php';
require_once API_ROOT . '/core/Auth.php';
require_once API_ROOT . '/core/Router.php';
require_once API_ROOT . '/controllers/StatusController.php';
require_once API_ROOT . '/controllers/CommandController.php';
require_once API_ROOT . '/controllers/DeviceController.php';
require_once API_ROOT . '/controllers/AuthController.php';
require_once API_ROOT . '/controllers/LogController.php';

function ensureStorage(): void
{
    $dirs = [
        STORAGE_DIR,
        STORAGE_DIR . '/status',
        STORAGE_DIR . '/commands',
        STORAGE_DIR . '/logs',
    ];

    foreach ($dirs as $dir) {
        if (!is_dir($dir)) {
            mkdir($dir, 0755, true);
        }
    }

    $files = [
        STORAGE_DIR . '/config.json' => json_encode([
            'app_name' => 'Karavan API',
            'version' => '1.0.0',
            'token_ttl' => 86400,
            'max_log_lines' => 5000,
            'max_history' => 200,
        ], JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE),
        STORAGE_DIR . '/devices.json' => json_encode([
            'karavan-1' => [
                'id' => 'karavan-1',
                'name' => 'Efsane Karavan',
                'api_key' => DEFAULT_DEVICE_API_KEY,
                'enabled' => true,
                'created_at' => date('c'),
            ],
        ], JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE),
        STORAGE_DIR . '/users.json' => json_encode([
            [
                'username' => 'admin',
                'password_hash' => password_hash('admin123', PASSWORD_DEFAULT),
                'role' => 'admin',
            ],
        ], JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE),
        STORAGE_DIR . '/tokens.json' => json_encode([], JSON_PRETTY_PRINT),
    ];

    foreach ($files as $path => $default) {
        if (!file_exists($path)) {
            file_put_contents($path, $default);
        }
    }
}

ensureStorage();
