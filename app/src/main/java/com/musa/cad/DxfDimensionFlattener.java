package com.musa.cad;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

/**
 * Replaces DXF DIMENSION records with the real graphics stored in their anonymous *D blocks.
 * LibreDWG emits those graphics in the current coordinate system, so they must not be rebuilt
 * by connecting DIMENSION definition points (10/13/14).
 */
final class DxfDimensionFlattener {
    private static final class Record {
        final ArrayList<String> lines=new ArrayList<>();
        String type="";
        String text(int wanted,String fallback){
            for(int i=0;i+1<lines.size();i+=2){
                try{if(Integer.parseInt(lines.get(i).trim())==wanted)return lines.get(i+1);}catch(NumberFormatException ignored){}
            }
            return fallback;
        }
        private Record copy(){Record c=new Record();c.type=type;c.lines.addAll(lines);return c;}
        private int indexOf(int wanted){
            for(int i=0;i+1<lines.size();i+=2){
                try{if(Integer.parseInt(lines.get(i).trim())==wanted)return i;}catch(NumberFormatException ignored){}
            }
            return -1;
        }
        private void setTag(int code,String value){
            int i=indexOf(code);
            if(i>=0)lines.set(i+1,value);
            else{lines.add(Integer.toString(code));lines.add(value);}
        }
        Record withInheritedStyle(Record parent){
            String inherited=parent.text(8,"0").trim();Record result=this;boolean copied=false;
            int layerIndex=indexOf(8);
            if(!inherited.isEmpty()&&!"0".equals(inherited)&&(layerIndex<0||"0".equals(lines.get(layerIndex+1).trim()))){
                result=copy();copied=true;result.setTag(8,inherited);
            }
            int trueIndex=result.indexOf(420),aciIndex=result.indexOf(62);
            if(trueIndex<0&&aciIndex>=0&&"0".equals(result.lines.get(aciIndex+1).trim())){
                if(!copied){result=result.copy();copied=true;}
                String parentTrue=parent.text(420,"").trim();
                if(!parentTrue.isEmpty()){result.setTag(420,parentTrue);result.setTag(62,"256");}
                else{
                    String parentAci=parent.text(62,"256").trim();
                    if("0".equals(parentAci)||parentAci.isEmpty())parentAci="256";
                    result.setTag(62,parentAci);
                }
            }
            return result;
        }
    }

    private static final class RecordReader {
        final BufferedReader reader;String pendingCode,pendingValue;
        RecordReader(BufferedReader reader){this.reader=reader;}
        Record next()throws IOException{
            String codeLine=pendingCode,value=pendingValue;pendingCode=pendingValue=null;
            if(codeLine==null){codeLine=reader.readLine();if(codeLine==null)return null;value=reader.readLine();}
            if(value==null)throw new IOException("Eksik DXF etiketi");
            Record r=new Record();r.lines.add(codeLine);r.lines.add(value);
            int first=parseCode(codeLine);r.type=first==0?value.trim():"";
            while(true){
                String nextCode=reader.readLine();if(nextCode==null)break;
                String nextValue=reader.readLine();if(nextValue==null)throw new IOException("Eksik DXF etiketi");
                if(parseCode(nextCode)==0){pendingCode=nextCode;pendingValue=nextValue;break;}
                r.lines.add(nextCode);r.lines.add(nextValue);
            }
            return r;
        }
    }

    static File flatten(File input,File cache)throws IOException{
        FileTransfer.checkCancelled();
        Map<String,List<Record>> blocks=collectDimensionBlocks(input);
        if(blocks.isEmpty())return input;
        File output=File.createTempFile("MusaCAD_dim_",".dxf",cache);
        boolean ok=false;
        try(BufferedReader reader=reader(input);BufferedWriter writer=writer(output)){
            RecordReader records=new RecordReader(reader);Record record;
            while((record=records.next())!=null){
                FileTransfer.checkCancelled();
                if("DIMENSION".equals(record.type))writeDimension(record,writer,blocks,new HashSet<>(),0);
                else write(record,writer);
            }
            ok=true;return output;
        }finally{if(!ok)output.delete();}
    }

    private static Map<String,List<Record>> collectDimensionBlocks(File input)throws IOException{
        HashMap<String,List<Record>> blocks=new HashMap<>();
        try(BufferedReader reader=reader(input)){
            RecordReader records=new RecordReader(reader);
            String section="";String activeName=null;ArrayList<Record> activeMembers=null;Record record;
            while((record=records.next())!=null){
                FileTransfer.checkCancelled();
                if("SECTION".equals(record.type)){section=record.text(2,"").trim();continue;}
                if("ENDSEC".equals(record.type)){section="";activeName=null;activeMembers=null;continue;}
                if(!"BLOCKS".equals(section))continue;
                if("BLOCK".equals(record.type)){
                    activeName=key(record.text(2,"").trim());
                    activeMembers=activeName.startsWith("*D")?new ArrayList<>():null;
                    continue;
                }
                if("ENDBLK".equals(record.type)){
                    if(activeMembers!=null&&!activeName.isEmpty())blocks.put(activeName,activeMembers);
                    activeName=null;activeMembers=null;continue;
                }
                if(activeMembers!=null)activeMembers.add(record);
            }
        }
        return blocks;
    }

    private static void writeDimension(Record dimension,BufferedWriter writer,Map<String,List<Record>> blocks,Set<String> stack,int depth)throws IOException{
        if(depth>=32)return;
        String name=key(dimension.text(2,"").trim());List<Record> members=blocks.get(name);
        if(members==null||members.isEmpty()||!stack.add(name))return;
        try{
            for(Record raw:members){
                FileTransfer.checkCancelled();Record member=raw.withInheritedStyle(dimension);
                if("DIMENSION".equals(member.type))writeDimension(member,writer,blocks,stack,depth+1);else write(member,writer);
            }
        }finally{stack.remove(name);}
    }

    private static int parseCode(String line)throws IOException{
        try{return Integer.parseInt(line.trim());}catch(NumberFormatException e){throw new IOException("Geçersiz ASCII DXF etiketi",e);}
    }
    private static void write(Record r,BufferedWriter writer)throws IOException{
        for(String line:r.lines){writer.write(line);writer.newLine();}
    }
    private static BufferedReader reader(File f)throws IOException{return new BufferedReader(new InputStreamReader(new FileInputStream(f),StandardCharsets.ISO_8859_1),128*1024);}
    private static BufferedWriter writer(File f)throws IOException{return new BufferedWriter(new OutputStreamWriter(new FileOutputStream(f),StandardCharsets.ISO_8859_1),128*1024);}
    private static String key(String s){return s.toUpperCase(Locale.ROOT);}
    private DxfDimensionFlattener(){}
}
