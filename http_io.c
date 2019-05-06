// dump1090, a Mode S messages decoder for RTLSDR devices.
//
// Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "dump1090.h"
#include <curl/curl.h>

//
// ============================= Http =============================
// Note: here we disregard any kind of good coding practice in favor of
//=========================================================================
//


//
//=========================================================================
//
void modesInitHttp (void) {
    
};

//
//=========================================================================
//
void modesOutputHttp (struct modesMessage *mm){
    if (Modes.enable) { sendMsgForHttp( mm ); }
};

void sendMsgForHttp(struct modesMessage *mm){
    
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = NULL;
    curl  = curl_easy_init(); 
    if(curl == NULL)
    {
        printf("curl is NULL.\n");
        curl_global_cleanup();        
        return ;
    }
    
    if (curl)
    {
        // curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/tmp/cookie.txt");
        //curl_easy_setopt(curl, CURLOPT_PROXY, "10.99.60.201:8080");
        // curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
        
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "&logintype=uid&u=xieyan&psw=xxx86"); // 指定post内容
        curl_easy_setopt(curl, CURLOPT_URL, Modes.sendurl);                              // 指定url
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 15000);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            printf("发送msg failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return true;
}




