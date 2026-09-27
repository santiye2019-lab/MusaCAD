import com.musa.cad.ShortLicenseCode;
import javax.swing.*;
import java.awt.*;
import java.awt.datatransfer.StringSelection;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/** Offline desktop UI for issuing 12-character MusaCAD licenses from the Serial shown in the app. */
public final class ShortLicenseGeneratorApp extends JFrame {
    private final JTextField serial=new JTextField();
    private final JComboBox<String> term=new JComboBox<>(new String[]{"30 gün","90 gün","365 gün","Süresiz"});
    private final JTextField output=new JTextField();

    public ShortLicenseGeneratorApp(){
        super("MusaCAD 12 Haneli Lisans Üretici");
        setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        setLayout(new BorderLayout(10,10));

        JPanel form=new JPanel(new GridLayout(0,1,6,6));
        form.setBorder(BorderFactory.createEmptyBorder(14,14,0,14));
        form.add(new JLabel("MusaCAD Serial (12 karakter)"));
        form.add(serial);
        form.add(new JLabel("Lisans süresi"));
        form.add(term);
        JButton issue=new JButton("LİSANS KODU ÜRET");
        issue.addActionListener(e->issue());
        form.add(issue);
        add(form,BorderLayout.NORTH);

        output.setEditable(false);
        output.setHorizontalAlignment(JTextField.CENTER);
        output.setFont(new Font(Font.MONOSPACED,Font.BOLD,18));
        JPanel center=new JPanel(new BorderLayout());
        center.setBorder(BorderFactory.createTitledBorder("12 karakterli lisans kodu"));
        center.add(output,BorderLayout.CENTER);
        add(center,BorderLayout.CENTER);

        JPanel actions=new JPanel(new FlowLayout(FlowLayout.RIGHT));
        JButton copy=new JButton("Kopyala");
        copy.addActionListener(e->copy());
        actions.add(copy);
        JButton save=new JButton("Dosyaya Kaydet");
        save.addActionListener(e->save());
        actions.add(save);
        add(actions,BorderLayout.SOUTH);

        pack();
        setMinimumSize(new Dimension(460,getHeight()));
        setLocationRelativeTo(null);
    }

    private void issue(){
        try{
            String selected=(String)term.getSelectedItem();
            int days=selected.startsWith("30")?30:selected.startsWith("90")?90:selected.startsWith("365")?365:0;
            String code=ShortLicenseCode.issue(serial.getText(),days,System.currentTimeMillis());
            output.setText(code);
        }catch(Exception ex){
            JOptionPane.showMessageDialog(this,ex.getMessage(),"Lisans üretilemedi",JOptionPane.ERROR_MESSAGE);
        }
    }

    private void copy(){
        String value=output.getText().trim();
        if(value.isEmpty())return;
        Toolkit.getDefaultToolkit().getSystemClipboard().setContents(new StringSelection(value),null);
    }

    private void save(){
        String value=output.getText().trim();
        if(value.isEmpty())return;
        JFileChooser chooser=new JFileChooser();
        chooser.setSelectedFile(new java.io.File("MusaCAD-lisans.txt"));
        if(chooser.showSaveDialog(this)==JFileChooser.APPROVE_OPTION){
            try{
                Files.writeString(chooser.getSelectedFile().toPath(),value+System.lineSeparator(),StandardCharsets.UTF_8);
            }catch(Exception ex){
                JOptionPane.showMessageDialog(this,ex.getMessage(),"Kaydedilemedi",JOptionPane.ERROR_MESSAGE);
            }
        }
    }

    public static void main(String[]args){
        SwingUtilities.invokeLater(()->new ShortLicenseGeneratorApp().setVisible(true));
    }
}
