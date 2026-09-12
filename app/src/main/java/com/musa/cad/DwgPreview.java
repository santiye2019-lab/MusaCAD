package com.musa.cad;

import android.graphics.*;
import java.io.*;

final class DwgPreview {
    private DwgPreview(){}
    static Bitmap read(File file) throws IOException {
        byte[] data=new byte[(int)Math.min(file.length(),16*1024*1024)];
        try(FileInputStream in=new FileInputStream(file)){int off=0,n;while(off<data.length&&(n=in.read(data,off,data.length-off))>0)off+=n;}
        byte[] sig={(byte)0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
        for(int i=0;i<data.length-sig.length;i++){
            boolean ok=true;for(int j=0;j<sig.length;j++)if(data[i+j]!=sig[j]){ok=false;break;}
            if(ok){Bitmap b=BitmapFactory.decodeByteArray(data,i,data.length-i);if(b!=null)return b;}
        }
        return null;
    }
}
