import javax.swing.*;
import java.awt.*;
import java.awt.datatransfer.StringSelection;
import java.awt.event.ActionEvent;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.KeyFactory;
import java.security.PrivateKey;
import java.security.Signature;
import java.security.spec.PKCS8EncodedKeySpec;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Base64;
import com.musa.cad.LicenseToken;

/** Small offline desktop UI for issuing MusaCAD licenses. */
public final class LicenseGeneratorApp extends JFrame {
    private final JTextField keyPath=new JTextField();
    private final JTextField installationId=new JTextField();
    private final JComboBox<String> term=new JComboBox<>(new String[]{"30 gün","90 gün","365 gün","Süresiz"});
    private final JTextArea output=new JTextArea(7,42);

    public LicenseGeneratorApp(){
        super("MusaCAD Lisans Üretici");
        setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        setLayout(new BorderLayout(10,10));
        JPanel form=new JPanel(new GridLayout(0,1,6,6));
        form.setBorder(BorderFactory.createEmptyBorder(14,14,0,14));
        form.add(new JLabel("Özel anahtar (.pem)"));
        JPanel keyRow=new JPanel(new BorderLayout(6,0));keyRow.add(keyPath,BorderLayout.CENTER);
        JButton browse=new JButton("Seç");browse.addActionListener(this::browse);keyRow.add(browse,BorderLayout.EAST);form.add(keyRow);
        form.add(new JLabel("Cihaz / Lisans Kimliği"));form.add(installationId);
        form.add(new JLabel("Lisans süresi"));form.add(term);
        JButton issue=new JButton("LİSANS KODU ÜRET");issue.addActionListener(this::issue);form.add(issue);
        add(form,BorderLayout.NORTH);

        output.setLineWrap(true);output.setWrapStyleWord(true);output.setEditable(false);output.setFont(new Font(Font.MONOSPACED,Font.PLAIN,12));
        JScrollPane scroll=new JScrollPane(output);scroll.setBorder(BorderFactory.createTitledBorder("Lisans kodu"));
        JPanel center=new JPanel(new BorderLayout());center.setBorder(BorderFactory.createEmptyBorder(0,14,0,14));center.add(scroll,BorderLayout.CENTER);add(center,BorderLayout.CENTER);
        JPanel actions=new JPanel(new FlowLayout(FlowLayout.RIGHT));
        JButton copy=new JButton("Kopyala");copy.addActionListener(e->copy());actions.add(copy);
        JButton save=new JButton("Dosyaya Kaydet");save.addActionListener(e->save());actions.add(save);add(actions,BorderLayout.SOUTH);
        pack();setLocationRelativeTo(null);
    }

    private void browse(ActionEvent e){
        JFileChooser chooser=new JFileChooser();if(chooser.showOpenDialog(this)==JFileChooser.APPROVE_OPTION)keyPath.setText(chooser.getSelectedFile().getAbsolutePath());
    }
    private void issue(ActionEvent e){
        try{
            String id=installationId.getText().trim();if(id.isEmpty())throw new IllegalArgumentException("Cihaz kimliği boş olamaz");
            Path key=Path.of(keyPath.getText().trim());
            long expiry=0L;String selected=(String)term.getSelectedItem();
            if(selected.startsWith("30"))expiry=Instant.now().plus(30,ChronoUnit.DAYS).toEpochMilli();
            else if(selected.startsWith("90"))expiry=Instant.now().plus(90,ChronoUnit.DAYS).toEpochMilli();
            else if(selected.startsWith("365"))expiry=Instant.now().plus(365,ChronoUnit.DAYS).toEpochMilli();
            String payload=LicenseToken.payload(id,expiry);
            Signature signer=Signature.getInstance("SHA256withRSA");signer.initSign(readPrivateKey(key));signer.update(payload.getBytes(StandardCharsets.UTF_8));
            output.setText(LicenseToken.encode(payload,signer.sign()));
        }catch(Exception ex){JOptionPane.showMessageDialog(this,ex.getMessage(),"Lisans üretilemedi",JOptionPane.ERROR_MESSAGE);}
    }
    private static PrivateKey readPrivateKey(Path path)throws Exception{
        String pem=Files.readString(path,StandardCharsets.US_ASCII);
        String b64=pem.replace("-----BEGIN PRIVATE KEY-----","").replace("-----END PRIVATE KEY-----","").replaceAll("\\s","");
        return KeyFactory.getInstance("RSA").generatePrivate(new PKCS8EncodedKeySpec(Base64.getDecoder().decode(b64)));
    }
    private void copy(){String v=output.getText().trim();if(v.isEmpty())return;Toolkit.getDefaultToolkit().getSystemClipboard().setContents(new StringSelection(v),null);}
    private void save(){
        String v=output.getText().trim();if(v.isEmpty())return;JFileChooser chooser=new JFileChooser("MusaCAD-lisans.txt");
        if(chooser.showSaveDialog(this)==JFileChooser.APPROVE_OPTION)try{Files.writeString(chooser.getSelectedFile().toPath(),v+System.lineSeparator(),StandardCharsets.UTF_8);}catch(Exception ex){JOptionPane.showMessageDialog(this,ex.getMessage());}
    }
    public static void main(String[] args){SwingUtilities.invokeLater(()->new LicenseGeneratorApp().setVisible(true));}
}
