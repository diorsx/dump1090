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
void sendMsgForHttp(char *p) {
    
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
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &p);                                  // 指定post内容
        curl_easy_setopt(curl, CURLOPT_URL, Modes.sendurl);                              // 指定url
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 15000);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            printf("发送msg failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return;
}

void modesInitMsgForHttp(struct modesMessage *mm, char *p) {
    char msg[256];
    p = msg;
    uint32_t     offset;
    struct timeb epocTime_receive, epocTime_now;
    struct tm    stTime_receive, stTime_now;
    int          msgType;

    // Decide on the basic SBS Message Type
    if        ((mm->msgtype ==  4) || (mm->msgtype == 20)) {
        msgType = 5;
    } else if ((mm->msgtype ==  5) || (mm->msgtype == 21)) {
        msgType = 6;
    } else if ((mm->msgtype ==  0) || (mm->msgtype == 16)) {
        msgType = 7;
    } else if  (mm->msgtype == 11) {
        msgType = 8;
    } else if ((mm->msgtype != 17) && (mm->msgtype != 18)) {
        return;
    } else if ((mm->metype >= 1) && (mm->metype <=  4)) {
        msgType = 1;
    } else if ((mm->metype >= 5) && (mm->metype <=  8)) {
        if (mm->bFlags & MODES_ACFLAGS_LATLON_VALID)
            {msgType = 2;}
        else
            {msgType = 7;}
    } else if ((mm->metype >= 9) && (mm->metype <= 18)) {
        if (mm->bFlags & MODES_ACFLAGS_LATLON_VALID)
            {msgType = 3;}
        else
            {msgType = 7;}
    } else if (mm->metype !=  19) {
        return;
    } else if ((mm->mesub == 1) || (mm->mesub == 2)) {
        msgType = 4;
    } else {
        return;
    }

    // Fields 1 to 6 : SBS message type and ICAO address of the aircraft and some other stuff
    p += sprintf(p, "MSG,%d,111,11111,%06X,111111,", msgType, mm->addr); 

    // Find current system time
    ftime(&epocTime_now);                                         // get the current system time & date
    stTime_now = *localtime(&epocTime_now.time);

    // Find message reception time
    if (mm->timestampMsg && !mm->remote) {                        // Make sure the records' timestamp is valid before using it
        epocTime_receive = Modes.stSystemTimeBlk;                 // This is the time of the start of the Block we're processing
        offset   = (int) (mm->timestampMsg - Modes.timestampBlk); // This is the time (in 12Mhz ticks) into the Block
        offset   = offset / 12000;                                // convert to milliseconds
        epocTime_receive.millitm += offset;                       // add on the offset time to the Block start time
        if (epocTime_receive.millitm > 999) {                     // if we've caused an overflow into the next second...
            epocTime_receive.millitm -= 1000;
            epocTime_receive.time ++;                             //    ..correct the overflow
        }
        stTime_receive = *localtime(&epocTime_receive.time);
    } else {
        epocTime_receive = epocTime_now;                          // We don't have a usable reception time; use the current system time
        stTime_receive = stTime_now;
    }

    // Fields 7 & 8 are the message reception time and date
    p += sprintf(p, "%04d/%02d/%02d,", (stTime_receive.tm_year+1900),(stTime_receive.tm_mon+1), stTime_receive.tm_mday);
    p += sprintf(p, "%02d:%02d:%02d.%03d,", stTime_receive.tm_hour, stTime_receive.tm_min, stTime_receive.tm_sec, epocTime_receive.millitm);

    // Fields 9 & 10 are the current time and date
    p += sprintf(p, "%04d/%02d/%02d,", (stTime_now.tm_year+1900),(stTime_now.tm_mon+1), stTime_now.tm_mday);
    p += sprintf(p, "%02d:%02d:%02d.%03d", stTime_now.tm_hour, stTime_now.tm_min, stTime_now.tm_sec, epocTime_now.millitm);

    // Field 11 is the callsign (if we have it)
    if (mm->bFlags & MODES_ACFLAGS_CALLSIGN_VALID) {p += sprintf(p, ",%s", mm->flight);}
    else                                           {p += sprintf(p, ",");}

    // Field 12 is the altitude (if we have it) - force to zero if we're on the ground
    if ((mm->bFlags & MODES_ACFLAGS_AOG_GROUND) == MODES_ACFLAGS_AOG_GROUND) {
        p += sprintf(p, ",0");
    } else if (mm->bFlags & MODES_ACFLAGS_ALTITUDE_VALID) {
        p += sprintf(p, ",%d", mm->altitude);
    } else {
        p += sprintf(p, ",");
    }

    // Field 13 is the ground Speed (if we have it)
    if (mm->bFlags & MODES_ACFLAGS_SPEED_VALID) {
        p += sprintf(p, ",%d", mm->velocity);
    } else {
        p += sprintf(p, ","); 
    }

    // Field 14 is the ground Heading (if we have it)       
    if (mm->bFlags & MODES_ACFLAGS_HEADING_VALID) {
        p += sprintf(p, ",%d", mm->heading);
    } else {
        p += sprintf(p, ",");
    }

    // Fields 15 and 16 are the Lat/Lon (if we have it)
    if (mm->bFlags & MODES_ACFLAGS_LATLON_VALID) {p += sprintf(p, ",%1.5f,%1.5f", mm->fLat, mm->fLon);}
    else                                         {p += sprintf(p, ",,");}

    // Field 17 is the VerticalRate (if we have it)
    if (mm->bFlags & MODES_ACFLAGS_VERTRATE_VALID) {p += sprintf(p, ",%d", mm->vert_rate);}
    else                                           {p += sprintf(p, ",");}

    // Field 18 is  the Squawk (if we have it)
    if (mm->bFlags & MODES_ACFLAGS_SQUAWK_VALID) {p += sprintf(p, ",%x", mm->modeA);}
    else                                         {p += sprintf(p, ",");}

    // Field 19 is the Squawk Changing Alert flag (if we have it)
    if (mm->bFlags & MODES_ACFLAGS_FS_VALID) {
        if ((mm->fs >= 2) && (mm->fs <= 4)) {
            p += sprintf(p, ",-1");
        } else {
            p += sprintf(p, ",0");
        }
    } else {
        p += sprintf(p, ",");
    }

    // Field 20 is the Squawk Emergency flag (if we have it)
    if (mm->bFlags & MODES_ACFLAGS_SQUAWK_VALID) {
        if ((mm->modeA == 0x7500) || (mm->modeA == 0x7600) || (mm->modeA == 0x7700)) {
            p += sprintf(p, ",-1");
        } else {
            p += sprintf(p, ",0");
        }
    } else {
        p += sprintf(p, ",");
    }

    // Field 21 is the Squawk Ident flag (if we have it)
    if (mm->bFlags & MODES_ACFLAGS_FS_VALID) {
        if ((mm->fs >= 4) && (mm->fs <= 5)) {
            p += sprintf(p, ",-1");
        } else {
            p += sprintf(p, ",0");
        }
    } else {
        p += sprintf(p, ",");
    }

    // Field 22 is the OnTheGround flag (if we have it)
    if (mm->bFlags & MODES_ACFLAGS_AOG_VALID) {
        if (mm->bFlags & MODES_ACFLAGS_AOG) {
            p += sprintf(p, ",-1");
        } else {
            p += sprintf(p, ",0");
        }
    } else {
        p += sprintf(p, ",");
    }
    p += sprintf(p, "\r\n");
    return p;
}

void modesOutputHttp (struct modesMessage *mm){
    char *p;
    modesInitMsgForHttp(mm, p);
    sendMsgForHttp( p ); 
};

