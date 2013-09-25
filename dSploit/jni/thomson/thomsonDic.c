/*
 * Copyright 2012 Rui Araújo, Luís Fonseca
 *
 * This file is part of Router Keygen.
 *
 * Router Keygen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Router Keygen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Router Keygen.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "it_evilsocket_dsploit_wifi_algorithms_ThomsonKeygen.h"
#include <ctype.h>
#include <string.h>
#include "sha.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <android/log.h>


static char charectbytes0[] = {
        '3','3','3','3','3','3',
        '3','3','3','3','4','4',
        '4','4','4','4','4','4',
        '4','4','4','4','4','4',
        '4','5','5','5','5','5',
        '5','5','5','5','5','5',
        };

static char charectbytes1[] = {
	'0','1','2','3','4','5',
	'6','7','8','9','1','2',
	'3','4','5','6','7','8',
	'9','A','B','C','D','E',
	'F','0','1','2','3','4',
	'5','6','7','8','9','A',
	};

JNIEXPORT jobjectArray JNICALL Java_it_evilsocket_dsploit_wifi_algorithms_ThomsonKeygen_thirdDicNative
  (JNIEnv * env, jobject obj , jbyteArray ess , jbyteArray ent, jint size)
{
	int year = 4;
	int week = 1;
	int i = 0 , j = 0;
	char  debug[80];
	char input[13];
	char result[5][11];
	int keys = 0;
	unsigned int sequenceNumber;
	unsigned int inc = 0;
	uint8_t message_digest[20];
	SHA_CTX sha1;
	int a,b,c;

    jclass cls = (*env)->GetObjectClass(env, obj);
    jfieldID fid_s = (*env)->GetFieldID(env, cls, "stopRequested", "Z");
    if ( fid_s == NULL ) {
        return; /* exception already thrown */
    }
    unsigned char stop = (*env)->GetBooleanField(env, obj, fid_s);
    int len = size;
    jbyte *entry_native= (*env)->GetByteArrayElements(env, ent, 0);
    uint8_t * entry = ( uint8_t * )malloc( len * sizeof(uint8_t) );
    for( i = 0; i < len; ++i  )
    {
    	entry[i] = entry_native[i];
    }
    jbyte *e_native= (*env)->GetByteArrayElements(env, ess, 0);
    uint8_t ssid[3];
    ssid[0] = e_native[0];
    ssid[1] = e_native[1];
    ssid[2] = e_native[2];
	input[0] = 'C';
	input[1] = 'P';
	input[2] = '0';
	sequenceNumber = 0;
	for( i = 0; i < len; i+=2  )
	{
		sequenceNumber += ( entry[i + 0 ]<<8 ) |  entry[i + 1 ];
		for ( j = 0 ; j < 18 ; ++j )
		{
			inc = j* ( 36*36*36*6*3);
			year = ( (sequenceNumber+inc) / ( 36*36*36 )% 6) + 4 ;
			week = (sequenceNumber+inc) / ( 36*36*36*6 )  + 1 ;
			c = sequenceNumber % 36;
			b = sequenceNumber/36 % 36;
			a = sequenceNumber/(36*36) % 36;

			input[3] = '0' + year % 10 ;
			input[4] = '0' + week / 10;
			input[5] = '0' + week % 10;
			input[6] = charectbytes0[a];
			input[7] = charectbytes1[a];
			input[8] = charectbytes0[b];
			input[9] = charectbytes1[b];
			input[10] = charectbytes0[c];
			input[11] = charectbytes1[c];
			SHA1_Init(&sha1);
			SHA1_Update(&sha1 ,(const void *) input , 12 );
			SHA1_Final(message_digest , &sha1 );
			if( ( memcmp(&message_digest[17],&ssid[0],3) == 0) ){
			sprintf( result[keys++], "%02X%02X%02X%02X%02X\0" , message_digest[0], message_digest[1] ,
								message_digest[2] , message_digest[3], message_digest[4] );
			}
		}
	}
	jobjectArray ret;
	ret= (jobjectArray)(*env)->NewObjectArray(env,keys, (*env)->FindClass(env,"java/lang/String"),0);
	for ( i = 0; i < keys ; ++i )
		(*env)->SetObjectArrayElement(env,ret,i,(*env)->NewStringUTF(env, result[i]));
	(*env)->ReleaseByteArrayElements(env, ess, e_native, 0);
 	return ret;
}

