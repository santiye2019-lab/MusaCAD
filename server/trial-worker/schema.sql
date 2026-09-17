CREATE TABLE IF NOT EXISTS trials (
  device_id TEXT PRIMARY KEY,
  package_name TEXT NOT NULL,
  started_at_ms INTEGER NOT NULL,
  expires_at_ms INTEGER NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_trials_expires_at ON trials(expires_at_ms);
