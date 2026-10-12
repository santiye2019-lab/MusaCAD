package com.musa.cad;

/**
 * Read-only ACIS transport diagnostics. Entity parity or nonempty SAT/SAB chunks
 * must NEVER be treated as proof of B-Rep/topology/rendering fidelity.
 *
 * Source counters include all DWG objects whereas the DXF audit follows emitted
 * placements (including expanded INSERTs). A count mismatch is only a warning:
 * the two counts do not necessarily refer to identical object populations.
 */
public final class AcisConversionDiagnostic {
    public static String status(long sourceEntities,long sourceExternalAcis,
                                DxfAcisCoverage.Snapshot dxf) {
        long source=Math.max(0L,sourceEntities);
        long external=Math.max(0L,sourceExternalAcis);
        int exported=dxf==null?0:dxf.entityCount();
        if(source==0&&exported==0&&external==0)return "";
        StringBuilder text=new StringBuilder();
        if(source>0)text.append("ACIS DWG ").append(source).append(" / DXF ").append(exported);
        else text.append("ACIS DXF ").append(exported);
        if(source>exported)text.append("  •  nesne sayıları farklı");
        if(dxf!=null){
            if(dxf.withoutInlineData>0)
                text.append("  •  gövde verisi eksik ").append(dxf.withoutInlineData);
            if(dxf.invalidHexChunks>0)
                text.append("  •  bozuk SAB parçası ").append(dxf.invalidHexChunks);
        }
        if(external>0)text.append("  •  harici AcDs ").append(external);
        text.append("  •  3B geometri doğrulanmadı");
        return text.toString();
    }

    private AcisConversionDiagnostic(){}
}
