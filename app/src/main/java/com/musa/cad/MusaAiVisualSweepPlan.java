package com.musa.cad;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Fixed coverage plan for the active DWG content rectangle.
 * The first image in every network batch is a small whole-sheet overview;
 * all nine high-resolution regions are then rendered exactly once, with overlap.
 * This class has no Android dependencies and can be regression-tested with javac.
 */
public final class MusaAiVisualSweepPlan {
    public static final int GRID=3;
    public static final int TILE_COUNT=GRID*GRID;
    public static final int TILES_PER_BATCH=4;
    public static final int BATCH_COUNT=(TILE_COUNT+TILES_PER_BATCH-1)/TILES_PER_BATCH;
    private static final double OVERLAP=0.015d;

    public static final class Tile {
        public final int index,row,column;
        public final double left,top,right,bottom;
        Tile(int index,int row,int column,double left,double top,double right,double bottom){
            this.index=index;this.row=row;this.column=column;
            this.left=left;this.top=top;this.right=right;this.bottom=bottom;
        }
        public String label(){return "sheet-tile-"+(index+1);}
    }

    public static List<Tile> tiles(){
        ArrayList<Tile> tiles=new ArrayList<>(TILE_COUNT);
        for(int row=0;row<GRID;row++)for(int col=0;col<GRID;col++){
            double l=Math.max(0d,(col/(double)GRID)-OVERLAP);
            double t=Math.max(0d,(row/(double)GRID)-OVERLAP);
            double r=Math.min(1d,((col+1)/(double)GRID)+OVERLAP);
            double b=Math.min(1d,((row+1)/(double)GRID)+OVERLAP);
            tiles.add(new Tile(row*GRID+col,row,col,l,t,r,b));
        }
        return Collections.unmodifiableList(tiles);
    }

    public static int firstTile(int batch){
        checkBatch(batch);
        return batch*TILES_PER_BATCH;
    }
    public static int lastExclusive(int batch){
        checkBatch(batch);
        return Math.min(TILE_COUNT,(batch+1)*TILES_PER_BATCH);
    }
    private static void checkBatch(int batch){
        if(batch<0||batch>=BATCH_COUNT)throw new IllegalArgumentException("invalid sweep batch");
    }
    private MusaAiVisualSweepPlan(){}
}
