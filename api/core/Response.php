<?php

declare(strict_types=1);

final class Response
{
    public static function json(mixed $data, int $status = 200): void
    {
        http_response_code($status);
        header('Content-Type: application/json; charset=utf-8');
        header('X-Content-Type-Options: nosniff');
        echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
        exit;
    }

    public static function success(mixed $data = null, string $message = 'OK', int $status = 200): void
    {
        self::json([
            'success' => true,
            'message' => $message,
            'data' => $data,
            'timestamp' => date('c'),
        ], $status);
    }

    public static function error(string $message, int $status = 400, mixed $details = null): void
    {
        self::json([
            'success' => false,
            'message' => $message,
            'details' => $details,
            'timestamp' => date('c'),
        ], $status);
    }
}
