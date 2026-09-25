package com.musa.cad;

import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.ProductDetails;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PendingPurchasesParams;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.QueryProductDetailsParams;
import com.android.billingclient.api.QueryPurchasesParams;

import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Google Play yearly MusaCAD license renewal flow with mandatory backend verification. */
public final class PlayBillingManager implements PurchasesUpdatedListener, BillingClientStateListener {
    public interface Listener {
        void onProductReady(boolean ready,String displayPrice);
        void onEntitlementChanged(boolean active);
        void onBillingMessage(String message);
    }

    private final Context context;
    private final Listener listener;
    private final Handler mainHandler=new Handler(Looper.getMainLooper());
    private final BillingClient billingClient;
    private final ExecutorService verifierExecutor=Executors.newSingleThreadExecutor();
    private final Set<String> verifyingTokens=ConcurrentHashMap.newKeySet();
    private ProductDetails productDetails;
    private String offerToken;
    private boolean started;

    public PlayBillingManager(Context context,Listener listener){
        this.context=context.getApplicationContext();
        this.listener=listener;
        billingClient=BillingClient.newBuilder(this.context)
            .setListener(this)
            .enablePendingPurchases(PendingPurchasesParams.newBuilder().enableOneTimeProducts().build())
            .enableAutoServiceReconnection()
            .build();
    }

    public void start(){
        if(started)return;
        started=true;
        billingClient.startConnection(this);
    }

    public void refresh(){
        if(!billingClient.isReady())return;
        queryOwnedPurchases();
        queryProduct();
    }

    public void launchPurchase(Activity activity){
        if(activity==null)return;
        if(!LicenseManager.eligibleForPlayYearlyRenewal(context)){
            notifyMessage("Google Play yalnızca mevcut yıllık MusaCAD lisansını yenilemek için kullanılabilir");
            return;
        }
        if(!secureVerificationConfigured()){
            notifyMessage("Google Play satın alma doğrulama sunucusu yapılandırılmadı");
            return;
        }
        if(!billingClient.isReady()){
            notifyMessage("Google Play bağlantısı henüz hazır değil");
            return;
        }
        if(productDetails==null||offerToken==null||offerToken.isEmpty()){
            queryProduct();
            notifyMessage("Satın alma bilgileri hazırlanıyor");
            return;
        }

        BillingFlowParams.ProductDetailsParams item=
            BillingFlowParams.ProductDetailsParams.newBuilder()
                .setProductDetails(productDetails)
                .setOfferToken(offerToken)
                .build();
        BillingFlowParams params=BillingFlowParams.newBuilder()
            .setProductDetailsParamsList(Collections.singletonList(item))
            .setObfuscatedAccountId(LicenseManager.installationId(context))
            .build();
        BillingResult result=billingClient.launchBillingFlow(activity,params);
        if(result.getResponseCode()!=BillingClient.BillingResponseCode.OK){
            notifyMessage(result.getDebugMessage().isEmpty()?"Google Play satın alma ekranı açılamadı":result.getDebugMessage());
        }
    }

    @Override public void onBillingSetupFinished(BillingResult result){
        if(result.getResponseCode()==BillingClient.BillingResponseCode.OK){
            queryOwnedPurchases();
            queryProduct();
        }else{
            notifyProductReady(false,"");
        }
    }

    @Override public void onBillingServiceDisconnected(){
        notifyProductReady(false,"");
    }

    @Override public void onPurchasesUpdated(BillingResult result,List<Purchase> purchases){
        int code=result.getResponseCode();
        if(code==BillingClient.BillingResponseCode.OK&&purchases!=null){
            boolean matched=false;
            for(Purchase purchase:purchases){
                if(matchesProduct(purchase)){
                    matched=true;
                    processPurchase(purchase);
                }
            }
            if(!matched)queryOwnedPurchases();
        }else if(code==BillingClient.BillingResponseCode.USER_CANCELED){
            notifyMessage("Satın alma iptal edildi");
        }else if(code!=BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED){
            notifyMessage(result.getDebugMessage().isEmpty()?"Satın alma tamamlanamadı":result.getDebugMessage());
        }else{
            queryOwnedPurchases();
        }
    }

    private void queryProduct(){
        String productId=BuildConfig.PLAY_YEARLY_PRODUCT_ID==null?"":BuildConfig.PLAY_YEARLY_PRODUCT_ID.trim();
        if(productId.isEmpty()||!secureVerificationConfigured()){
            notifyProductReady(false,"");
            return;
        }
        QueryProductDetailsParams.Product product=
            QueryProductDetailsParams.Product.newBuilder()
                .setProductId(productId)
                .setProductType(BillingClient.ProductType.SUBS)
                .build();
        QueryProductDetailsParams params=QueryProductDetailsParams.newBuilder()
            .setProductList(Collections.singletonList(product))
            .build();

        billingClient.queryProductDetailsAsync(params,(result,detailsResult)->{
            if(result.getResponseCode()!=BillingClient.BillingResponseCode.OK
                    ||detailsResult==null
                    ||detailsResult.getProductDetailsList().isEmpty()){
                productDetails=null;
                offerToken=null;
                notifyProductReady(false,"");
                return;
            }
            ProductDetails details=detailsResult.getProductDetailsList().get(0);
            List<ProductDetails.SubscriptionOfferDetails> offers=details.getSubscriptionOfferDetails();
            if(offers==null||offers.isEmpty()){
                productDetails=null;
                offerToken=null;
                notifyProductReady(false,"");
                return;
            }
            ProductDetails.SubscriptionOfferDetails offer=offers.get(0);
            productDetails=details;
            offerToken=offer.getOfferToken();
            String price="";
            if(offer.getPricingPhases()!=null
                    && offer.getPricingPhases().getPricingPhaseList()!=null
                    && !offer.getPricingPhases().getPricingPhaseList().isEmpty()){
                price=offer.getPricingPhases().getPricingPhaseList().get(0).getFormattedPrice();
            }
            notifyProductReady(LicenseManager.eligibleForPlayYearlyRenewal(context),price);
        });
    }

    private void queryOwnedPurchases(){
        QueryPurchasesParams params=QueryPurchasesParams.newBuilder()
            .setProductType(BillingClient.ProductType.SUBS)
            .build();
        billingClient.queryPurchasesAsync(params,(result,purchases)->{
            if(result.getResponseCode()!=BillingClient.BillingResponseCode.OK)return;
            boolean owned=false;
            if(purchases!=null){
                for(Purchase purchase:purchases){
                    if(matchesProduct(purchase)&&purchase.getPurchaseState()==Purchase.PurchaseState.PURCHASED){
                        owned=true;
                        processPurchase(purchase);
                    }
                }
            }
            if(!owned){
                LicenseManager.setPlayEntitlement(context,false,0L);
                notifyEntitlement(false);
            }
        });
    }

    private boolean matchesProduct(Purchase purchase){
        if(purchase==null)return false;
        String productId=BuildConfig.PLAY_YEARLY_PRODUCT_ID==null?"":BuildConfig.PLAY_YEARLY_PRODUCT_ID.trim();
        return !productId.isEmpty()&&purchase.getProducts()!=null&&purchase.getProducts().contains(productId);
    }

    private void processPurchase(Purchase purchase){
        if(purchase.getPurchaseState()==Purchase.PurchaseState.PENDING){
            notifyMessage("Ödeme beklemede. Google Play yıllık lisans yenilemesi tamamlandığında erişim güncellenecek.");
            return;
        }
        if(purchase.getPurchaseState()!=Purchase.PurchaseState.PURCHASED)return;

        String token=purchase.getPurchaseToken();
        if(token==null||token.trim().isEmpty()){
            notifyMessage("Google Play satın alma jetonu alınamadı");
            return;
        }
        token=token.trim();
        if(!verifyingTokens.add(token))return;
        final String purchaseToken=token;
        verifierExecutor.execute(()->{
            PlayPurchaseVerifier.Result verification=PlayPurchaseVerifier.verify(context,purchaseToken);
            verifyingTokens.remove(purchaseToken);
            handleVerification(verification);
        });
    }

    private void handleVerification(PlayPurchaseVerifier.Result verification){
        if(verification==null)return;
        switch(verification.status){
            case ACTIVE:
                LicenseManager.setPlayEntitlement(context,true,verification.expiresAtMs);
                notifyEntitlement(true);
                notifyMessage("Google Play yıllık lisans yenilemesi doğrulandı");
                break;
            case PENDING:
                notifyMessage("Ödeme beklemede. Google Play yıllık lisans yenilemesi tamamlanınca erişim güncellenecek.");
                break;
            case DENIED:
                LicenseManager.setPlayEntitlement(context,false,0L);
                notifyEntitlement(false);
                notifyMessage(verification.message==null||verification.message.isEmpty()
                    ?"Google Play satın alımı doğrulanamadı":verification.message);
                break;
            case NOT_CONFIGURED:
                notifyMessage("Google Play satın alma doğrulama sunucusu yapılandırılmadı");
                break;
            case NETWORK_ERROR:
                notifyMessage(verification.message==null||verification.message.isEmpty()
                    ?"Google Play doğrulama sunucusuna ulaşılamadı":verification.message);
                break;
            case INVALID_RESPONSE:
            default:
                notifyMessage(verification.message==null||verification.message.isEmpty()
                    ?"Google Play doğrulama yanıtı geçersiz":verification.message);
                break;
        }
    }

    private boolean secureVerificationConfigured(){
        String endpoint=BuildConfig.PLAY_VERIFY_URL==null?"":BuildConfig.PLAY_VERIFY_URL.trim();
        return endpoint.startsWith("https://");
    }

    private void notifyProductReady(boolean ready,String price){
        if(listener==null)return;
        mainHandler.post(()->listener.onProductReady(ready,price==null?"":price));
    }

    private void notifyEntitlement(boolean active){
        if(listener==null)return;
        mainHandler.post(()->listener.onEntitlementChanged(active));
    }

    private void notifyMessage(String message){
        if(listener==null||message==null||message.trim().isEmpty())return;
        mainHandler.post(()->listener.onBillingMessage(message));
    }

    public void close(){
        verifierExecutor.shutdownNow();
        if(billingClient.isReady())billingClient.endConnection();
    }
}
