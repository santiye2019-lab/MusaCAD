package com.musa.cad;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Thin Android transport for the SAME portable first-party MC1032 v2 C core
 * used on Windows, Linux, macOS and iOS. No Java CAD geometry/format parser.
 *
 * Native flat records, nine raw long words:
 * [handle, kind (1 LINE3D / 2 POINT3D), layer ID,
 *  x1 bits, y1 bits, z1 bits, x2 bits, y2 bits, z2 bits].
 * Names include default layer "0", with opaque-white color 0xffffffff.
 * Missing native core fails closed with NO external CAD codec fallback.
 */
public final class Mc1032V2Native {
    public static final int MAX_BYTES = 48 + 64 * 64 + 80 * 4096;
    private static final boolean loaded;
    static {
        boolean active = false;
        try { System.loadLibrary("musacad_dwg"); active = true; }
        catch (LinkageError | SecurityException unavailable) { }
        loaded = active;
    }
    private static native Object[] decodeNative(byte[] input) throws IOException;
    private static native byte[] encodeNative(String[] names,int[] rgba,
                                               long[] entities) throws IOException;

    public static final class Drawing {
        public final String[] names;
        public final int[] colors;
        public final long[] entities;
        public Drawing(String[] names,int[] colors,long[] entities) {
            this.names = names;
            this.colors = colors;
            this.entities = entities;
        }
    }
    public static Drawing decode(byte[] bytes) throws IOException {
        if (!loaded) throw new IOException("MC1032 v2 ortak CAD cekirdegi yok");
        if (bytes == null || bytes.length < 48 || bytes.length > MAX_BYTES)
            throw new IOException("MC1032 v2 dosya boyutu gecersiz");
        try {
            Object[] data = decodeNative(bytes);
            if (data == null || data.length != 3)
                throw new IOException("MC1032 v2 eksik yerel veri");
            return new Drawing((String[])data[1],(int[])data[2],(long[])data[0]);
        } catch (LinkageError ex) {
            throw new IOException("MC1032 v2 JNI okuyucu yok",ex);
        }
    }
    public static byte[] encode(Drawing d) throws IOException {
        if (!loaded) throw new IOException("MC1032 v2 ortak CAD cekirdegi yok");
        if (d == null || d.names == null || d.colors == null ||
            d.entities == null || d.names.length < 1 ||
            d.names.length > 64 || d.names.length != d.colors.length ||
            d.entities.length % 9 != 0 || d.entities.length / 9 > 4096)
            throw new IOException("MC1032 v2 gecersiz katman/nesne listesi");
        try {
            return encodeNative(d.names,d.colors,d.entities);
        } catch (LinkageError ex) {
            throw new IOException("MC1032 v2 JNI yazici yok",ex);
        }
    }
    public static Drawing read(File file) throws IOException {
        if (file == null || !file.isFile() || file.length() < 48 ||
            file.length() > MAX_BYTES)
            throw new IOException("MC1032 v2 dosyasi yok ya da cok buyuk");
        try (FileInputStream input = new FileInputStream(file);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[8192];
            int n;
            while ((n = input.read(buffer)) != -1) {
                if (output.size() > MAX_BYTES - n)
                    throw new IOException("MC1032 v2 dosyasi fazla buyuk");
                output.write(buffer,0,n);
            }
            return decode(output.toByteArray());
        }
    }
    /* Save ONLY to a new unique file; never overwrite an existing source. */
    public static File writeNew(File folder,Drawing document) throws IOException {
        if (folder == null || !folder.isDirectory())
            throw new IOException("MC1032 v2 kayit klasoru yok");
        byte[] bytes = encode(document);
        File file = File.createTempFile("MusaCAD-MC1032-v2-", ".mcad", folder);
        boolean committed = false;
        try (FileOutputStream output = new FileOutputStream(file)) {
            output.write(bytes);
            output.flush();
            output.getFD().sync();
            committed = true;
        } finally {
            if (!committed) file.delete();
        }
        return file;
    }
    private Mc1032V2Native() {}
}
