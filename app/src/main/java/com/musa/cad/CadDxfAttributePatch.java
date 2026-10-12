package com.musa.cad;

import java.io.*;
import java.nio.charset.Charset;
import java.util.*;

/**
 * Loss-minimizing DXF INSERT/ATTRIB value patcher.
 * Only group-code 1 of an ATTRIB belonging to a specifically identified
 * root INSERT handle may change. Does not implement native DWG writing.
 *
 * Call with a private staging target: while requests are preflighted before
 * the first output byte, a storage error during the second pass may still
 * leave the supplied output stream partial.
 */
public final class CadDxfAttributePatch {
    private CadDxfAttributePatch() {}
    private static final int MAX_RECORD_PAIRS = 50000;

    public static final class Change {
        public final String insertHandle, tag, value;
        public Change(String insertHandle, String tag, String value) {
            this.insertHandle = required(insertHandle, "INSERT handle");
            this.tag = required(tag, "ATTRIB tag");
            if (value == null || value.indexOf('\n') >= 0 || value.indexOf('\r') >= 0
                || value.indexOf('\0') >= 0 || value.length() > 8192)
                throw new IllegalArgumentException("Invalid attribute value");
            this.value = value;
        }
    }
    private static String required(String input, String description) {
        if (input == null || input.trim().isEmpty() || input.length() > 255
            || input.indexOf('\n') >= 0 || input.indexOf('\r') >= 0)
            throw new IllegalArgumentException("Invalid " + description);
        return input.trim().toUpperCase(Locale.ROOT);
    }
    private static String key(String handle, String tag) {
        return handle.toUpperCase(Locale.ROOT) + "\u0000" + tag.toUpperCase(Locale.ROOT);
    }
    private static final class Pair {
        final int code; final String rawCode, value;
        Pair(String rawCode, String value) throws IOException {
            this.rawCode = rawCode; this.value = value;
            try { code = Integer.parseInt(rawCode.trim()); }
            catch (NumberFormatException e) { throw new IOException("Malformed DXF code", e); }
        }
    }
    private static final class Record {
        final String type;
        final ArrayList<Pair> pairs = new ArrayList<>();
        Record(Pair first) { type = first.value.trim(); pairs.add(first); }
        String text(int code) {
            for (Pair pair : pairs) if (pair.code == code) return pair.value.trim();
            return "";
        }
    }
    private static final class Records implements Closeable {
        private final BufferedReader reader;
        private Pair lookahead;
        private boolean finished;
        Records(File file, Charset charset) throws IOException {
            reader = new BufferedReader(new InputStreamReader(new FileInputStream(file), charset), 65536);
        }
        private Pair readPair() throws IOException {
            String rawCode = reader.readLine();
            if (rawCode == null) return null;
            String value = reader.readLine();
            if (value == null) throw new IOException("Incomplete DXF tag pair");
            return new Pair(rawCode, value);
        }
        Record next() throws IOException {
            if (finished) return null;
            Pair first = lookahead == null ? readPair() : lookahead;
            lookahead = null;
            if (first == null) return null;
            if (first.code != 0) throw new IOException("DXF record without leading 0 code");
            Record record = new Record(first);
            while (true) {
                Pair pair = readPair();
                if (pair == null) { finished = true; break; }
                if (pair.code == 0) { lookahead = pair; break; }
                if (record.pairs.size() >= MAX_RECORD_PAIRS)
                    throw new IOException("Excessively large DXF record");
                record.pairs.add(pair);
            }
            return record;
        }
        public void close() throws IOException { reader.close(); }
    }
    private static final class Pass {
        int replaced;
        final HashMap<String,Integer> hits = new HashMap<>();
    }

    public static int write(File source, OutputStream target, Charset charset,
                            List<Change> requested) throws IOException {
        if (source == null || !source.isFile() || target == null || charset == null)
            throw new IOException("DXF source/target/encoding required");
        if (requested == null || requested.isEmpty()) throw new IOException("No attribute changes");
        LinkedHashMap<String,Change> changes = new LinkedHashMap<>();
        for (Change item : requested) {
            if (item == null) throw new IOException("Null attribute change");
            String id = key(item.insertHandle, item.tag);
            if (changes.putIfAbsent(id, item) != null) throw new IOException("Duplicate attribute request");
        }
        // Entire source is parsed and all edits are uniquely found before
        // caller's destination receives any bytes.
        Pass check = process(source, charset, changes, null);
        for (String id : changes.keySet())
            if (check.hits.getOrDefault(id, 0) != 1)
                throw new IOException("Unresolved or ambiguous INSERT/ATTRIB " + id.replace('\0', '/'));
        long length = source.length(), modified = source.lastModified();
        BufferedWriter out = new BufferedWriter(new OutputStreamWriter(target, charset), 65536);
        Pass written = process(source, charset, changes, out);
        if (source.length() != length || source.lastModified() != modified
            || written.replaced != changes.size())
            throw new IOException("DXF source changed during attribute patch");
        out.flush();
        return written.replaced;
    }

    private static Pass process(File source, Charset cs, Map<String,Change> changes,
                                BufferedWriter out) throws IOException {
        Pass pass = new Pass();
        String section = "", activeInsert = null;
        boolean endSeen = false, inSection = false;
        try (Records records = new Records(source, cs)) {
            Record record;
            while ((record = records.next()) != null) {
                FileTransfer.checkCancelled();
                if (endSeen) throw new IOException("Unexpected data after DXF EOF");
                if ("SECTION".equals(record.type)) {
                    if (inSection) throw new IOException("Nested DXF SECTION");
                    inSection = true;
                    section = record.text(2).toUpperCase(Locale.ROOT);
                } else if ("ENDSEC".equals(record.type)) {
                    if (!inSection) throw new IOException("DXF ENDSEC without SECTION");
                    inSection = false; section = ""; activeInsert = null;
                } else if ("EOF".equals(record.type)) {
                    if (inSection) throw new IOException("DXF EOF inside SECTION");
                    endSeen = true;
                }
                if ("ENTITIES".equals(section) && "INSERT".equals(record.type)) {
                    activeInsert = record.text(5).toUpperCase(Locale.ROOT);
                } else if ("ENTITIES".equals(section) && "ATTRIB".equals(record.type)) {
                    if (activeInsert != null && !activeInsert.isEmpty()) {
                        String id = key(activeInsert, record.text(2));
                        Change change = changes.get(id);
                        if (change != null) {
                            int count = pass.hits.getOrDefault(id, 0) + 1;
                            pass.hits.put(id, count);
                            if (count > 1) throw new IOException("Duplicate matching ATTRIB");
                            int valueCount = 0;
                            for (Pair pair : record.pairs) if (pair.code == 1) valueCount++;
                            if (valueCount != 1) throw new IOException("Missing/ambiguous ATTRIB text");
                            if (out != null) pass.replaced++;
                        }
                    }
                } else if ("ENTITIES".equals(section) && !"SEQEND".equals(record.type)
                           && !"SECTION".equals(record.type) && !"ENDSEC".equals(record.type)) {
                    activeInsert = null;
                } else if ("SEQEND".equals(record.type)) {
                    activeInsert = null;
                }
                if (out != null) {
                    String id = "ENTITIES".equals(section) && "ATTRIB".equals(record.type)
                        && activeInsert != null ? key(activeInsert, record.text(2)) : null;
                    Change change = id == null ? null : changes.get(id);
                    for (Pair pair : record.pairs) {
                        out.write(pair.rawCode); out.newLine();
                        out.write(change != null && pair.code == 1 ? change.value : pair.value);
                        out.newLine();
                    }
                }
            }
        }
        if (!endSeen) throw new IOException("DXF EOF missing");
        return pass;
    }
}
