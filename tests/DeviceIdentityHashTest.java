import com.musa.cad.DeviceIdentityHash;

public final class DeviceIdentityHashTest {
    public static void main(String[] args){
        String a=DeviceIdentityHash.derive("ABCDEF1234567890","com.musa.cad");
        String b=DeviceIdentityHash.derive("abcdef1234567890","com.musa.cad");
        require(!a.isEmpty()&&a.equals(b),"stable/case-normalized");
        require(DeviceIdentityHash.isValidPublicId(a),"public format");
        require(!a.contains("ABCDEF1234567890"),"raw id not exposed");
        require(!a.equals(DeviceIdentityHash.derive("ABCDEF1234567890","com.other.app")),"package domain separation");
        require(DeviceIdentityHash.derive("", "com.musa.cad").isEmpty(),"empty Android ID rejected");
        require(DeviceIdentityHash.derive("ABCDEF1234567890", "").isEmpty(),"empty package rejected");
        require(DeviceIdentityHash.derive("9774d56d682e549c","com.musa.cad").isEmpty(),"legacy bad id rejected");
        require(!DeviceIdentityHash.isValidPublicId("MC-FALLBACK-12345678-1234-1234-1234-123456789012"),"reinstall-resettable fallback rejected");
        require(!DeviceIdentityHash.isValidPublicId(""),"empty public id rejected");
        System.out.println("DeviceIdentityHashTest OK");
    }
    private static void require(boolean ok,String name){if(!ok)throw new AssertionError(name);}
}
