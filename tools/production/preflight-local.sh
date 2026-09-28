#!/usr/bin/env bash
set -euo pipefail

missing=0
need_var() {
  local name="$1"
  if [ -z "${!name:-}" ]; then
    echo "MISSING: $name"
    missing=1
  else
    echo "OK: $name"
  fi
}

need_var MUSACAD_KEYSTORE_FILE
need_var MUSACAD_KEYSTORE_PASSWORD
need_var MUSACAD_KEY_ALIAS
need_var MUSACAD_KEY_PASSWORD
need_var MUSACAD_TRIAL_API_URL
need_var MUSACAD_TRIAL_PUBLIC_KEY_PEM

if [ "${1:-direct}" = "play" ]; then
  need_var MUSACAD_PLAY_VERIFY_URL
  need_var MUSACAD_PLAY_YEARLY_PRODUCT_ID
fi

if [ "$missing" -ne 0 ]; then
  exit 1
fi

[ -f "$MUSACAD_KEYSTORE_FILE" ] || { echo "Keystore not found: $MUSACAD_KEYSTORE_FILE"; exit 1; }
case "$MUSACAD_TRIAL_API_URL" in https://*) ;; *) echo "Trial API must use HTTPS"; exit 1;; esac
if [ "${1:-direct}" = "play" ]; then
  case "$MUSACAD_PLAY_VERIFY_URL" in https://*) ;; *) echo "Play verify API must use HTTPS"; exit 1;; esac
fi

keytool -list -keystore "$MUSACAD_KEYSTORE_FILE" -storepass "$MUSACAD_KEYSTORE_PASSWORD" -alias "$MUSACAD_KEY_ALIAS" >/dev/null
echo "Production environment preflight passed (no APK/AAB built)."
