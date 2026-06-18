<?php

declare(strict_types=1);

final class Router
{
    /** @var array<string, array<string, callable>> */
    private array $routes = [];

    public function get(string $path, callable $handler): void
    {
        $this->add('GET', $path, $handler);
    }

    public function post(string $path, callable $handler): void
    {
        $this->add('POST', $path, $handler);
    }

    public function put(string $path, callable $handler): void
    {
        $this->add('PUT', $path, $handler);
    }

    public function delete(string $path, callable $handler): void
    {
        $this->add('DELETE', $path, $handler);
    }

    private function add(string $method, string $path, callable $handler): void
    {
        $this->routes[$method][$this->normalize($path)] = $handler;
    }

    public function dispatch(): void
    {
        $method = Request::method();
        $path = Request::path();

        if ($method === 'OPTIONS') {
            Response::json(['success' => true]);
        }

        $handler = $this->match($method, $path);
        if ($handler) {
            $handler($this->params);
            return;
        }

        Response::error('Endpoint bulunamadı: ' . $path, 404);
    }

    /** @var array<string, string> */
    private array $params = [];

    private function match(string $method, string $path): ?callable
    {
        $routes = $this->routes[$method] ?? [];
        $path = $this->normalize($path);

        if (isset($routes[$path])) {
            $this->params = [];
            return $routes[$path];
        }

        foreach ($routes as $pattern => $handler) {
            $regex = preg_replace('#\{([a-zA-Z_]+)\}#', '(?P<$1>[^/]+)', $pattern);
            $regex = '#^' . $regex . '$#';

            if (preg_match($regex, $path, $matches)) {
                $this->params = array_filter(
                    $matches,
                    fn ($k) => !is_int($k),
                    ARRAY_FILTER_USE_KEY
                );
                return $handler;
            }
        }

        return null;
    }

    private function normalize(string $path): string
    {
        $path = '/' . trim($path, '/');
        return $path === '/' ? '/' : rtrim($path, '/');
    }

    public function param(string $key, ?string $default = null): ?string
    {
        return $this->params[$key] ?? $default;
    }
}
