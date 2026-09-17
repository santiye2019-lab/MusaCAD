import com.musa.cad.PrintLayout;

public class PrintLayoutTest {
    private static void close(double actual,double expected){
        if(Math.abs(actual-expected)>1e-9)throw new AssertionError(actual+" != "+expected);
    }
    private static void check(double[] value,int length){if(value.length!=length)throw new AssertionError("length="+value.length);}
    public static void main(String[] args){
        double[] exact=PrintLayout.fit(100,200,0,0,100,200);check(exact,4);
        close(exact[0],0);close(exact[1],0);close(exact[2],100);close(exact[3],200);

        double[] portrait=PrintLayout.fit(200,100,0,0,100,200);check(portrait,4);
        close(portrait[0],0);close(portrait[1],75);close(portrait[2],100);close(portrait[3],50);

        double[] landscape=PrintLayout.fit(100,200,10,20,310,170);check(landscape,4);
        close(landscape[0],122.5);close(landscape[1],20);close(landscape[2],75);close(landscape[3],150);

        check(PrintLayout.fit(0,100,0,0,100,100),0);
        check(PrintLayout.fit(100,100,10,0,10,100),0);
        System.out.println("Print layout cases passed");
    }
}
