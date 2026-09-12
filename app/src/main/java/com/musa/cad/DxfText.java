package com.musa.cad;

/** Plain text fallback: formatting is intentionally not reproduced. */
public final class DxfText {
    public static String plain(String raw) {
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
        return out.toString().replace("%%d","°").replace("%%p","±").replace("%%c","Ø");
    }
    private DxfText(){}
}
