# Realtime Streaming Architecture

## Overview

This document describes the planned realtime market data architecture for AlgoTradeX. The goal is to separate realtime market feed handling from the arbitrage engine, while preserving the existing REST pipeline.

## Core components

- `IWebSocketClient`
  - Abstract interface for a websocket transport layer.
  - Responsible for connection lifecycle, message send/receive, and reconnect triggers.

- `IStreamManager`
  - Abstract interface for stream orchestration.
  - Responsible for subscribing to Binance streams, managing symbol lists, and delivering raw event payloads.

- `IMarketDataCache`
  - Abstract interface for realtime state storage.
  - Responsible for thread-safe symbol snapshot updates and retrieval.

## Configuration

- `WebSocketClientConfig`
  - `uri` - target Binance websocket base endpoint.
  - `initial_streams` - initial market data channels.
  - `reconnect_attempts`, `reconnect_backoff_ms`, `auto_reconnect`.

- `StreamManagerConfig`
  - `symbols` - subscribed market symbols.
  - `depth` - optional orderbook depth for incoming stream processing.
  - `update_interval_ms` - processing cadence.

- `MarketDataCacheConfig`
  - `max_symbols` - cache capacity.
  - `keep_snapshot_history` - whether to retain recent snapshots.
  - `history_depth` - number of snapshots to preserve.

- `RealtimeConfig`
  - Aggregates websocket, stream, and cache settings.
  - Includes `enable_realtime_mode`, `scan_interval_ms`, `venue_name`, `app_name`.

## Planned data flow

1. `IWebSocketClient` opens a Binance websocket connection.
2. `IStreamManager` subscribes to market channels and parses raw updates.
3. Parsed market events are forwarded into `IMarketDataCache`.
4. Algorithm engine consumes the latest `MarketSnapshot` from the cache.

## Phase 1 architecture decision

- Keep the current REST live scanning path intact.
- Introduce abstract interfaces first to avoid coupling the core engine to any specific websocket library.
- Use configuration objects to centralize realtime parameters and support runtime mode switching.
- Defer implementation details (networking, concurrency, parsing) to later commits.
