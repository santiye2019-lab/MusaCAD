package com.musa.cad;

/** Pure URL helper shared by Gandalf gateway/session clients. */
public final class MusaAiGatewayEndpoint {
    public static String session(String analyzeUrl){
        String s=analyzeUrl==null?"":analyzeUrl.trim();
        int query=s.indexOf('?');if(query>=0)s=s.substring(0,query);
        if(s.endsWith("/analyze"))return s.substring(0,s.length()-"/analyze".length())+"/session";
        int slash=s.lastIndexOf('/');
        return slash>="https://".length()?s.substring(0,slash)+"/session":s+"/session";
    }
    private MusaAiGatewayEndpoint(){}
}
