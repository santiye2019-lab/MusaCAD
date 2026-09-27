import com.musa.cad.CadSpreadsheetLayout;
import java.util.*;

public final class CadSpreadsheetLayoutTest {
    private static void require(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static void eq(String expected,String actual,String message){if(!Objects.equals(expected,actual))throw new AssertionError(message+": "+actual);}

    public static void main(String[]args){
        String xlsx="— Sayfa 1 —\nA1\tEkipman\nB1\tGüç\nA2\tPompa\nB2\t5.5 kW\n\n— Sayfa 2 —\nA1\tNot\nA2\tYedek";
        List<CadSpreadsheetLayout.Sheet> sheets=CadSpreadsheetLayout.parseExtractedXlsx(xlsx);
        require(sheets.size()==2,"two sheets");
        CadSpreadsheetLayout.Sheet first=sheets.get(0);
        require(first.rows==2&&first.columns==2,"2x2 table");
        eq("Pompa",first.valueAt(2,1),"A2");
        eq("5.5 kW",first.valueAt(2,2),"B2");
        float[] widths=CadSpreadsheetLayout.columnWidths(first,60f,220f,9f);
        require(widths.length==2&&widths[0]>=60f&&widths[1]>=60f,"column widths");

        List<CadSpreadsheetLayout.Sheet> csv=CadSpreadsheetLayout.parseCsv("Ad;Değer\nPompa;\"5,5 kW\"\nFan;3,2 kW");
        require(csv.size()==1,"CSV sheet");
        CadSpreadsheetLayout.Sheet table=csv.get(0);
        eq("Ad",table.valueAt(1,1),"CSV header");
        eq("5,5 kW",table.valueAt(2,2),"quoted decimal comma");
        eq("Fan",table.valueAt(3,1),"CSV row 3");

        List<CadSpreadsheetLayout.Sheet> quoted=CadSpreadsheetLayout.parseCsv("A,B\n1,\"iki\nsatır\"");
        require(quoted.size()==1,"quoted newline sheet");
        eq("iki satır",quoted.get(0).valueAt(2,2),"quoted newline normalized");

        List<CadSpreadsheetLayout.Sheet> trailing=CadSpreadsheetLayout.parseCsv("A;B\n1;2\n");
        require(trailing.size()==1,"trailing newline sheet");
        require(trailing.get(0).rows==2,"trailing newline must not add an empty third row");
        System.out.println("CadSpreadsheetLayoutTest OK");
    }
}
