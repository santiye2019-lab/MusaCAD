CREATE TABLE IF NOT EXISTS trials (
  device_id TEXT PRIMARY KEY,
  package_name TEXT NOT NULL,
  started_at_ms INTEGER NOT NULL,
  expires_at_ms INTEGER NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_trials_expires_at ON trials(expires_at_ms);

CREATE TABLE IF NOT EXISTS play_purchases (
  token_hash TEXT PRIMARY KEY,
  device_id TEXT NOT NULL,
  package_name TEXT NOT NULL,
  product_id TEXT NOT NULL,
  order_id TEXT,
  verified_at_ms INTEGER NOT NULL,
  acknowledged_at_ms INTEGER,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_play_purchases_device_id ON play_purchases(device_id);
CREATE INDEX IF NOT EXISTS idx_play_purchases_order_id ON play_purchases(order_id);
