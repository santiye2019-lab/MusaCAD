import com.musa.cad.DxfColor;
import java.util.*;

public class DxfColorTest {
    private static void eq(int actual,int expected,String name){if(actual!=expected)throw new AssertionError(name+": 0x"+Integer.toHexString(actual));}
    public static void main(String[] args){
        eq(DxfColor.aciArgb(1),0xffff0000,"ACI red");
        eq(DxfColor.aciArgb(2),0xffffff00,"ACI yellow");
        eq(DxfColor.aciArgb(3),0xff00ff00,"ACI green");
        eq(DxfColor.aciArgb(4),0xff00ffff,"ACI cyan");
        eq(DxfColor.aciArgb(5),0xff0000ff,"ACI blue");
        eq(DxfColor.aciArgb(6),0xffff00ff,"ACI magenta");
        eq(DxfColor.aciArgb(11),0xffffaaaa,"ACI pastel red");
        eq(DxfColor.trueColor(3261100864L),0x00607340,"TrueColor method bits");
        DxfColor.Ref trueColor=DxfColor.resolve(256,0x123456,"X",null);
        eq(DxfColor.argb(trueColor,Collections.emptyMap()),0xff123456,"TrueColor");
        Map<String,Integer> layers=new HashMap<>();layers.put("BORU",0xff00ff00);
        eq(DxfColor.argb(DxfColor.resolve(256,-1,"boru",null),layers),0xff00ff00,"BYLAYER");
        DxfColor.Ref parent=DxfColor.resolve(1,-1,"0",null);
        eq(DxfColor.argb(DxfColor.resolve(0,-1,"0",parent),layers),0xffff0000,"BYBLOCK");
        System.out.println("11 DXF color cases passed");
    }
}
