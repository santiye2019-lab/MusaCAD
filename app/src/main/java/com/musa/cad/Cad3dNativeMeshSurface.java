package com.musa.cad;

/**
 * Diagnostic Android bridge to the shared C11 3D triangle surface engine.
 * The existing Cad3dSurfaceMetrics Java result remains authoritative.
 * No UI, automatic metraj, or ACIS B-Rep claims are enabled here.
 */
public final class Cad3dNativeMeshSurface {
    private Cad3dNativeMeshSurface(){}
    private static volatile boolean attempted;
    private static volatile boolean ready;

    private static synchronized boolean ensureLibrary() {
        if(!attempted) {
            attempted=true;
            try {
                System.loadLibrary("musacad_dwg");
                ready=true;
            } catch(LinkageError | SecurityException unavailable) {
                ready=false;
            }
        }
        return ready;
    }

    /** Native [area, centerX, centerY, centerZ, triangleCount, degenerateCount]. */
    private static native double[] measureNative(float[] xyz,int[] triangles);

    public static final class Comparison {
        public final Cad3dSurfaceMetrics.Result javaResult;
        public final double[] nativeResult;
        public final boolean nativeAvailable, agreesWithJava;
        public final String status;
        private Comparison(Cad3dSurfaceMetrics.Result reference,double[] candidate,
                           boolean nativeAvailable,boolean matches,String status) {
            javaResult=reference;
            nativeResult=candidate==null?null:candidate.clone();
            this.nativeAvailable=nativeAvailable;
            agreesWithJava=matches;
            this.status=status;
        }
    }

    private static boolean sameFinite(double a,double b) {
        return Double.isFinite(a)&&Double.isFinite(b)&&
            Math.abs(a-b)<=Math.max(1e-10,
                    1e-12*Math.max(Math.abs(a),Math.abs(b)));
    }
    private static boolean sameCenter(double a,double b) {
        return Double.isNaN(a)&&Double.isNaN(b)||sameFinite(a,b);
    }

    /**
     * Numerically inspect native vs Java; passing means these two engines
     * agree on this specific mesh, NOT that CAD or metraj parity is certified.
     */
    public static boolean agrees(Cad3dSurfaceMetrics.Result original,double[] candidate) {
        if(original==null||candidate==null||candidate.length!=6||
           !sameFinite(original.area,candidate[0]))return false;
        if(candidate[4]!=original.triangleCount ||
           candidate[5]!=original.degenerateCount)return false;
        return sameCenter(original.centroidX,candidate[1]) &&
               sameCenter(original.centroidY,candidate[2]) &&
               sameCenter(original.centroidZ,candidate[3]);
    }

    /**
     * EXPERIMENTAL: call from a CAD file worker, not the drawing UI thread.
     * Always calculate/retain the existing Java output before native opt-in.
     * Unsupported or absent JNI is an explicit status, never fabricated pass.
     */
    public static Comparison compare(float[] xyz,int[] triangles) {
        Cad3dSurfaceMetrics.Result reference=Cad3dSurfaceMetrics.measure(xyz,triangles);
        /* Once production Android uses the same C implementation directly,
         * a native-vs-native self-compare would be a false parity claim. */
        String vm=System.getProperty("java.vm.name","");
        if("Dalvik".equalsIgnoreCase(vm)||"ART".equalsIgnoreCase(vm))
            return new Comparison(reference,null,false,false,
                    "native_authoritative_no_independent_oracle");
        if(xyz.length==0||triangles.length==0)
            return new Comparison(reference,null,false,false,"empty_mesh_java_only");
        if(xyz.length/3>1000000||triangles.length/3>50000)
            return new Comparison(reference,null,false,false,"native_input_limit");
        if(!ensureLibrary())
            return new Comparison(reference,null,false,false,"native_unavailable");
        try {
            double[] candidate=measureNative(xyz,triangles);
            if(candidate==null||candidate.length!=6)
                return new Comparison(reference,null,true,false,"native_rejected");
            boolean equal=agrees(reference,candidate);
            return new Comparison(reference,candidate,true,equal,
                    equal?"cross_check_passed":"native_mismatch");
        } catch(LinkageError unavailable) {
            ready=false;
            return new Comparison(reference,null,false,false,"native_unavailable");
        }
    }
}
