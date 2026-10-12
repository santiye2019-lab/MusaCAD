package com.musa.cad;

import java.io.IOException;
import java.math.BigDecimal;

/** Emits finite DXF real group values without the former fixed eight-decimal rounding. */
public final class DxfNumber {
    private DxfNumber(){}

    public static String format(double value)throws IOException{
        if(!Double.isFinite(value))throw new IOException("Geçersiz DXF sayısal koordinat");
        // BigDecimal.valueOf uses a round-trip-safe decimal representation of
        // the double; toPlainString avoids scientific notation in CAD readers.
        return BigDecimal.valueOf(value).stripTrailingZeros().toPlainString();
    }
}
