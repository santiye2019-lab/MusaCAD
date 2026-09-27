package com.musa.cad;

import java.util.*;
import java.util.regex.*;

/** Converts extracted spreadsheet text into a bounded cell grid suitable for CAD placement. */
public final class CadSpreadsheetLayout {
    private static final int MAX_SHEETS=8,MAX_CELLS=2500,MAX_ROWS=250,MAX_COLS=50,MAX_CELL_CHARS=2000;
    private static final Pattern SHEET=Pattern.compile("^\\s*[—-]{1,2}\\s*Sayfa\\s+(\\d+)\\s*[—-]{1,2}\\s*$",Pattern.CASE_INSENSITIVE|Pattern.UNICODE_CASE);
    private static final Pattern CELL=Pattern.compile("^([A-Za-z]{1,3})([1-9][0-9]{0,5})\\t(.*)$");

    public static final class Cell {
        public final int row,col;public final String value;
        Cell(int row,int col,String value){this.row=row;this.col=col;this.value=value==null?"":value;}
    }
    public static final class Sheet {
        public final int index;public final String title;public final List<Cell> cells;public final int rows,columns;
        private final Map<Long,Cell> byAddress;
        Sheet(int index,String title,List<Cell> cells){
            this.index=index;this.title=title==null?"Sayfa "+index:title;
            ArrayList<Cell> copy=new ArrayList<>(cells);this.cells=Collections.unmodifiableList(copy);
            LinkedHashMap<Long,Cell> map=new LinkedHashMap<>();int r=0,c=0;
            for(Cell cell:copy){map.put(key(cell.row,cell.col),cell);r=Math.max(r,cell.row);c=Math.max(c,cell.col);}
            byAddress=Collections.unmodifiableMap(map);rows=r;columns=c;
        }
        public String valueAt(int row,int col){Cell c=byAddress.get(key(row,col));return c==null?"":c.value;}
        public boolean isEmpty(){return cells.isEmpty();}
    }

    private static final class MutableSheet{
        final int index;final String title;final LinkedHashMap<Long,Cell> cells=new LinkedHashMap<>();
        MutableSheet(int index,String title){this.index=index;this.title=title;}
        void put(int row,int col,String value){
            if(row<1||row>MAX_ROWS||col<1||col>MAX_COLS||cells.size()>=MAX_CELLS)return;
            String v=sanitize(value);cells.put(key(row,col),new Cell(row,col,v));
        }
        void append(int row,int col,String extra){
            long k=key(row,col);Cell old=cells.get(k);if(old==null)return;String add=sanitize(extra);if(add.isEmpty())return;
            String combined=old.value.isEmpty()?add:old.value+" "+add;if(combined.length()>MAX_CELL_CHARS)combined=combined.substring(0,MAX_CELL_CHARS);
            cells.put(k,new Cell(row,col,combined));
        }
        Sheet freeze(){return new Sheet(index,title,new ArrayList<>(cells.values()));}
    }

    private CadSpreadsheetLayout(){}

    /** Parses OfficeTextExtractor's XLSX text format (A1<TAB>value, grouped by "Sayfa N"). */
    public static List<Sheet> parseExtractedXlsx(String text){
        if(text==null||text.trim().isEmpty())return Collections.emptyList();
        ArrayList<MutableSheet> sheets=new ArrayList<>();MutableSheet current=null;int total=0,lastRow=-1,lastCol=-1;
        String[] lines=text.replace("\r\n","\n").replace('\r','\n').split("\n",-1);
        for(String raw:lines){
            Matcher sh=SHEET.matcher(raw);
            if(sh.matches()){
                if(sheets.size()>=MAX_SHEETS){current=null;continue;}
                int index=parsePositive(sh.group(1),sheets.size()+1);current=new MutableSheet(index,"Sayfa "+index);sheets.add(current);lastRow=lastCol=-1;continue;
            }
            Matcher m=CELL.matcher(raw);
            if(m.matches()){
                if(current==null){if(sheets.size()>=MAX_SHEETS)continue;current=new MutableSheet(1,"Sayfa 1");sheets.add(current);}
                int col=columnIndex(m.group(1)),row=parsePositive(m.group(2),-1);
                if(row<1||row>MAX_ROWS||col<1||col>MAX_COLS||total>=MAX_CELLS)continue;
                long before=current.cells.size();current.put(row,col,m.group(3));if(current.cells.size()>before)total++;lastRow=row;lastCol=col;continue;
            }
            String continuation=raw.trim();
            if(current!=null&&lastRow>0&&lastCol>0&&!continuation.isEmpty())current.append(lastRow,lastCol,continuation);
        }
        ArrayList<Sheet> out=new ArrayList<>();for(MutableSheet s:sheets){Sheet frozen=s.freeze();if(!frozen.isEmpty())out.add(frozen);}
        return Collections.unmodifiableList(out);
    }

    /** Parses CSV/TSV/semicolon text into one bounded sheet, honoring quoted separators and quoted newlines. */
    public static List<Sheet> parseCsv(String text){
        if(text==null||text.trim().isEmpty())return Collections.emptyList();char delimiter=detectDelimiter(text);
        MutableSheet sheet=new MutableSheet(1,"CSV");int row=1,col=1,cells=0;StringBuilder field=new StringBuilder();boolean quote=false;
        for(int i=0;i<=text.length()&&row<=MAX_ROWS&&cells<MAX_CELLS;i++){
            char ch=i<text.length()?text.charAt(i):'\n';
            if(ch=='"'){
                if(quote&&i+1<text.length()&&text.charAt(i+1)=='"'){if(field.length()<MAX_CELL_CHARS)field.append('"');i++;}
                else quote=!quote;
                continue;
            }
            if(!quote&&ch==delimiter){
                sheet.put(row,col,field.toString());cells++;field.setLength(0);col++;if(col>MAX_COLS){while(i+1<text.length()&&text.charAt(i+1)!='\n'&&text.charAt(i+1)!='\r')i++;}
                continue;
            }
            if(!quote&&(ch=='\n'||ch=='\r')){
                if(ch=='\r'&&i+1<text.length()&&text.charAt(i+1)=='\n')i++;
                if(col<=MAX_COLS){sheet.put(row,col,field.toString());cells++;}field.setLength(0);row++;col=1;continue;
            }
            if(field.length()<MAX_CELL_CHARS)field.append(ch);
        }
        Sheet out=sheet.freeze();return out.isEmpty()?Collections.emptyList():Collections.singletonList(out);
    }

    public static float[] columnWidths(Sheet sheet,float minWidth,float maxWidth,float charWidth){
        if(sheet==null||sheet.columns<1)return new float[0];float min=Math.max(1f,minWidth),max=Math.max(min,maxWidth),cw=Math.max(.1f,charWidth);
        float[] widths=new float[sheet.columns];Arrays.fill(widths,min);
        for(Cell cell:sheet.cells){if(cell.col<1||cell.col>widths.length)continue;int longest=0;for(String part:cell.value.split("\\s+",-1))longest=Math.max(longest,part.length());float wanted=Math.min(max,Math.max(min,(Math.min(60,longest)+2)*cw));widths[cell.col-1]=Math.max(widths[cell.col-1],wanted);}
        return widths;
    }

    static int columnIndex(String letters){
        if(letters==null||letters.isEmpty())return-1;int value=0;for(int i=0;i<letters.length();i++){char c=Character.toUpperCase(letters.charAt(i));if(c<'A'||c>'Z')return-1;value=value*26+(c-'A'+1);if(value>MAX_COLS)return value;}return value;
    }
    private static char detectDelimiter(String text){
        int comma=0,semicolon=0,tab=0;boolean quote=false;int limit=Math.min(text.length(),12000);
        for(int i=0;i<limit;i++){char c=text.charAt(i);if(c=='"'){if(quote&&i+1<limit&&text.charAt(i+1)=='"'){i++;continue;}quote=!quote;continue;}if(quote)continue;if(c=='\n'||c=='\r')continue;if(c==',')comma++;else if(c==';')semicolon++;else if(c=='\t')tab++;}
        if(tab>=semicolon&&tab>=comma&&tab>0)return'\t';if(semicolon>=comma&&semicolon>0)return';';return',';
    }
    private static String sanitize(String value){if(value==null)return"";String v=value.replace('\r',' ').replace('\n',' ').replaceAll("[ \\t]+"," ").trim();return v.length()>MAX_CELL_CHARS?v.substring(0,MAX_CELL_CHARS):v;}
    private static long key(int row,int col){return(((long)row)<<32)|(col&0xffffffffL);}
    private static int parsePositive(String value,int fallback){try{int n=Integer.parseInt(value);return n>0?n:fallback;}catch(Exception e){return fallback;}}
}
