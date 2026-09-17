package com.musa.cad;

import java.util.*;

/** Parses AutoCAD MTEXT inline formatting into style runs without Android dependencies. */
public final class DxfMText {
    public static final class Run {
        public final String text,font;
        public final double heightScale,widthScale,absoluteHeight,obliqueDegrees,tracking;
        public final int aci,trueColor;
        public final boolean underline,overline,strike;

        Run(String text,State s){
            this.text=text;
            this.font=s.font;
            this.heightScale=s.heightScale;
            this.widthScale=s.widthScale;
            this.absoluteHeight=s.absoluteHeight;
            this.obliqueDegrees=s.obliqueDegrees;
            this.tracking=s.tracking;
            this.aci=s.aci;
            this.trueColor=s.trueColor;
            this.underline=s.underline;
            this.overline=s.overline;
            this.strike=s.strike;
        }

        public boolean hasAbsoluteHeight(){return Double.isFinite(absoluteHeight)&&absoluteHeight>0d;}
    }

    public static final class Result {
        public final List<Run> runs;
        Result(List<Run> runs){this.runs=Collections.unmodifiableList(runs);}
        public String plainText(){
            StringBuilder b=new StringBuilder();
            for(Run r:runs)b.append(r.text);
            return b.toString();
        }
    }

    private static final class State {
        String font="";
        double heightScale=1d,widthScale=1d,absoluteHeight=Double.NaN,obliqueDegrees=0d,tracking=1d;
        int aci=-1,trueColor=-1;
        boolean underline,overline,strike;
        State copy(){
            State s=new State();
            s.font=font;s.heightScale=heightScale;s.widthScale=widthScale;s.absoluteHeight=absoluteHeight;
            s.obliqueDegrees=obliqueDegrees;s.tracking=tracking;s.aci=aci;s.trueColor=trueColor;
            s.underline=underline;s.overline=overline;s.strike=strike;
            return s;
        }
    }

    public static Result parse(String raw){
        ArrayList<Run> out=new ArrayList<>();
        if(raw==null||raw.isEmpty())return new Result(out);
        State state=new State();
        ArrayDeque<State> stack=new ArrayDeque<>();
        StringBuilder text=new StringBuilder();

        for(int i=0;i<raw.length();i++){
            char c=raw.charAt(i);

            if(c=='{'){
                flush(out,text,state);
                stack.push(state.copy());
                continue;
            }
            if(c=='}'){
                flush(out,text,state);
                if(!stack.isEmpty())state=stack.pop();
                continue;
            }

            if(c=='%'&&i+2<raw.length()&&raw.charAt(i+1)=='%'){
                char code=raw.charAt(i+2),lower=Character.toLowerCase(code);
                if(lower=='d'){text.append('°');i+=2;continue;}
                if(lower=='p'){text.append('±');i+=2;continue;}
                if(lower=='c'){text.append('Ø');i+=2;continue;}
                if(code=='%'){text.append('%');i+=2;continue;}
                if(lower=='u'||lower=='o'){
                    flush(out,text,state);
                    if(lower=='u')state.underline=!state.underline; else state.overline=!state.overline;
                    i+=2;continue;
                }
                if(Character.isDigit(code)){
                    int end=i+2;
                    while(end<raw.length()&&end<i+5&&Character.isDigit(raw.charAt(end)))end++;
                    try{
                        int cp=Integer.parseInt(raw.substring(i+2,end));
                        if(Character.isValidCodePoint(cp)){text.appendCodePoint(cp);i=end-1;continue;}
                    }catch(NumberFormatException ignored){}
                }
            }

            if(c!='\\'){
                text.append(c);
                continue;
            }
            if(i+1>=raw.length()){text.append('\\');continue;}

            char cmd=raw.charAt(++i);
            if(cmd=='P'){text.append('\n');continue;}
            if(cmd=='~'){text.append(' ');continue;}
            if(cmd=='\\'||cmd=='{'||cmd=='}'){text.append(cmd);continue;}
            if(cmd=='U'&&i+5<raw.length()&&raw.charAt(i+1)=='+'){
                try{
                    int cp=Integer.parseInt(raw.substring(i+2,i+6),16);
                    if(Character.isValidCodePoint(cp)){text.appendCodePoint(cp);i+=5;continue;}
                }catch(NumberFormatException ignored){}
            }
            if(cmd=='L'||cmd=='l'||cmd=='O'||cmd=='o'||cmd=='K'||cmd=='k'){
                flush(out,text,state);
                if(cmd=='L')state.underline=true;
                else if(cmd=='l')state.underline=false;
                else if(cmd=='O')state.overline=true;
                else if(cmd=='o')state.overline=false;
                else if(cmd=='K')state.strike=true;
                else state.strike=false;
                continue;
            }

            int semi=raw.indexOf(';',i+1);
            if(semi<0){text.append('\\').append(cmd);continue;}
            String arg=raw.substring(i+1,semi);
            boolean handled=true;
            flush(out,text,state);
            switch(cmd){
                case 'H': case 'h': applyHeight(state,arg);break;
                case 'W': case 'w': state.widthScale=positive(number(stripX(arg),state.widthScale),state.widthScale);break;
                case 'Q': case 'q': state.obliqueDegrees=finite(number(arg,state.obliqueDegrees),state.obliqueDegrees);break;
                case 'T': case 't': state.tracking=positive(number(arg,state.tracking),state.tracking);break;
                case 'C':
                    state.aci=integer(arg,state.aci);state.trueColor=-1;break;
                case 'c':
                    state.trueColor=integer(arg,state.trueColor);state.aci=-1;break;
                case 'F': case 'f':
                    int bar=arg.indexOf('|');state.font=(bar>=0?arg.substring(0,bar):arg).trim();break;
                case 'S':
                    text.append(stackText(arg));break;
                case 'A': case 'a':
                    // MTEXT paragraph/vertical alignment control: entity attachment handles placement.
                    break;
                case 'p':
                    // Paragraph properties are kept at the entity level for now.
                    break;
                default:
                    handled=false;
                    break;
            }
            if(handled){i=semi;continue;}
            text.append('\\').append(cmd);
        }
        flush(out,text,state);
        return new Result(merge(out));
    }

    private static void applyHeight(State s,String raw){
        String value=raw==null?"":raw.trim();
        boolean relative=value.endsWith("x")||value.endsWith("X");
        double v=number(stripX(value),Double.NaN);
        if(!Double.isFinite(v)||v<=0d)return;
        if(relative){s.heightScale=v;s.absoluteHeight=Double.NaN;}
        else{s.absoluteHeight=v;s.heightScale=1d;}
    }

    private static String stripX(String s){
        if(s==null)return "";
        String v=s.trim();
        return v.endsWith("x")||v.endsWith("X")?v.substring(0,v.length()-1).trim():v;
    }

    private static String stackText(String value){
        if(value==null)return "";
        int split=-1;
        for(int i=0;i<value.length();i++)if(value.charAt(i)=='#'||value.charAt(i)=='^'||value.charAt(i)=='/'){split=i;break;}
        if(split<0)return value;
        return value.substring(0,split)+"/"+value.substring(split+1);
    }

    private static void flush(List<Run> out,StringBuilder text,State state){
        if(text.length()==0)return;
        out.add(new Run(text.toString(),state.copy()));
        text.setLength(0);
    }

    private static List<Run> merge(List<Run> source){
        ArrayList<Run> out=new ArrayList<>();
        for(Run r:source){
            if(r.text.isEmpty())continue;
            if(!out.isEmpty()&&same(out.get(out.size()-1),r)){
                Run a=out.remove(out.size()-1);
                State s=stateOf(a);
                out.add(new Run(a.text+r.text,s));
            }else out.add(r);
        }
        return out;
    }

    private static State stateOf(Run r){
        State s=new State();s.font=r.font;s.heightScale=r.heightScale;s.widthScale=r.widthScale;s.absoluteHeight=r.absoluteHeight;
        s.obliqueDegrees=r.obliqueDegrees;s.tracking=r.tracking;s.aci=r.aci;s.trueColor=r.trueColor;
        s.underline=r.underline;s.overline=r.overline;s.strike=r.strike;return s;
    }

    private static boolean same(Run a,Run b){
        return Objects.equals(a.font,b.font)&&eq(a.heightScale,b.heightScale)&&eq(a.widthScale,b.widthScale)
            &&eqNaN(a.absoluteHeight,b.absoluteHeight)&&eq(a.obliqueDegrees,b.obliqueDegrees)&&eq(a.tracking,b.tracking)
            &&a.aci==b.aci&&a.trueColor==b.trueColor&&a.underline==b.underline&&a.overline==b.overline&&a.strike==b.strike;
    }
    private static boolean eq(double a,double b){return Math.abs(a-b)<1e-9;}
    private static boolean eqNaN(double a,double b){return (Double.isNaN(a)&&Double.isNaN(b))||eq(a,b);}
    private static double number(String s,double fallback){try{double v=Double.parseDouble(s.trim());return Double.isFinite(v)?v:fallback;}catch(Exception e){return fallback;}}
    private static int integer(String s,int fallback){try{return Integer.parseInt(s.trim());}catch(Exception e){return fallback;}}
    private static double positive(double v,double fallback){return Double.isFinite(v)&&v>0d?v:fallback;}
    private static double finite(double v,double fallback){return Double.isFinite(v)?v:fallback;}

    private DxfMText(){}
}
