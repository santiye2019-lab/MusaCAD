/* SPDX-License-Identifier: GPL-3.0-or-later */
/* R2018 original ELLIPSE.dwg, no third-party DWG decoder linked.
 * Numeric oracle from source project's AutoCAD Ellipse.txt reference:
 * H=1BE center=(11.1433,9.5374,0), major=(-8.7508,-8.03465,0),
 * ratio=.336109, start=0, end=2pi.
 * Partial v6 preview is NOT final BYLAYER or whole DWG visibility. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int close_to(double a,double b,double eps){
    return isfinite(a)&&fabs(a-b)<eps;
}
static int check_packet(const float *v,size_t n){
    if(n<16u||v[0]!=6.0f||v[1]<1.0f||v[6]!=1.0f)return 0;
    int found=0;
    size_t p=7u;size_t seen=0u;
    while(p<n){
        const float word=v[p++];
        const unsigned abs_code=(unsigned)fabs((double)word);
        const unsigned type=abs_code>>8u;
        if(abs_code<256u||type<1u||type>4u)return 0;
        if(word<0.0f){if(p==n)return 0;p++;}
        if(type==4u){
            if(n-p<8u)return 0;
            const double cx=v[p],cy=v[p+1u];
            const double ax=v[p+2u],ay=v[p+3u];
            const double bx=v[p+4u],by=v[p+5u];
            const double start=v[p+6u],sweep=v[p+7u];
            if(close_to(cx,11.1433,.003)&&close_to(cy,9.5374,.003)){
                if(!close_to(ax,-8.7508,.003)||
                   !close_to(ay,-8.03465,.003)||
                   !close_to(bx,2.70052,.004)||
                   !close_to(by,-2.94122,.004)||
                   !close_to(start,0.0,.001)||
                   !close_to(sweep,6.283185307,.003))
                    return 0;
                for(unsigned k=0u;k<721u;k++){
                    const double t=start+sweep*(double)k/720.0;
                    const double x=cx+ax*cos(t)+bx*sin(t);
                    const double y=cy+ay*cos(t)+by*sin(t);
                    if(x<(double)v[2]-0.002||x>(double)v[4]+0.002||
                       y<(double)v[3]-0.002||y>(double)v[5]+0.002)
                        return 0;
                }
                found++;
            }
            p+=8u;
        }else if(type==1u){if(n-p<4u)return 0;p+=4u;}
        else if(type==3u){if(n-p<2u)return 0;p+=2u;}
        else{
            if(p==n)return 0;
            const float f=v[p++];
            const int verts=(int)fabs((double)f);
            if(verts<2||verts>100000||(size_t)verts>(n-p)/2u)return 0;
            p+=2u*(size_t)verts;
        }
        seen++;
    }
    return found==1&&p==n&&seen==(size_t)v[1];
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* no fixture needed for host CTest smoke */
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");
    if(!f)return 1;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 1;}
    const long len=ftell(f);
    if(len<256||len>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 1;}
    uint8_t *file=(uint8_t*)malloc((size_t)len);
    uint8_t *copy=(uint8_t*)malloc((size_t)len);
    if(!file||!copy){fclose(f);free(file);free(copy);return 1;}
    int ok=fread(file,1u,(size_t)len,f)==(size_t)len;
    if(fclose(f)!=0)ok=0;
    memcpy(copy,file,(size_t)len);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
          file,(size_t)len,&scene)&&!memcmp(file,copy,(size_t)len);
    free(file);free(copy);
    MusaDwgAc1032NativeEllipse e={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_ellipse(
       scene,UINT64_C(0x1be),&e)&&
       e.geometry_decoded&&e.indexed_handle==UINT64_C(0x1be)&&
       e.common.common_decoded&&
       close_to(e.center[0],11.1433,.001)&&
       close_to(e.center[1],9.5374,.001)&&e.center[2]==0.0&&
       close_to(e.semi_major[0],-8.7508,.001)&&
       close_to(e.semi_major[1],-8.03465,.001)&&
       e.semi_major[2]==0.0&&
       e.extrusion[0]==0.0&&e.extrusion[1]==0.0&&
       e.extrusion[2]==1.0&&
       close_to(e.ratio,.336109,.00002)&&
       close_to(e.start_angle,0.0,.001)&&
       close_to(e.end_angle,6.283185307,.002);
    MusaAc1032PreviewV6Stats required={0},copied={0};
    MusaAc1032PreviewV6SourceCensus census={0};
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_source_aci(
          scene,NULL,0u,&required)&&
       musa_dwg_ac1032_preview_v6_source_census(scene,&census)&&
       census.packet_admitted_source_records>0u&&
       census.emitted_v6_primitives==required.rendered_primitives&&
       census.complete_dwg_display_accepted==0u&&
       required.required_float_words>7u;
    float *v=NULL;
    if(ok){v=(float*)calloc(required.required_float_words,sizeof(float));if(!v)ok=0;}
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_source_aci(
         scene,v,required.required_float_words,&copied)&&
       !memcmp(&required,&copied,sizeof(required))&&
       check_packet(v,required.required_float_words);
    if(ok){
        MusaDwgAc1032NativeEllipse sentinel=e;
        if(musa_dwg_ac1032_scene_index_read_ellipse(scene,UINT64_C(1),&e)||
           memcmp(&e,&sentinel,sizeof(e))!=0)ok=0;
    }
    fprintf(stderr,"%s original R2018 ellipse H1BE WCS major/minor "
          "angle + native v6 partial drawing; no layer visibility claim\n",
          ok?"PASS":"FAIL");
    free(v);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok?0:1;
}
