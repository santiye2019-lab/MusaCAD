package com.musa.cad;

/** Plain-text fallback for AutoCAD TEXT/MTEXT control sequences. */
public final class DxfText {
    public static String plain(String raw) {
        if(raw==null||raw.isEmpty())return "";
        StringBuilder out=new StringBuilder();
        for(int i=0;i<raw.length();i++) {
            char c=raw.charAt(i);
            if(c=='{'||c=='}')continue;
            if(c!='\\'||i+1==raw.length()){out.append(c);continue;}
            char command=raw.charAt(++i);
            if(command=='P'){out.append('\n');continue;}
            if(command=='~'){out.append(' ');continue;}
            if(command=='\\'||command=='{'||command=='}'){out.append(command);continue;}
            if(command=='U'&&i+5<raw.length()&&raw.charAt(i+1)=='+'){
                try{out.append((char)Integer.parseInt(raw.substring(i+2,i+6),16));i+=5;continue;}catch(NumberFormatException ignored){}
            }
            if("LlOoKk".indexOf(command)>=0)continue;
            if("ACcFfHhQqTtWwpS".indexOf(command)>=0){
                int end=raw.indexOf(';',i+1);
                if(end>=0){if(command=='S')out.append(raw.substring(i+1,end).replace('#','/').replace('^','/'));i=end;continue;}
            }
            out.append('\\').append(command);
        }
        return decodePercentCodes(out.toString());
    }

    private static String decodePercentCodes(String value){
        StringBuilder out=new StringBuilder();
        for(int i=0;i<value.length();i++){
            if(value.charAt(i)!='%'||i+2>=value.length()||value.charAt(i+1)!='%'){out.append(value.charAt(i));continue;}
            char code=value.charAt(i+2),lower=Character.toLowerCase(code);
            if(lower=='d'){out.append('°');i+=2;continue;}
            if(lower=='p'){out.append('±');i+=2;continue;}
            if(lower=='c'){out.append('Ø');i+=2;continue;}
            if(code=='%'){out.append('%');i+=2;continue;}
            if(Character.isDigit(code)){
                int end=i+2;while(end<value.length()&&end<i+5&&Character.isDigit(value.charAt(end)))end++;
                try{int cp=Integer.parseInt(value.substring(i+2,end));if(cp>=0&&cp<=Character.MAX_VALUE){out.append((char)cp);i=end-1;continue;}}catch(NumberFormatException ignored){}
            }
            // Formatting toggles such as %%o/%%u affect appearance only; keep following text.
            if(lower=='o'||lower=='u'){i+=2;continue;}
            out.append('%').append('%');i++;
        }
        return out.toString();
    }
    private DxfText(){}
}
