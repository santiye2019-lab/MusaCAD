// First-party thin Swift adapter for iOS / iPadOS.
// It exposes the SAME MusaCAD C ABI used on Android, Windows and macOS.
// This is NOT a finished iOS application, renderer or DWG-opening claim.
import MusaCADCore

public enum MusaCADCoreAdapter {
    /// The engine ABI is checked before a future UI accesses CAD operations.
    public static var abiVersion: UInt32 {
        musa_core_abi_version()
    }

    public static var supportsRequiredABI: Bool {
        abiVersion == UInt32(MUSA_CORE_ABI_VERSION)
    }
}
