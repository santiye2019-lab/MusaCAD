import com.musa.cad.*;
import java.util.*;

public final class MusaAiStructuralAdvancedTest {
    public static void main(String[] args){
        List<MusaAiDrawingIndex.Item> items=Arrays.asList(
            new MusaAiDrawingIndex.Item(1,"TEXT","STATIK","PERDE P08 ZEMIN KAT AKS C/7 25x350"),
            new MusaAiDrawingIndex.Item(2,"TEXT","STATIK","KIRIS K01 1. KAT AKS A/1 30x60"),
            new MusaAiDrawingIndex.Item(3,"TEXT","STATIK","PERDE P08 2. KAT AKS D/7 25x180"),
            new MusaAiDrawingIndex.Item(4,"TEXT","STATIK","KIRIS K145 REZERVASYON R01 20x30 CM"),
            new MusaAiDrawingIndex.Item(5,"TEXT","STATIK","ASANSOR KUYU 220x250 CM"),
            new MusaAiDrawingIndex.Item(6,"TEXT","MIMARI","ASANSOR KUYU 210x250 CM"),
            new MusaAiDrawingIndex.Item(7,"TEXT","STATIK","MERDIVEN M02"),
            new MusaAiDrawingIndex.Item(8,"TEXT","STATIK","5 CM DILATASYON"),
            new MusaAiDrawingIndex.Item(9,"TEXT","STATIK","KONSOL K20"),
            new MusaAiDrawingIndex.Item(10,"TEXT","STATIK","TRANSFER KIRIS K30"),
            new MusaAiDrawingIndex.Item(11,"TEXT","STATIK","DOSEME D08"),
            new MusaAiDrawingIndex.Item(12,"TEXT","STATIK","KISA KOLON S12 ZEMIN KAT AKS E/3 30x50"),
            new MusaAiDrawingIndex.Item(13,"TEXT","STATIK","PERDE BAĞ KİRİŞİ K40 1. KAT AKS D/5 30x80 ÇAPRAZ DONATI"),
            new MusaAiDrawingIndex.Item(14,"TEXT","STATIK","RİJİT DİYAFRAM D08 1. KAT"),
            new MusaAiDrawingIndex.Item(15,"TEXT","STATIK","PERDE P20 BODRUM 1 KAT AKS A/1 30x300"),
            new MusaAiDrawingIndex.Item(16,"TEXT","STATIK","PERDE P20 ZEMIN KAT AKS B/1 25x300"),
            new MusaAiDrawingIndex.Item(17,"TEXT","STATIK","ZEMİN SINIFI ZC ZEMİN TAŞIMA GÜCÜ 250 KPA YATAK KATSAYISI 30000 KN/M3 TEMEL ALT KOTU -4.50 M"),
            new MusaAiDrawingIndex.Item(18,"TEXT","STATIK","PROJE REV B7")
        );
        MusaAiDrawingIndex index=new MusaAiDrawingIndex("KALIP",items.size(),0,
            Arrays.asList("STATIK","MIMARI"),Arrays.asList("STATIK","MIMARI"),items,"cm");

        String report="C35 B420C SDS=1.10\n"+
            "Güçlü kolon zayıf kiriş kontrolü sağlıyor\n"+
            "Kolon kiriş birleşim kontrolü uygun\n"+
            "Düzensizlik A1 kontrolü uygun\n"+
            "Göreli kat ötelenmesi kontrolü uygun\n"+
            "Burulma düzensizliği uygun\n"+
            "Yumuşak kat kontrolü uygun\n"+
            "Modal analiz response spectrum\n"+
            "Kütle katılım oranı X %95 Y %94\n"+
            "Periyot T1X=1.25\n"+
            "Kısa kolon kontrolü uygun\n"+
            "Perde bağ kirişi kontrolü uygun\n"+
            "Rijit diyafram kabulü\n"+
            "Radye temel Kazık temel\n"+
            "Zemin Sınıfı ZC\n"+
            "Zemin Taşıma Gücü 200 KPA\n"+
            "Yatak Katsayısı 30000 KN/M3\n"+
            "Yeraltı Suyu -3.50 M\n"+
            "Temel Alt Kotu -4.50 M\n"+
            "Zemin basıncı ve oturma kontrolü uygun\n"+
            "Kazık kapasitesi kontrolü uygun\n"+
            "Uplift yüzme kontrolü uygun\n"+
            "P-Delta ikinci mertebe kontrolü uygun\n"+
            "Taban kesme kuvveti ve spektrum ölçekleme uygun\n"+
            "Zemin yapı etkileşimi area spring uygun\n"+
            "Kütle kaynağı G + 0.30Q\n"+
            "Kütle merkezi ve rijitlik merkezi eksantrisite kontrolü uygun\n"+
            "Tesadüfi eksantrisite accidental eccentricity uygulanmıştır\n"+
            "Diyafram collector drag strut kontrolü uygun\n"+
            "Düşey deprem vertical seismic etkisi uygun\n"+
            "Yük kombinasyonu COMB1 = 1.4G + 1.6Q\n"+
            "Deprem yük durumu RSX response spectrum X\n"+
            "Deprem yük durumu RSY response spectrum Y\n"+
            "Taşıyıcı sistem katsayıları R/D/I uygundur\n"+
            "Etkin rijitlik cracked section stiffness modifier uygulanmıştır\n"+
            "Kiriş uçlarında end release ve rigid zone tanımları uygundur\n"+
            "Kolon S12 PMM interaction ratio = 0.82 uygun\n"+
            "Kiriş K40 beam shear ratio = 0.71 uygun\n"+
            "Perde P20 wall shear capacity ratio = 0.66 uygun\n"+
            "Kolon S99 capacity ratio = 0.95\n"+
            "Servisabilite sehim deflection = 12 mm uygun\n"+
            "Çatlak genişliği crack width = 0.25 mm uygun\n"+
            "Döşeme titreşim vibration comfort frequency = 8.0 Hz uygun\n"+
            "Uzun süreli sehim creep shrinkage kontrolü uygun\n"+
            "Döşeme titreşim limiti aşıldı\n"+
            "ERROR stiffness matrix singular at joint J45\n"+
            "Unconnected joint J77 detected\n"+
            "Shell mesh quality aspect ratio = 7.5 review required\n"+
            "Non-convergence: nonlinear case NL1 did not converge\n"+
            "Local axis orientation assignment checked and suitable\n"+
            "Model revision REV=B7\n"+
            "Material assignment C35 B420C assigned and suitable\n"+
            "Section assignment property checked and suitable\n"+
            "Story assignment and floor data checked\n"+
            "1. KAT KIRIS K90 AKS A/2 30x60 C35 B420C\n"+
            "1. KAT KIRIS K90 AKS A/2 30x60 C35 B420C\n"+
            "Zero length element geometry check uygun; zero length element yok\n"+
            "Support restraint boundary condition assignment uygun\n"+
            "Diaphragm assignment constraint assignment uygun\n"+
            "Load assignment area load frame load uygun\n"+
            "Self weight multiplier = 1.0 gravity load uygun\n"+
            "Unit system kN-m-C checked and suitable\n"+
            "Design code TBDY 2018 / TS500 checked and suitable\n"+
            "Story elevation coordinate system checked and suitable\n"+
            "Analysis case run status checked and suitable\n"+
            "Combination reference load case reference checked and suitable\n"+
            "Object property compatibility checked and suitable\n"+
            "Shell thickness area thickness property checked and suitable\n"+
            "Pier label and spandrel assignment checked and suitable\n"+
            "Design procedure design status checked and suitable\n"+
            "Auto mesh area mesh assignment checked and suitable\n"+
            "Load pattern type DEAD LIVE WIND checked and suitable\n"+
            "Response spectrum function spectrum direction UX UY checked and suitable\n"+
            "Modal case method Eigen Ritz vector checked and suitable\n"+
            "Damping ratio modal damping checked and suitable\n"+
            "Modal combination CQC directional combination checked and suitable\n"+
            "Time history function ground motion function checked and suitable\n"+
            "Time step duration checked and suitable\n"+
            "Nonlinear hinge assignment plastic hinge property checked and suitable\n"+
            "Nonlinear case parameters maximum iterations tolerance checked and suitable\n"+
            "Staged construction stage sequence checked and suitable\n"+
            "Link property assignment checked and suitable\n"+
            "Viscous damper property checked and suitable\n"+
            "Base isolator property assignment checked and suitable\n"+
            "Prestress tendon force checked and suitable\n"+
            "Tension only cable behavior checked and suitable\n";
        MusaAiStructuralCalc.Model calc=MusaAiStructuralCalc.parse("hesap.txt",report);
        MusaAiStructuralAdvanced.Result result=MusaAiStructuralAdvanced.analyze(index,calc);
        require(result.matched,"advanced structural result must match");
        Set<String> ids=new LinkedHashSet<>();
        for(MusaAiStructuralAdvanced.Finding f:result.findings)ids.add(f.id);

        require(ids.contains("ST-03"),"wall continuity candidate missing");
        require(ids.contains("ST-04"),"axis-change candidate missing");
        require(ids.contains("ST-05"),"section-change candidate missing");
        require(ids.contains("ST-08"),"beam reservation candidate missing");
        require(ids.contains("ST-10"),"elevator coordination candidate missing");
        require(ids.contains("ST-11"),"stair coordination candidate missing");
        require(ids.contains("ST-12"),"dilatation review candidate missing");
        require(ids.contains("ST-16"),"cantilever detail verification missing");
        require(ids.contains("ST-17"),"transfer-system review missing");
        require(ids.contains("ST-18"),"slab thickness verification missing");
        require(ids.contains("ST-20"),"seismic parameter verification missing");
        require(ids.contains("ST-21"),"load assumption verification missing");
        require(ids.contains("ST-22"),"beam-column joint report check missing");
        require(ids.contains("ST-23"),"strong-column weak-beam report check missing");
        require(ids.contains("ST-25"),"irregularity report check missing");
        require(ids.contains("ST-26"),"story drift report check missing");
        require(ids.contains("ST-27"),"torsion report check missing");
        require(ids.contains("ST-28"),"soft/weak story report check missing");
        require(ids.contains("ST-29"),"modal analysis report check missing");
        require(ids.contains("ST-30"),"mass participation report check missing");
        require(ids.contains("ST-31"),"period report check missing");
        require(ids.contains("ST-32"),"short-column check missing");
        require(ids.contains("ST-33"),"coupling-beam check missing");
        require(ids.contains("ST-34"),"diaphragm check missing");
        require(ids.contains("ST-35"),"basement-wall transition check missing");
        require(ids.contains("ST-36"),"geotechnical parameter completeness check missing");
        require(ids.contains("ST-37"),"geotechnical project-report cross-check missing");
        require(ids.contains("ST-38"),"foundation pressure/settlement check missing");
        require(ids.contains("ST-39"),"pile capacity check missing");
        require(ids.contains("ST-40"),"uplift check missing");
        require(ids.contains("ST-41"),"second-order check missing");
        require(ids.contains("ST-42"),"base-shear scaling check missing");
        require(ids.contains("ST-43"),"soil-structure interaction check missing");
        require(ids.contains("ST-44"),"mass-source check missing");
        require(ids.contains("ST-45"),"center/eccentricity check missing");
        require(ids.contains("ST-46"),"accidental-eccentricity check missing");
        require(ids.contains("ST-47"),"diaphragm force-path check missing");
        require(ids.contains("ST-48"),"vertical seismic check missing");
        require(ids.contains("ST-49"),"load-combination check missing");
        require(ids.contains("ST-50"),"seismic-load-case check missing");
        require(ids.contains("ST-51"),"RDI system check missing");
        require(ids.contains("ST-52"),"effective-stiffness check missing");
        require(ids.contains("ST-53"),"release/rigid-zone check missing");
        require(ids.contains("ST-54"),"PMM interaction check missing");
        require(ids.contains("ST-55"),"beam capacity check missing");
        require(ids.contains("ST-56"),"wall shear capacity check missing");
        require(ids.contains("ST-58"),"utilization ranking check missing");
        require(ids.contains("ST-59"),"deflection/serviceability check missing");
        require(ids.contains("ST-60"),"crack-width check missing");
        require(ids.contains("ST-61"),"vibration check missing");
        require(ids.contains("ST-62"),"long-term effects check missing");
        require(ids.contains("ST-63"),"explicit serviceability failure check missing");
        require(ids.contains("ST-64"),"model instability check missing");
        require(ids.contains("ST-65"),"disconnected model check missing");
        require(ids.contains("ST-66"),"mesh quality check missing");
        require(ids.contains("ST-67"),"convergence check missing");
        require(ids.contains("ST-68"),"local-axis check missing");
        require(ids.contains("ST-69"),"revision consistency check missing");
        require(ids.contains("ST-70"),"material assignment check missing");
        require(ids.contains("ST-71"),"section assignment check missing");
        require(ids.contains("ST-72"),"story/element metadata check missing");
        require(ids.contains("ST-73"),"duplicate identity check missing");
        require(ids.contains("ST-74"),"degenerate geometry check missing");
        require(ids.contains("ST-75"),"support boundary assignment check missing");
        require(ids.contains("ST-76"),"diaphragm assignment check missing");
        require(ids.contains("ST-77"),"load assignment integrity check missing");
        require(ids.contains("ST-78"),"self-weight/gravity check missing");
        require(ids.contains("ST-79"),"unit-system check missing");
        require(ids.contains("ST-80"),"design-code check missing");
        require(ids.contains("ST-81"),"story-elevation/coordinate check missing");
        require(ids.contains("ST-82"),"analysis-case status check missing");
        require(ids.contains("ST-83"),"load-case reference integrity check missing");
        require(ids.contains("ST-84"),"object/property compatibility check missing");
        require(ids.contains("ST-85"),"shell thickness assignment check missing");
        require(ids.contains("ST-86"),"pier/spandrel assignment check missing");
        require(ids.contains("ST-87"),"design procedure/status check missing");
        require(ids.contains("ST-88"),"auto-mesh assignment check missing");
        require(ids.contains("ST-89"),"load-pattern type check missing");
        require(ids.contains("ST-90"),"response-spectrum assignment check missing");
        require(ids.contains("ST-91"),"modal-case setup check missing");
        require(ids.contains("ST-92"),"damping definition check missing");
        require(ids.contains("ST-93"),"dynamic combination method check missing");
        require(ids.contains("ST-94"),"time-history function check missing");
        require(ids.contains("ST-95"),"time-step/duration check missing");
        require(ids.contains("ST-96"),"nonlinear hinge assignment check missing");
        require(ids.contains("ST-97"),"nonlinear case control check missing");
        require(ids.contains("ST-98"),"staged-construction check missing");
        require(ids.contains("ST-99"),"link property assignment check missing");
        require(ids.contains("ST-100"),"damper/gap nonlinear-link check missing");
        require(ids.contains("ST-101"),"base-isolator assignment check missing");
        require(ids.contains("ST-102"),"tendon/prestress check missing");
        require(ids.contains("ST-103"),"tension/compression-only behavior check missing");

        require(MusaAiStructuralAdvanced.isFocusedQuery("Modal analizi kontrol et"),"modal focus query missing");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Göreli kat ötelenmesini incele"),"story drift should require report");
        require(!MusaAiStructuralAdvanced.focusedQueryNeedsReport("Zımbalama kontrolü"),"drawing punching check should not always require report");
        require(!MusaAiStructuralAdvanced.isFocusedQuery("Statik projeyi kontrol et"),"generic structural review must stay unfiltered");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Kısa kolon kontrolü yap"),"short-column focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Perde bağ kirişini kontrol et"),"coupling-beam focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Rijit diyaframı kontrol et"),"diaphragm focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Zemin taşıma gücünü kontrol et"),"geotechnical focus query missing");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Zemin taşıma gücünü kontrol et"),"geotechnical focus must require report");
        require(MusaAiStructuralAdvanced.isFocusedQuery("P-Delta kontrolü yap"),"P-Delta focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Taban kesmesini kontrol et"),"base-shear focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Kazık kapasitesini incele"),"pile-capacity focus query missing");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("P-Delta kontrolü yap"),"P-Delta focus must require report");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Kütle kaynağını kontrol et"),"mass-source focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Kütle merkezi rijitlik merkezi eksantrisite kontrolü"),"center/eccentricity focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Tesadüfi eksantrisiteyi kontrol et"),"accidental-eccentricity focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Diyafram collector kontrolü"),"collector focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Düşey deprem etkisini kontrol et"),"vertical seismic focus query missing");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Kütle kaynağını kontrol et"),"mass-source focus must require report");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Düşey deprem etkisini kontrol et"),"vertical seismic focus must require report");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Yük kombinasyonlarını kontrol et"),"load-combination focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Deprem yük durumlarını kontrol et"),"seismic-load-case focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("R/D/I katsayılarını kontrol et"),"RDI focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Etkin rijitlikleri kontrol et"),"effective-stiffness focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Mafsal ve rijit bölge kabullerini kontrol et"),"release/rigid-zone focus query missing");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Yük kombinasyonlarını kontrol et"),"load-combination focus must require report");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Etkin rijitlikleri kontrol et"),"effective-stiffness focus must require report");
        require(MusaAiStructuralAdvanced.isFocusedQuery("PMM oranlarını kontrol et"),"PMM focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Kiriş kesme oranlarını kontrol et"),"beam capacity focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Perde kesme kapasitesini kontrol et"),"wall shear focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Kritik eleman kapasite oranlarını sırala"),"utilization ranking focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Sehim kontrolünü incele"),"deflection focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Çatlak genişliğini kontrol et"),"crack-width focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Döşeme titreşimini kontrol et"),"vibration focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Sünme rötre etkilerini kontrol et"),"long-term effects focus query missing");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Çatlak genişliğini kontrol et"),"crack-width focus must require report");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Tekil rijitlik ve kararsızlık kontrolü"),"instability focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Bağlantısız düğümleri kontrol et"),"connectivity focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Mesh kalitesini kontrol et"),"mesh focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Analiz yakınsamasını kontrol et"),"convergence focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Yerel eksenleri kontrol et"),"local-axis focus query missing");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Mesh kalitesini kontrol et"),"mesh focus must require report");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Model revizyonunu proje ile karşılaştır"),"revision focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Malzeme atamalarını kontrol et"),"material assignment focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Kesit atamalarını kontrol et"),"section assignment focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Kat aks eleman verisini kontrol et"),"story metadata focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Mükerrer eleman kimliklerini kontrol et"),"duplicate identity focus query missing");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Kesit atamalarını kontrol et"),"section assignment focus must require report");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Sıfır uzunluklu elemanları kontrol et"),"degenerate geometry focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Mesnet atamalarını kontrol et"),"support assignment focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Diyafram atamalarını kontrol et"),"diaphragm assignment focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Yük atamalarını kontrol et"),"load assignment focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Öz ağırlık çarpanını kontrol et"),"self-weight focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Birim sistemini kontrol et"),"unit-system focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("TBDY yönetmelik sürümünü kontrol et"),"design-code focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Kat kotlarını kontrol et"),"story-elevation focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Analiz case durumunu kontrol et"),"analysis-case status focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Load case referanslarını kontrol et"),"load-case reference focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Property tip uyumluluğunu kontrol et"),"object/property focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Shell thickness atamalarını kontrol et"),"shell-thickness focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Pier label atamalarını kontrol et"),"pier/spandrel focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Tasarım durumunu kontrol et"),"design-status focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Auto mesh atamalarını kontrol et"),"auto-mesh focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Yük pattern tipini kontrol et"),"load-pattern type focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Response spectrum function kontrol et"),"response-spectrum assignment focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Modal yöntemini kontrol et"),"modal-case setup focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Sönüm oranını kontrol et"),"damping focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("CQC modal combination kontrol et"),"dynamic combination focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Time history function kontrol et"),"time-history function focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Zaman adımını kontrol et"),"time-step focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Plastik mafsal atamalarını kontrol et"),"nonlinear hinge focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Nonlinear solution control kontrol et"),"nonlinear case control focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Yapım aşamalarını kontrol et"),"staged-construction focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Link property atamalarını kontrol et"),"link property focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Viscous damper property kontrol et"),"damper focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Sismik izolatör atamalarını kontrol et"),"isolator focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Ön germe tendonunu kontrol et"),"prestress focus query missing");
        require(MusaAiStructuralAdvanced.isFocusedQuery("Sadece çekme elemanlarını kontrol et"),"tension-only focus query missing");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Sönüm oranını kontrol et"),"damping focus must require report");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Mesnet atamalarını kontrol et"),"support assignment focus must require report");
        require(MusaAiStructuralAdvanced.focusedQueryNeedsReport("Birim sistemini kontrol et"),"unit-system focus must require report");

        MusaAiStructuralAdvanced.Result modalFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Modal analizi kontrol et");
        Set<String>modalIds=new LinkedHashSet<>();
        for(MusaAiStructuralAdvanced.Finding f:modalFocus.findings)modalIds.add(f.id);
        require(modalIds.size()==3&&modalIds.contains("ST-29")&&modalIds.contains("ST-30")&&modalIds.contains("ST-31"),
            "modal focus must only return ST-29/30/31");
        require(!modalFocus.text.contains("[ST-27]"),"modal focus leaked torsion finding");

        MusaAiStructuralAdvanced.Result driftFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Göreli kat ötelenmesini incele");
        require(driftFocus.findings.size()==1&&"ST-26".equals(driftFocus.findings.get(0).id),
            "story-drift focus must only return ST-26");

        MusaAiStructuralAdvanced.Result geoFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Zemin taşıma gücünü kontrol et");
        Set<String>geoIds=new LinkedHashSet<>();
        for(MusaAiStructuralAdvanced.Finding f:geoFocus.findings)geoIds.add(f.id);
        require(geoIds.contains("ST-36")&&geoIds.contains("ST-37"),"geotechnical focus missing ST-36/ST-37");
        require(geoFocus.text.contains("proje 250KPA")&&geoFocus.text.contains("hesap 200KPA"),
            "geotechnical mismatch values missing");

        MusaAiStructuralAdvanced.Result pDeltaFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"P-Delta kontrolü yap");
        require(pDeltaFocus.findings.size()==1&&"ST-41".equals(pDeltaFocus.findings.get(0).id),
            "P-Delta focus must only return ST-41");

        MusaAiStructuralAdvanced.Result massFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Kütle kaynağını kontrol et");
        require(massFocus.findings.size()==1&&"ST-44".equals(massFocus.findings.get(0).id),
            "mass-source focus must only return ST-44");

        MusaAiStructuralAdvanced.Result eccentricityFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Tesadüfi eksantrisiteyi kontrol et");
        require(eccentricityFocus.findings.size()==1&&"ST-46".equals(eccentricityFocus.findings.get(0).id),
            "accidental eccentricity focus must only return ST-46");

        MusaAiStructuralAdvanced.Result verticalFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Düşey deprem etkisini kontrol et");
        require(verticalFocus.findings.size()==1&&"ST-48".equals(verticalFocus.findings.get(0).id),
            "vertical seismic focus must only return ST-48");

        MusaAiStructuralAdvanced.Result loadFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Yük kombinasyonlarını kontrol et");
        require(loadFocus.findings.size()==1&&"ST-49".equals(loadFocus.findings.get(0).id),
            "load combination focus must only return ST-49");

        MusaAiStructuralAdvanced.Result rdiFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"R/D/I katsayılarını kontrol et");
        require(rdiFocus.findings.size()==1&&"ST-51".equals(rdiFocus.findings.get(0).id),
            "RDI focus must only return ST-51");

        MusaAiStructuralAdvanced.Result stiffnessFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Etkin rijitlikleri kontrol et");
        require(stiffnessFocus.findings.size()==1&&"ST-52".equals(stiffnessFocus.findings.get(0).id),
            "effective stiffness focus must only return ST-52");

        MusaAiStructuralAdvanced.Result pmmFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"PMM oranlarını kontrol et");
        require(pmmFocus.findings.size()==1&&"ST-54".equals(pmmFocus.findings.get(0).id),
            "PMM focus must only return ST-54");

        MusaAiStructuralAdvanced.Result utilizationFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Kritik eleman kapasite oranlarını sırala");
        require(utilizationFocus.findings.size()==1&&"ST-58".equals(utilizationFocus.findings.get(0).id),
            "utilization focus must only return ST-58");
        require(utilizationFocus.text.contains("0.950"),"utilization ranking should include highest reported ratio");

        MusaAiStructuralAdvanced.Result crackFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Çatlak genişliğini kontrol et");
        require(crackFocus.findings.size()==1&&"ST-60".equals(crackFocus.findings.get(0).id),
            "crack-width focus must only return ST-60");

        MusaAiStructuralAdvanced.Result vibrationFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Döşeme titreşimini kontrol et");
        require(vibrationFocus.findings.size()==1&&"ST-61".equals(vibrationFocus.findings.get(0).id),
            "vibration focus must only return ST-61");
        require(vibrationFocus.findings.get(0).status==MusaAiStructuralAdvanced.Status.UYUMSUZLUK,
            "vibration limit exceedance must be surfaced as mismatch");

        MusaAiStructuralAdvanced.Result instabilityFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Tekil rijitlik ve kararsızlık kontrolü");
        require(instabilityFocus.findings.size()==1&&"ST-64".equals(instabilityFocus.findings.get(0).id),
            "instability focus must only return ST-64");
        require(instabilityFocus.findings.get(0).status==MusaAiStructuralAdvanced.Status.UYUMSUZLUK,
            "reported singularity must be surfaced as mismatch");

        MusaAiStructuralAdvanced.Result meshFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Mesh kalitesini kontrol et");
        require(meshFocus.findings.size()==1&&"ST-66".equals(meshFocus.findings.get(0).id),
            "mesh focus must only return ST-66");

        MusaAiStructuralAdvanced.Result axisFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Yerel eksenleri kontrol et");
        require(axisFocus.findings.size()==1&&"ST-68".equals(axisFocus.findings.get(0).id),
            "local-axis focus must only return ST-68");
        require(axisFocus.findings.get(0).status==MusaAiStructuralAdvanced.Status.BILGI,
            "reported suitable local-axis assignment should be informational");

        MusaAiStructuralAdvanced.Result revisionFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Model revizyonunu proje ile karşılaştır");
        require(revisionFocus.findings.size()==1&&"ST-69".equals(revisionFocus.findings.get(0).id),
            "revision focus must only return ST-69");
        require(revisionFocus.findings.get(0).status==MusaAiStructuralAdvanced.Status.BILGI,
            "matching project/model revision should be informational: "+revisionFocus.text);

        MusaAiStructuralAdvanced.Result sectionFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Kesit atamalarını kontrol et");
        require(sectionFocus.findings.size()==1&&"ST-71".equals(sectionFocus.findings.get(0).id),
            "section assignment focus must only return ST-71");

        MusaAiStructuralAdvanced.Result duplicateFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Mükerrer eleman kimliklerini kontrol et");
        require(duplicateFocus.findings.size()==1&&"ST-73".equals(duplicateFocus.findings.get(0).id),
            "duplicate identity focus must only return ST-73");
        require(duplicateFocus.findings.get(0).status==MusaAiStructuralAdvanced.Status.INCELEME_GEREKLI,
            "duplicate identical report rows should require review without false conflict");

        MusaAiStructuralAdvanced.Result supportFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Mesnet atamalarını kontrol et");
        require(supportFocus.findings.size()==1&&"ST-75".equals(supportFocus.findings.get(0).id),
            "support assignment focus must only return ST-75");
        require(supportFocus.findings.get(0).status==MusaAiStructuralAdvanced.Status.BILGI,
            "reported suitable support assignment should be informational");

        MusaAiStructuralAdvanced.Result loadAssignmentFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Yük atamalarını kontrol et");
        require(loadAssignmentFocus.findings.size()==1&&"ST-77".equals(loadAssignmentFocus.findings.get(0).id),
            "load assignment focus must only return ST-77");

        MusaAiStructuralAdvanced.Result selfWeightFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Öz ağırlık çarpanını kontrol et");
        require(selfWeightFocus.findings.size()==1&&"ST-78".equals(selfWeightFocus.findings.get(0).id),
            "self-weight focus must only return ST-78");
        require(selfWeightFocus.findings.get(0).status==MusaAiStructuralAdvanced.Status.BILGI,
            "reported self-weight multiplier should be informational");

        MusaAiStructuralAdvanced.Result unitFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Birim sistemini kontrol et");
        require(unitFocus.findings.size()==1&&"ST-79".equals(unitFocus.findings.get(0).id),
            "unit-system focus must only return ST-79");
        require(unitFocus.findings.get(0).status==MusaAiStructuralAdvanced.Status.BILGI,
            "reported unit system should be informational");

        MusaAiStructuralAdvanced.Result codeFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"TBDY yönetmelik sürümünü kontrol et");
        require(codeFocus.findings.size()==1&&"ST-80".equals(codeFocus.findings.get(0).id),
            "design-code focus must only return ST-80");

        MusaAiStructuralAdvanced.Result elevationFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Kat kotlarını kontrol et");
        require(elevationFocus.findings.size()==1&&"ST-81".equals(elevationFocus.findings.get(0).id),
            "story-elevation focus must only return ST-81");

        MusaAiStructuralAdvanced.Result analysisStatusFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Analiz case durumunu kontrol et");
        require(analysisStatusFocus.findings.size()==1&&"ST-82".equals(analysisStatusFocus.findings.get(0).id),
            "analysis-case status focus must only return ST-82");

        MusaAiStructuralAdvanced.Result loadRefFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Load case referanslarını kontrol et");
        require(loadRefFocus.findings.size()==1&&"ST-83".equals(loadRefFocus.findings.get(0).id),
            "load-case reference focus must only return ST-83");

        MusaAiStructuralAdvanced.Result propertyFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Property tip uyumluluğunu kontrol et");
        require(propertyFocus.findings.size()==1&&"ST-84".equals(propertyFocus.findings.get(0).id),
            "object/property focus must only return ST-84");

        MusaAiStructuralAdvanced.Result shellFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Shell thickness atamalarını kontrol et");
        require(shellFocus.findings.size()==1&&"ST-85".equals(shellFocus.findings.get(0).id),
            "shell-thickness focus must only return ST-85");

        MusaAiStructuralAdvanced.Result pierFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Pier label atamalarını kontrol et");
        require(pierFocus.findings.size()==1&&"ST-86".equals(pierFocus.findings.get(0).id),
            "pier/spandrel focus must only return ST-86");

        MusaAiStructuralAdvanced.Result designStatusFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Tasarım durumunu kontrol et");
        require(designStatusFocus.findings.size()==1&&"ST-87".equals(designStatusFocus.findings.get(0).id),
            "design-status focus must only return ST-87");

        MusaAiStructuralAdvanced.Result autoMeshFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Auto mesh atamalarını kontrol et");
        require(autoMeshFocus.findings.size()==1&&"ST-88".equals(autoMeshFocus.findings.get(0).id),
            "auto-mesh focus must only return ST-88");

        MusaAiStructuralAdvanced.Result patternFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Yük pattern tipini kontrol et");
        require(patternFocus.findings.size()==1&&"ST-89".equals(patternFocus.findings.get(0).id),
            "load-pattern focus must only return ST-89");

        MusaAiStructuralAdvanced.Result spectrumSetupFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Response spectrum function kontrol et");
        require(spectrumSetupFocus.findings.size()==1&&"ST-90".equals(spectrumSetupFocus.findings.get(0).id),
            "response-spectrum setup focus must only return ST-90");

        MusaAiStructuralAdvanced.Result modalSetupFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Modal yöntemini kontrol et");
        require(modalSetupFocus.findings.size()==1&&"ST-91".equals(modalSetupFocus.findings.get(0).id),
            "modal setup focus must only return ST-91");

        MusaAiStructuralAdvanced.Result dampingFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Sönüm oranını kontrol et");
        require(dampingFocus.findings.size()==1&&"ST-92".equals(dampingFocus.findings.get(0).id),
            "damping focus must only return ST-92");

        MusaAiStructuralAdvanced.Result combinationFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"CQC modal combination kontrol et");
        require(combinationFocus.findings.size()==1&&"ST-93".equals(combinationFocus.findings.get(0).id),
            "dynamic combination focus must only return ST-93");

        MusaAiStructuralAdvanced.Result thFunctionFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Time history function kontrol et");
        require(thFunctionFocus.findings.size()==1&&"ST-94".equals(thFunctionFocus.findings.get(0).id),
            "time-history function focus must only return ST-94");

        MusaAiStructuralAdvanced.Result timeStepFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Zaman adımını kontrol et");
        require(timeStepFocus.findings.size()==1&&"ST-95".equals(timeStepFocus.findings.get(0).id),
            "time-step focus must only return ST-95");

        MusaAiStructuralAdvanced.Result nonlinearHingeFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Plastik mafsal atamalarını kontrol et");
        require(nonlinearHingeFocus.findings.size()==1&&"ST-96".equals(nonlinearHingeFocus.findings.get(0).id),
            "nonlinear hinge focus must only return ST-96");

        MusaAiStructuralAdvanced.Result nonlinearControlFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Nonlinear solution control kontrol et");
        require(nonlinearControlFocus.findings.size()==1&&"ST-97".equals(nonlinearControlFocus.findings.get(0).id),
            "nonlinear control focus must only return ST-97");

        MusaAiStructuralAdvanced.Result stagedFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Yapım aşamalarını kontrol et");
        require(stagedFocus.findings.size()==1&&"ST-98".equals(stagedFocus.findings.get(0).id),
            "staged-construction focus must only return ST-98");

        MusaAiStructuralAdvanced.Result linkFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Link property atamalarını kontrol et");
        require(linkFocus.findings.size()==1&&"ST-99".equals(linkFocus.findings.get(0).id),
            "link property focus must only return ST-99");

        MusaAiStructuralAdvanced.Result damperFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Viscous damper property kontrol et");
        require(damperFocus.findings.size()==1&&"ST-100".equals(damperFocus.findings.get(0).id),
            "damper focus must only return ST-100");

        MusaAiStructuralAdvanced.Result isolatorFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Sismik izolatör atamalarını kontrol et");
        require(isolatorFocus.findings.size()==1&&"ST-101".equals(isolatorFocus.findings.get(0).id),
            "isolator focus must only return ST-101");

        MusaAiStructuralAdvanced.Result prestressFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Ön germe tendonunu kontrol et");
        require(prestressFocus.findings.size()==1&&"ST-102".equals(prestressFocus.findings.get(0).id),
            "prestress focus must only return ST-102");

        MusaAiStructuralAdvanced.Result tensionOnlyFocus=MusaAiStructuralAdvanced.analyzeFocused(index,calc,"Sadece çekme elemanlarını kontrol et");
        require(tensionOnlyFocus.findings.size()==1&&"ST-103".equals(tensionOnlyFocus.findings.get(0).id),
            "tension-only focus must only return ST-103");

        require(result.text.contains("DOĞRULANAMADI"),"status taxonomy missing");
        require(result.text.contains("hesap sonucu"),"conservative safety note missing");
        System.out.println("MusaAiStructuralAdvancedTest OK");
    }

    private static void require(boolean value,String message){
        if(!value)throw new AssertionError(message);
    }
}
