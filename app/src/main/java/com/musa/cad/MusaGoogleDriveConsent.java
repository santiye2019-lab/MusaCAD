package com.musa.cad;

import android.app.Activity;
import android.content.Intent;
import androidx.annotation.NonNull;
import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.common.api.Scope;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.auth.api.signin.GoogleSignInStatusCodes;

/**
 * Explicit user-consent gateway for the Google Drive file scope.
 * Google Sign-In authentication is NOT a Drive access token: do not pass
 * account.getIdToken() to Drive and do not assume silent sign-in grants scope.
 *
 * The authorization token exchange will be implemented separately using the
 * Google Identity Authorization API. This gateway intentionally cannot upload.
 */
final class MusaGoogleDriveConsent {
    static final int REQUEST_CODE=8341;
    private static final Scope FILE_SCOPE=new Scope(MusaGoogleDriveTransport.SCOPE);

    static GoogleSignInClient client(@NonNull Activity activity) {
        GoogleSignInOptions options=new GoogleSignInOptions.Builder(
            GoogleSignInOptions.DEFAULT_SIGN_IN)
            .requestEmail()
            .requestScopes(FILE_SCOPE)
            .build();
        return GoogleSignIn.getClient(activity,options);
    }
    static void request(@NonNull Activity activity) {
        activity.startActivityForResult(client(activity).getSignInIntent(),REQUEST_CODE);
    }
    static boolean hasScope(@NonNull Activity activity) {
        GoogleSignInAccount account=GoogleSignIn.getLastSignedInAccount(activity);
        return account!=null&&GoogleSignIn.hasPermissions(account,FILE_SCOPE);
    }
    static String consentResult(@NonNull Intent data) {
        try {
            Task<GoogleSignInAccount> task=GoogleSignIn.getSignedInAccountFromIntent(data);
            GoogleSignInAccount account=task.getResult(ApiException.class);
            return account!=null&&GoogleSignIn.hasPermissions(account,FILE_SCOPE)
                ? "Drive dosya izni alındı. Gerçek yükleme bağlantısı henüz etkin değil."
                : "Google hesabı seçildi ancak Drive dosya izni verilmedi.";
        }catch(ApiException error){
            int code=error.getStatusCode();
            if(code==10)return "Google OAuth yapılandırma hatası (kod 10). Sandbox paket kimliği ve imza SHA-1 kaydını kontrol edin.";
            if(code==12501||code==GoogleSignInStatusCodes.SIGN_IN_CANCELLED)
                return "Google hesap bağlantısı iptal edildi (kod "+code+").";
            return "Google hesap bağlantısı başarısız (kod "+code+").";
        }catch(Exception error){
            return "Google bağlantısı işlenemedi; uygulama kayıtlarını kontrol edin.";
        }
    }
    private MusaGoogleDriveConsent(){}
}
