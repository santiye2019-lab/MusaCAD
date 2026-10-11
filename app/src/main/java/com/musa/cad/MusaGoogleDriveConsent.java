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
    static boolean consentResult(@NonNull Intent data) {
        try {
            Task<GoogleSignInAccount> task=GoogleSignIn.getSignedInAccountFromIntent(data);
            GoogleSignInAccount account=task.getResult(ApiException.class);
            return account!=null&&GoogleSignIn.hasPermissions(account,FILE_SCOPE);
        }catch(ApiException error){
            return false;
        }catch(Exception error){
            return false;
        }
    }
    private MusaGoogleDriveConsent(){}
}
