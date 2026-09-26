#ifndef MUSACAD_NATIVE_COLOR_H
#define MUSACAD_NATIVE_COLOR_H

typedef enum {
    MUSA_COLOR_INHERIT_LAYER=0,
    MUSA_COLOR_INHERIT_BLOCK=1,
    MUSA_COLOR_ACI=2,
    MUSA_COLOR_RGB=3
} MusaNativeColorKind;

typedef struct {
    MusaNativeColorKind kind;
    int aci;
    unsigned int rgb;
} MusaNativeColorToken;

/*
 * Normalize LibreDWG CMC colors before the fast native first paint.
 *
 * LibreDWG DWG_COLOR_METHOD values are authoritative:
 *   c0 ByLayer, c1 ByBlock, c2 ACI, c3 TrueColor.
 *
 * Keep ACI and TrueColor separate. A TrueColor can still carry a palette-like
 * index, and treating that index as the display color makes the native first
 * frame disagree with the complete DXF renderer.
 */
static inline MusaNativeColorToken musa_native_color_token(unsigned int method,int index,unsigned int rgb){
    MusaNativeColorToken out;
    out.kind=MUSA_COLOR_INHERIT_LAYER;out.aci=7;out.rgb=rgb&0x00ffffffu;

    unsigned int m=method&0xffu;
    if(!m&&(rgb&0xff000000u))m=(rgb>>24)&0xffu;
    int aci=index<0?-index:index;

    if(m==0xc0u){
        out.kind=MUSA_COLOR_INHERIT_LAYER;return out;
    }
    if(m==0xc1u){
        out.kind=MUSA_COLOR_INHERIT_BLOCK;return out;
    }
    if(m==0xc2u){
        if(index==0){
            out.kind=MUSA_COLOR_INHERIT_BLOCK;return out;
        }
        if(aci==256){
            out.kind=MUSA_COLOR_INHERIT_LAYER;return out;
        }
        if(aci>=1&&aci<=255){
            out.kind=MUSA_COLOR_ACI;out.aci=aci;return out;
        }
        return out;
    }
    if(m==0xc3u){
        out.kind=MUSA_COLOR_RGB;return out;
    }

    /* Legacy/pre-R2004 index semantics. */
    if(index==0){
        out.kind=MUSA_COLOR_INHERIT_BLOCK;return out;
    }
    if(aci==256){
        out.kind=MUSA_COLOR_INHERIT_LAYER;return out;
    }
    if(aci>=1&&aci<=255){
        out.kind=MUSA_COLOR_ACI;out.aci=aci;return out;
    }

    return out;
}

#endif
