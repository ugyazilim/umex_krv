<?php

declare(strict_types=1);

final class LogController
{
    private static function logFile(): string
    {
        return STORAGE_DIR . '/logs/system.txt';
    }

    public static function write(string $type, string $ref, array $meta = []): void
    {
        FileStore::appendLine(self::logFile(), [
            'at' => date('c'),
            'type' => $type,
            'ref' => $ref,
            'ip' => Request::ip(),
            'meta' => $meta,
        ]);

        $config = FileStore::readJson(STORAGE_DIR . '/config.json', []);
        $max = (int) ($config['max_log_lines'] ?? 5000);
        FileStore::trimLog(self::logFile(), $max);
    }

    public static function list(): void
    {
        Auth::requireAdmin();
        $limit = min(1000, max(1, (int) Request::query('limit', 100)));
        $type = Request::query('type');

        $rows = FileStore::readLines(self::logFile(), $limit, true);

        if ($type) {
            $rows = array_values(array_filter($rows, fn ($r) => ($r['type'] ?? '') === $type));
        }

        Response::success([
            'count' => count($rows),
            'logs' => $rows,
        ]);
    }
}
