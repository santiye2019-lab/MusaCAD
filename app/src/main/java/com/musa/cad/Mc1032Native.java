package com.musa.cad;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * MC1032 is MusaCAD's OWN native binary file format, not Autodesk DWG/AC1032.
 * Only file I/O and JNI transportation live here. Encoding, geometry,
 * integrity verification and identity checks are implemented in one C core.
 *
 * A flat native entity has nine exact 64-bit words:
 * [unsigned handle bits, type(1 LINE/2 CIRCLE/3 ARC/4 POINT),
 *  x1 bits, y1 bits, x2 bits, y2 bits, radius bits, start bits, end bits].
 * Numeric coordinate fields are Double.doubleToRawLongBits values.
 */
public final class Mc1032Native {
    public static final int MAX_BYTES = 32 + 72 * 4096;
    private static final boolean available;
    static {
        boolean loaded = false;
        try { System.loadLibrary("musacad_dwg"); loaded = true; }
        catch (LinkageError | SecurityException ignored) {
            // Never install a Java/third-party CAD codec fallback.
        }
        available = loaded;
    }
    private static native long[] decodeNative(byte[] source) throws IOException;
    private static native byte[] encodeNative(long[] entities) throws IOException;

    public static long[] decode(byte[] source) throws IOException {
        if (!available) throw new IOException("MC1032 ortak CAD cekirdegi yuklenemedi");
        if (source == null || source.length < 32 || source.length > MAX_BYTES)
            throw new IOException("MC1032 dosya siniri veya basligi gecersiz");
        try { return decodeNative(source); }
        catch (LinkageError missing) {
            throw new IOException("MC1032 JNI okuyucu bulunamadi", missing);
        }
    }

    public static byte[] encode(long[] packedEntities) throws IOException {
        if (!available) throw new IOException("MC1032 ortak CAD cekirdegi yuklenemedi");
        if (packedEntities == null || packedEntities.length % 9 != 0 ||
                packedEntities.length / 9 > 4096)
            throw new IOException("MC1032 nesne dizisi gecersiz");
        try { return encodeNative(packedEntities); }
        catch (LinkageError missing) {
            throw new IOException("MC1032 JNI yazici bulunamadi", missing);
        }
    }

    public static long[] read(File file) throws IOException {
        if (file == null || !file.isFile()) throw new IOException("MC1032 dosyasi yok");
        if (file.length() > MAX_BYTES || file.length() < 32)
            throw new IOException("MC1032 dosya boyutu gecersiz");
        try (FileInputStream input = new FileInputStream(file);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] block = new byte[8192];
            int count;
            while ((count = input.read(block)) >= 0) {
                if (count == 0) continue;
                if (output.size() > MAX_BYTES - count)
                    throw new IOException("MC1032 bellek siniri asildi");
                output.write(block, 0, count);
            }
            return decode(output.toByteArray());
        }
    }

    /**
     * Write to a NEW uniquely named .mcad file. Never overwrite source or
     * another drawing. fsync before returning to the platform file layer.
     */
    public static File writeNew(File directory, long[] packedEntities) throws IOException {
        if (directory == null || !directory.isDirectory())
            throw new IOException("MC1032 kayit klasoru bulunamadi");
        byte[] bytes = encode(packedEntities);
        File destination = File.createTempFile("MusaCAD-MC1032-", ".mcad", directory);
        boolean complete = false;
        try (FileOutputStream output = new FileOutputStream(destination)) {
            output.write(bytes);
            output.flush();
            output.getFD().sync();
            complete = true;
        } finally {
            if (!complete) destination.delete();
        }
        return destination;
    }
    private Mc1032Native() {}
}
