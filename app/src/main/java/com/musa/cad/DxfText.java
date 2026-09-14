package com.musa.cad;

/** Converts common DXF TEXT/MTEXT escapes into readable Unicode text. */
public final class DxfText {
    public static String plain(String raw) {
        if(raw==null||raw.isEmpty())return "";
        StringBuilder out=new StringBuilder();
        for(int i=0;i<raw.length();i++) {
            char c=raw.charAt(i);
            if(c=='{'||c=='}')continue;
            if(c!='\\'||i+1==raw.length()){out.append(c);continue;}
            char command=raw.charAt(++i);
            if(command=='P'||command=='X'){out.append('\n');continue;}
            if(command=='~'){out.append(' ');continue;}
            if(command=='\\'||command=='{'||command=='}'){out.append(command);continue;}
            if(command=='U'&&i+5<raw.length()&&raw.charAt(i+1)=='+'){
                try{out.append((char)Integer.parseInt(raw.substring(i+2,i+6),16));i+=5;continue;}catch(NumberFormatException ignored){}
            }
            // Underline/overline/strike toggles affect appearance, not characters.
            if("LlOoKk".indexOf(command)>=0)continue;
            // Formatting commands terminated by ';'. Preserve stacked fractions as readable text.
            if("AaCcFfHhQqTtWwpS".indexOf(command)>=0){
                int end=raw.indexOf(';',i+1);
                if(end>=0){
                    if(command=='S')out.append(stackPlain(raw.substring(i+1,end)));
                    i=end;continue;
                }
            }
            out.append('\\').append(command);
        }
        return percentCodes(out.toString());
    }

    private static String stackPlain(String value){
        return value.replace("#","/").replace("^","/").replace("\\","/");
    }
    private static String percentCodes(String s){
        StringBuilder out=new StringBuilder();
        for(int i=0;i<s.length();i++){
            if(i+2<s.length()&&s.charAt(i)=='%'&&s.charAt(i+1)=='%'){
                char q=Character.toLowerCase(s.charAt(i+2));
                if(q=='d'){out.append('°');i+=2;continue;}
                if(q=='p'){out.append('±');i+=2;continue;}
                if(q=='c'){out.append('Ø');i+=2;continue;}
                if(q=='u'||q=='o'){i+=2;continue;}
            }
            out.append(s.charAt(i));
        }
        return out.toString();
    }
    private DxfText(){}
}
