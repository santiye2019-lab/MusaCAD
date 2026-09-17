package com.musa.cad;

/** Unit and physical-scale helpers shared by DXF printing and regression tests. */
public final class CadPrintMath {
    /** Millimeters represented by one DXF drawing unit, or NaN when units are unknown. */
    public static double millimetersPerUnit(int insUnits){
        switch(insUnits){
            case 1:return 25.4;             // inches
            case 2:return 304.8;            // feet
            case 3:return 1609344.0;        // miles
            case 4:return 1.0;              // millimeters
            case 5:return 10.0;             // centimeters
            case 6:return 1000.0;           // meters
            case 7:return 1000000.0;        // kilometers
            case 8:return 0.0000254;        // microinches
            case 9:return 0.0254;           // mils
            case 10:return 914.4;           // yards
            case 11:return 1e-7;            // angstroms
            case 12:return 1e-6;            // nanometers
            case 13:return 0.001;            // microns
            case 14:return 100.0;            // decimeters
            case 15:return 10000.0;          // decameters
            case 16:return 100000.0;         // hectometers
            case 17:return 1e12;             // gigameters
            case 18:return 149597870700000.0;// astronomical units
            case 19:return 9.4607304725808e18;// light years
            case 20:return 3.0856775814913673e19;// parsecs
            case 21:return 304.8006096012192; // US survey feet
            case 22:return 25.4000508001016;  // US survey inches
            case 23:return 914.4018288036576; // US survey yards
            case 24:return 1609347.2186944373;// US survey miles
            default:return Double.NaN;
        }
    }

    public static String unitName(int insUnits){
        switch(insUnits){
            case 1:return "in";case 2:return "ft";case 3:return "mi";case 4:return "mm";
            case 5:return "cm";case 6:return "m";case 7:return "km";case 8:return "µin";
            case 9:return "mil";case 10:return "yd";case 11:return "Å";case 12:return "nm";
            case 13:return "µm";case 14:return "dm";case 15:return "dam";case 16:return "hm";
            case 17:return "Gm";case 18:return "AU";case 19:return "ly";case 20:return "pc";
            case 21:return "US-ft";case 22:return "US-in";case 23:return "US-yd";case 24:return "US-mi";
            default:return "birimsiz";
        }
    }

    /** PostScript points per drawing unit for an engineering scale 1:denominator. */
    public static double pointsPerDrawingUnit(double millimetersPerUnit,int denominator){
        if(!Double.isFinite(millimetersPerUnit)||millimetersPerUnit<=0||denominator<=0)return Double.NaN;
        return millimetersPerUnit*72.0/(25.4*denominator);
    }

    private CadPrintMath(){}
}
