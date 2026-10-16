package com.musa.cad;

import java.util.List;

/**
 * Human-readable XREF inventory; diagnostic only. Nothing in this class
 * opens external files, trusts an XREF path, or claims it has been resolved.
 */
public final class CadXrefSummary {
    private static final int MAX_ROWS = 12;
    private CadXrefSummary() {}

    public static String describe(List<DxfBlocks.ExternalReference> references) {
        if (references == null || references.isEmpty()) return "Harici CAD referansı yok.";
        int attachmentCount = 0, overlayCount = 0;
        for (DxfBlocks.ExternalReference ref : references) {
            if (ref == null) continue;
            if (ref.overlay) overlayCount++; else attachmentCount++;
        }
        StringBuilder out = new StringBuilder(256);
        out.append("XREF listesi (yüklenmemiş): ").append(references.size())
           .append("  •  Ekli: ").append(attachmentCount)
           .append("  •  Overlay: ").append(overlayCount)
           .append("\nHarici çizimler içeri aktarılmadı; gösterilen yollar yalnızca dosyadan okunan bilgilerdir.");
        int shown = Math.min(MAX_ROWS, references.size());
        for (int i = 0; i < shown; i++) {
            DxfBlocks.ExternalReference ref = references.get(i);
            out.append("\n").append(i + 1).append(". ");
            if (ref == null) { out.append("(geçersiz kayıt)"); continue; }
            String name = safeText(ref.name, 70);
            String path = safeText(ref.path, 160);
            out.append(name.isEmpty() ? "(isimsiz)" : name)
               .append(ref.overlay ? " [overlay]" : " [attach]")
               .append("\n   ")
               .append(path.isEmpty() ? "(dosya yolu bildirilmemiş)" : path);
        }
        if (references.size() > shown) {
            out.append("\n… ").append(references.size() - shown).append(" ek referans listelenmedi.");
        }
        return out.toString();
    }

    static String safeText(String raw, int maxCodePoints) {
        if (raw == null || maxCodePoints <= 0) return "";
        StringBuilder out = new StringBuilder(Math.min(raw.length(), maxCodePoints));
        int appended = 0;
        for (int i = 0; i < raw.length() && appended < maxCodePoints;) {
            int codePoint = raw.codePointAt(i);
            i += Character.charCount(codePoint);
            byte direction = Character.getDirectionality(codePoint);
            if (Character.isISOControl(codePoint)
                || direction == Character.DIRECTIONALITY_RIGHT_TO_LEFT_OVERRIDE
                || direction == Character.DIRECTIONALITY_LEFT_TO_RIGHT_OVERRIDE
                || direction == Character.DIRECTIONALITY_RIGHT_TO_LEFT_EMBEDDING
                || direction == Character.DIRECTIONALITY_LEFT_TO_RIGHT_EMBEDDING
                || direction == Character.DIRECTIONALITY_POP_DIRECTIONAL_FORMAT
                || codePoint == 0x2066 || codePoint == 0x2067
                || codePoint == 0x2068 || codePoint == 0x2069) {
                out.append(' ');
            } else {
                out.appendCodePoint(codePoint);
            }
            appended++;
        }
        if (raw.codePointCount(0, raw.length()) > maxCodePoints) out.append('…');
        return out.toString().trim();
    }
}
