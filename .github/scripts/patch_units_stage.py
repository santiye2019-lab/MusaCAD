from pathlib import Path


def patch(path,replacements):
    p=Path(path);s=p.read_text()
    for old,new,count in replacements:
        n=s.count(old)
        if n!=count: raise SystemExit(f'{path}: expected {count}, got {n}: {old[:180]!r}')
        s=s.replace(old,new,count)
    p.write_text(s)

patch('app/src/main/java/com/musa/cad/DxfParser.java',[
('''        public final int entityCount, layerCount, skippedCount;\n        public int conversionWarnings;\n''','''        public final int entityCount, layerCount, skippedCount;\n        public int conversionWarnings,insUnits;\n        public boolean automaticUnits;\n        public double unitsPerImagePixel=1d;\n        public String unitName="piksel";\n''',1),
('''            Result result=renderLayers(document,view,layerNames,visible,skippedCount,layoutNames,activeLayout);\n            result.conversionWarnings=conversionWarnings;\n            return result;\n''','''            Result result=renderLayers(document,view,layerNames,visible,skippedCount,layoutNames,activeLayout);\n            result.conversionWarnings=conversionWarnings;result.insUnits=insUnits;result.automaticUnits=automaticUnits;\n            result.unitsPerImagePixel=unitsPerImagePixel;result.unitName=unitName;\n            return result;\n''',1),
('''        DxfTextStyles.Table textStyles=DxfTextStyles.parse(lines);\n        DxfBlocks.Result expanded=DxfBlocks.expand(lines,preferredLayout);\n''','''        DxfTextStyles.Table textStyles=DxfTextStyles.parse(lines);\n        int insUnits=DxfUnits.parse(lines);\n        DxfBlocks.Result expanded=DxfBlocks.expand(lines,preferredLayout);\n''',1),
('''        return finishEntities(entities,layers,visibleLayers,skipped,expanded.layouts,expanded.activeLayout);\n    }\n\n    private interface StreamNode {}\n''','''        return applyUnits(finishEntities(entities,layers,visibleLayers,skipped,expanded.layouts,expanded.activeLayout),insUnits);\n    }\n\n    private interface StreamNode {}\n''',1),
('''    private static final class StreamContext {\n        String section="",rootSequenceLayout;StreamBlock activeBlock;PendingPoly pending;int skipped;\n''','''    private static final class StreamContext {\n        String section="",rootSequenceLayout;StreamBlock activeBlock;PendingPoly pending;int skipped,insUnits;\n''',1),
('''        return finishEntities(entities,layers,visibleLayers,context.skipped,context.layouts,activeLayout);\n    }\n''','''        return applyUnits(finishEntities(entities,layers,visibleLayers,context.skipped,context.layouts,activeLayout),context.insUnits);\n    }\n''',1),
('''            if("HEADER".equals(c.section)){\n                for(int i=0;i+1<r.tags.size();i+=2)if(intOf(r.tags.get(i))==9&&"$LTSCALE".equalsIgnoreCase(r.tags.get(i+1).trim())){\n''','''            if("HEADER".equals(c.section)){\n                c.insUnits=DxfUnits.parseHeaderRecord(r.tags);\n                for(int i=0;i+1<r.tags.size();i+=2)if(intOf(r.tags.get(i))==9&&"$LTSCALE".equalsIgnoreCase(r.tags.get(i+1).trim())){\n''',1),
('''    private static String key(String value){return value.toUpperCase(Locale.ROOT);}\n\n    private static Result finishEntities''','''    private static String key(String value){return value.toUpperCase(Locale.ROOT);}\n\n    private static Result applyUnits(Result result,int insUnits){\n        if(result==null)return null;result.insUnits=insUnits;\n        if(!DxfSpace.isModel(result.activeLayout)||!DxfUnits.known(insUnits))return result;\n        float[] vectors={1,0,0,1};result.view.mapVectors(vectors);\n        double scale=(Math.hypot(vectors[0],vectors[1])+Math.hypot(vectors[2],vectors[3]))*.5;\n        if(Double.isFinite(scale)&&scale>1e-12){\n            result.unitsPerImagePixel=1d/scale;result.unitName=DxfUnits.symbol(insUnits);result.automaticUnits=true;\n        }\n        return result;\n    }\n\n    private static Result finishEntities''',1),
])

patch('app/src/main/java/com/musa/cad/CadView.java',[
('''        drawing=null;vectorDrawing=result;unitsPerImagePixel=1;unitName="piksel";mode=Mode.PAN;points.clear();\n''','''        drawing=null;vectorDrawing=result;\n        unitsPerImagePixel=result.automaticUnits?result.unitsPerImagePixel:1d;unitName=result.automaticUnits?result.unitName:"piksel";\n        mode=Mode.PAN;points.clear();\n''',1),
])

patch('app/src/main/java/com/musa/cad/MainActivity.java',[
('''            "Görünür katman: "+activeDxf.visibleLayers.size()+"\\n"+\n            "Görüntüleme: vektörel / net yakınlaştırma";\n''','''            "Görünür katman: "+activeDxf.visibleLayers.size()+"\\n"+\n            "Ölçü birimi: "+(activeDxf.automaticUnits?DxfUnits.name(activeDxf.insUnits)+" (otomatik)":"kalibrasyon gerekli")+"\\n"+\n            "Görüntüleme: vektörel / net yakınlaştırma";\n''',1),
])
