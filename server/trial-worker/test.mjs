import { validDeviceId } from "./src/index.js";

function requireOk(ok, name) {
  if (!ok) throw new Error(name);
}

requireOk(validDeviceId("MC-12345678-90ABCDEF-12345678"), "stable id accepted");
requireOk(!validDeviceId("MC-FALLBACK-12345678-1234-1234-1234-123456789012"), "fallback rejected");
requireOk(!validDeviceId("MC-12345678-90abcdef-12345678"), "lowercase rejected before normalization");
requireOk(!validDeviceId(""), "empty rejected");
console.log("trial worker identity tests passed");
