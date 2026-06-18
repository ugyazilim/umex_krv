<?php

declare(strict_types=1);

final class FileStore
{
    public static function readJson(string $path, mixed $default = []): mixed
    {
        if (!file_exists($path)) {
            return $default;
        }

        $content = file_get_contents($path);
        if ($content === false || trim($content) === '') {
            return $default;
        }

        $data = json_decode($content, true);
        return json_last_error() === JSON_ERROR_NONE ? $data : $default;
    }

    public static function writeJson(string $path, mixed $data): bool
    {
        $dir = dirname($path);
        if (!is_dir($dir)) {
            mkdir($dir, 0755, true);
        }

        $json = json_encode($data, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
        if ($json === false) {
            return false;
        }

        return self::writeAtomic($path, $json);
    }

    public static function appendLine(string $path, array $row): bool
    {
        $dir = dirname($path);
        if (!is_dir($dir)) {
            mkdir($dir, 0755, true);
        }

        $line = json_encode($row, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
        if ($line === false) {
            return false;
        }

        $fp = fopen($path, 'ab');
        if ($fp === false) {
            return false;
        }

        $ok = false;
        if (flock($fp, LOCK_EX)) {
            $ok = fwrite($fp, $line . PHP_EOL) !== false;
            flock($fp, LOCK_UN);
        }
        fclose($fp);

        return $ok;
    }

    public static function readLines(string $path, int $limit = 100, bool $reverse = true): array
    {
        if (!file_exists($path)) {
            return [];
        }

        $lines = file($path, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES) ?: [];
        if ($reverse) {
            $lines = array_reverse($lines);
        }

        $result = [];
        foreach (array_slice($lines, 0, $limit) as $line) {
            $row = json_decode($line, true);
            if (is_array($row)) {
                $result[] = $row;
            }
        }

        return $result;
    }

    public static function trimLog(string $path, int $maxLines): void
    {
        if (!file_exists($path)) {
            return;
        }

        $lines = file($path, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES) ?: [];
        if (count($lines) <= $maxLines) {
            return;
        }

        $keep = array_slice($lines, -$maxLines);
        self::writeAtomic($path, implode(PHP_EOL, $keep) . PHP_EOL);
    }

    private static function writeAtomic(string $path, string $content): bool
    {
        $tmp = $path . '.' . uniqid('tmp', true);
        if (file_put_contents($tmp, $content, LOCK_EX) === false) {
            return false;
        }
        return rename($tmp, $path);
    }
}
