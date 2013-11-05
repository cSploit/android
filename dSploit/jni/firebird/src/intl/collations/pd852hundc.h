/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

const int NUM_EXPAND_CHARS		= 3;
const int NUM_COMPRESS_CHARS	= 24;
const int LOWERCASE_LEN			= 256;
const int UPPERCASE_LEN			= 256;
const int NOCASESORT_LEN		= 256;
const int LDRV_TIEBREAK			= LOCAL_EXPAND;

//const int MAX_NCO_PRIMARY		= 211;
const int MAX_NCO_SECONDARY		= 10;
const int MAX_NCO_TERTIARY		= 0;
//const int MAX_NCO_IGNORE		= 0;
const int NULL_SECONDARY		= 0;
const int NULL_TERTIARY			= 0;
//const int FIRST_IGNORE			= 1;
const int FIRST_TERTIARY		= 1;
const int FIRST_SECONDARY		= (FIRST_TERTIARY + MAX_NCO_TERTIARY + 1);
const int FIRST_PRIMARY			= (FIRST_SECONDARY + MAX_NCO_SECONDARY + 1);

static const BYTE ToUpperConversionTbl[UPPERCASE_LEN] = {
	0,							/*     0 ->     0 */
	1,							/*     1 ->     1 */
	2,							/*     2 ->     2 */
	3,							/*     3 ->     3 */
	4,							/*     4 ->     4 */
	5,							/*     5 ->     5 */
	6,							/*     6 ->     6 */
	7,							/*     7 ->     7 */
	8,							/*     8 ->     8 */
	9,							/*     9 ->     9 */
	10,							/*    10 ->    10 */
	11,							/*    11 ->    11 */
	12,							/*    12 ->    12 */
	13,							/*    13 ->    13 */
	14,							/*    14 ->    14 */
	15,							/*    15 ->    15 */
	16,							/*    16 ->    16 */
	17,							/*    17 ->    17 */
	18,							/*    18 ->    18 */
	19,							/*    19 ->    19 */
	20,							/*    20 ->    20 */
	21,							/*    21 ->    21 */
	22,							/*    22 ->    22 */
	23,							/*    23 ->    23 */
	24,							/*    24 ->    24 */
	25,							/*    25 ->    25 */
	26,							/*    26 ->    26 */
	27,							/*    27 ->    27 */
	28,							/*    28 ->    28 */
	29,							/*    29 ->    29 */
	30,							/*    30 ->    30 */
	31,							/*    31 ->    31 */
	32,							/*    32 ->    32 */
	33,							/* !  33 -> !  33 */
	34,							/* "  34 -> "  34 */
	35,							/* #  35 -> #  35 */
	36,							/* $  36 -> $  36 */
	37,							/* %  37 -> %  37 */
	38,							/* &  38 -> &  38 */
	39,							/* '  39 -> '  39 */
	40,							/* (  40 -> (  40 */
	41,							/* )  41 -> )  41 */
	42,							/* *  42 -> *  42 */
	43,							/* +  43 -> +  43 */
	44,							/* ,  44 -> ,  44 */
	45,							/* -  45 -> -  45 */
	46,							/* .  46 -> .  46 */
	47,							/* /  47 -> /  47 */
	48,							/* 0  48 -> 0  48 */
	49,							/* 1  49 -> 1  49 */
	50,							/* 2  50 -> 2  50 */
	51,							/* 3  51 -> 3  51 */
	52,							/* 4  52 -> 4  52 */
	53,							/* 5  53 -> 5  53 */
	54,							/* 6  54 -> 6  54 */
	55,							/* 7  55 -> 7  55 */
	56,							/* 8  56 -> 8  56 */
	57,							/* 9  57 -> 9  57 */
	58,							/* :  58 -> :  58 */
	59,							/* ;  59 -> ;  59 */
	60,							/* <  60 -> <  60 */
	61,							/* =  61 -> =  61 */
	62,							/* >  62 -> >  62 */
	63,							/* ?  63 -> ?  63 */
	64,							/* @  64 -> @  64 */
	65,							/* A  65 -> A  65 */
	66,							/* B  66 -> B  66 */
	67,							/* C  67 -> C  67 */
	68,							/* D  68 -> D  68 */
	69,							/* E  69 -> E  69 */
	70,							/* F  70 -> F  70 */
	71,							/* G  71 -> G  71 */
	72,							/* H  72 -> H  72 */
	73,							/* I  73 -> I  73 */
	74,							/* J  74 -> J  74 */
	75,							/* K  75 -> K  75 */
	76,							/* L  76 -> L  76 */
	77,							/* M  77 -> M  77 */
	78,							/* N  78 -> N  78 */
	79,							/* O  79 -> O  79 */
	80,							/* P  80 -> P  80 */
	81,							/* Q  81 -> Q  81 */
	82,							/* R  82 -> R  82 */
	83,							/* S  83 -> S  83 */
	84,							/* T  84 -> T  84 */
	85,							/* U  85 -> U  85 */
	86,							/* V  86 -> V  86 */
	87,							/* W  87 -> W  87 */
	88,							/* X  88 -> X  88 */
	89,							/* Y  89 -> Y  89 */
	90,							/* Z  90 -> Z  90 */
	91,							/* [  91 -> [  91 */
	92,							/* \  92 -> \  92 */
	93,							/* ]  93 -> ]  93 */
	94,							/* ^  94 -> ^  94 */
	95,							/* _  95 -> _  95 */
	96,							/* `  96 -> `  96 */
	65,							/* a  97 -> A  65 */
	66,							/* b  98 -> B  66 */
	67,							/* c  99 -> C  67 */
	68,							/* d 100 -> D  68 */
	69,							/* e 101 -> E  69 */
	70,							/* f 102 -> F  70 */
	71,							/* g 103 -> G  71 */
	72,							/* h 104 -> H  72 */
	73,							/* i 105 -> I  73 */
	74,							/* j 106 -> J  74 */
	75,							/* k 107 -> K  75 */
	76,							/* l 108 -> L  76 */
	77,							/* m 109 -> M  77 */
	78,							/* n 110 -> N  78 */
	79,							/* o 111 -> O  79 */
	80,							/* p 112 -> P  80 */
	81,							/* q 113 -> Q  81 */
	82,							/* r 114 -> R  82 */
	83,							/* s 115 -> S  83 */
	84,							/* t 116 -> T  84 */
	85,							/* u 117 -> U  85 */
	86,							/* v 118 -> V  86 */
	87,							/* w 119 -> W  87 */
	88,							/* x 120 -> X  88 */
	89,							/* y 121 -> Y  89 */
	90,							/* z 122 -> Z  90 */
	123,						/* { 123 -> { 123 */
	124,						/* | 124 -> | 124 */
	125,						/* } 125 -> } 125 */
	126,						/* ~ 126 -> ~ 126 */
	127,						/*   127 ->   127 */
	128,						/*   128 ->   128 */
	154,						/*   129 ->   154 */
	144,						/*   130 ->   144 */
	131,						/*   131 ->   131 */
	132,						/*   132 ->   132 */
	133,						/*   133 ->   133 */
	134,						/*   134 ->   134 */
	135,						/*   135 ->   135 */
	136,						/*   136 ->   136 */
	137,						/*   137 ->   137 */
	138,						/*   138 ->   138 */
	138,						/*   139 ->   138 */
	140,						/*   140 ->   140 */
	141,						/*   141 ->   141 */
	142,						/*   142 ->   142 */
	143,						/*   143 ->   143 */
	144,						/*   144 ->   144 */
	145,						/*   145 ->   145 */
	146,						/*   146 ->   146 */
	167,						/*   147 -> § 167 */
	153,						/*   148 ->   153 */
	149,						/*   149 ->   149 */
	152,						/*   150 ->   152 */
	151,						/*   151 ->   151 */
	152,						/*   152 ->   152 */
	153,						/*   153 ->   153 */
	154,						/*   154 ->   154 */
	155,						/*   155 ->   155 */
	156,						/*   156 ->   156 */
	157,						/*   157 ->   157 */
	158,						/*   158 ->   158 */
	159,						/*   159 ->   159 */
	181,						/*   160 -> µ 181 */
	214,						/* ¡ 161 -> Ö 214 */
	224,						/* ¢ 162 -> à 224 */
	233,						/* £ 163 -> é 233 */
	164,						/* ¤ 164 -> ¤ 164 */
	165,						/* ¥ 165 -> ¥ 165 */
	166,						/* ¦ 166 -> ¦ 166 */
	167,						/* § 167 -> § 167 */
	168,						/* ¨ 168 -> ¨ 168 */
	169,						/* © 169 -> © 169 */
	170,						/* ª 170 -> ª 170 */
	171,						/* « 171 -> « 171 */
	172,						/* ¬ 172 -> ¬ 172 */
	173,						/* ­ 173 -> ­ 173 */
	174,						/* ® 174 -> ® 174 */
	175,						/* ¯ 175 -> ¯ 175 */
	176,						/* ° 176 -> ° 176 */
	177,						/* ± 177 -> ± 177 */
	178,						/* ² 178 -> ² 178 */
	179,						/* ³ 179 -> ³ 179 */
	180,						/* ´ 180 -> ´ 180 */
	181,						/* µ 181 -> µ 181 */
	182,						/* ¶ 182 -> ¶ 182 */
	183,						/* · 183 -> · 183 */
	184,						/* ¸ 184 -> ¸ 184 */
	185,						/* ¹ 185 -> ¹ 185 */
	186,						/* º 186 -> º 186 */
	187,						/* » 187 -> » 187 */
	188,						/* ¼ 188 -> ¼ 188 */
	189,						/* ½ 189 -> ½ 189 */
	190,						/* ¾ 190 -> ¾ 190 */
	191,						/* ¿ 191 -> ¿ 191 */
	192,						/* À 192 -> À 192 */
	193,						/* Á 193 -> Á 193 */
	194,						/* Â 194 -> Â 194 */
	195,						/* Ã 195 -> Ã 195 */
	196,						/* Ä 196 -> Ä 196 */
	197,						/* Å 197 -> Å 197 */
	198,						/* Æ 198 -> Æ 198 */
	199,						/* Ç 199 -> Ç 199 */
	200,						/* È 200 -> È 200 */
	201,						/* É 201 -> É 201 */
	202,						/* Ê 202 -> Ê 202 */
	203,						/* Ë 203 -> Ë 203 */
	204,						/* Ì 204 -> Ì 204 */
	205,						/* Í 205 -> Í 205 */
	206,						/* Î 206 -> Î 206 */
	207,						/* Ï 207 -> Ï 207 */
	208,						/* Ð 208 -> Ð 208 */
	209,						/* Ñ 209 -> Ñ 209 */
	210,						/* Ò 210 -> Ò 210 */
	211,						/* Ó 211 -> Ó 211 */
	212,						/* Ô 212 -> Ô 212 */
	213,						/* Õ 213 -> Õ 213 */
	214,						/* Ö 214 -> Ö 214 */
	215,						/* × 215 -> × 215 */
	216,						/* Ø 216 -> Ø 216 */
	217,						/* Ù 217 -> Ù 217 */
	218,						/* Ú 218 -> Ú 218 */
	219,						/* Û 219 -> Û 219 */
	220,						/* Ü 220 -> Ü 220 */
	221,						/* Ý 221 -> Ý 221 */
	222,						/* Þ 222 -> Þ 222 */
	223,						/* ß 223 -> ß 223 */
	224,						/* à 224 -> à 224 */
	225,						/* á 225 -> á 225 */
	226,						/* â 226 -> â 226 */
	227,						/* ã 227 -> ã 227 */
	228,						/* ä 228 -> ä 228 */
	229,						/* å 229 -> å 229 */
	230,						/* æ 230 -> æ 230 */
	231,						/* ç 231 -> ç 231 */
	232,						/* è 232 -> è 232 */
	233,						/* é 233 -> é 233 */
	234,						/* ê 234 -> ê 234 */
	235,						/* ë 235 -> ë 235 */
	236,						/* ì 236 -> ì 236 */
	237,						/* í 237 -> í 237 */
	238,						/* î 238 -> î 238 */
	239,						/* ï 239 -> ï 239 */
	240,						/* ð 240 -> ð 240 */
	241,						/* ñ 241 -> ñ 241 */
	242,						/* ò 242 -> ò 242 */
	243,						/* ó 243 -> ó 243 */
	244,						/* ô 244 -> ô 244 */
	245,						/* õ 245 -> õ 245 */
	246,						/* ö 246 -> ö 246 */
	247,						/* ÷ 247 -> ÷ 247 */
	248,						/* ø 248 -> ø 248 */
	249,						/* ù 249 -> ù 249 */
	250,						/* ú 250 -> ú 250 */
	235,						/* û 251 -> ë 235 */
	252,						/* ü 252 -> ü 252 */
	253,						/* ý 253 -> ý 253 */
	254,						/* þ 254 -> þ 254 */
	255							/*   255 ->   255 */
};

static const BYTE ToLowerConversionTbl[LOWERCASE_LEN] = {
	0,							/*     0 ->     0 */
	1,							/*     1 ->     1 */
	2,							/*     2 ->     2 */
	3,							/*     3 ->     3 */
	4,							/*     4 ->     4 */
	5,							/*     5 ->     5 */
	6,							/*     6 ->     6 */
	7,							/*     7 ->     7 */
	8,							/*     8 ->     8 */
	9,							/*     9 ->     9 */
	10,							/*    10 ->    10 */
	11,							/*    11 ->    11 */
	12,							/*    12 ->    12 */
	13,							/*    13 ->    13 */
	14,							/*    14 ->    14 */
	15,							/*    15 ->    15 */
	16,							/*    16 ->    16 */
	17,							/*    17 ->    17 */
	18,							/*    18 ->    18 */
	19,							/*    19 ->    19 */
	20,							/*    20 ->    20 */
	21,							/*    21 ->    21 */
	22,							/*    22 ->    22 */
	23,							/*    23 ->    23 */
	24,							/*    24 ->    24 */
	25,							/*    25 ->    25 */
	26,							/*    26 ->    26 */
	27,							/*    27 ->    27 */
	28,							/*    28 ->    28 */
	29,							/*    29 ->    29 */
	30,							/*    30 ->    30 */
	31,							/*    31 ->    31 */
	32,							/*    32 ->    32 */
	33,							/* !  33 -> !  33 */
	34,							/* "  34 -> "  34 */
	35,							/* #  35 -> #  35 */
	36,							/* $  36 -> $  36 */
	37,							/* %  37 -> %  37 */
	38,							/* &  38 -> &  38 */
	39,							/* '  39 -> '  39 */
	40,							/* (  40 -> (  40 */
	41,							/* )  41 -> )  41 */
	42,							/* *  42 -> *  42 */
	43,							/* +  43 -> +  43 */
	44,							/* ,  44 -> ,  44 */
	45,							/* -  45 -> -  45 */
	46,							/* .  46 -> .  46 */
	47,							/* /  47 -> /  47 */
	48,							/* 0  48 -> 0  48 */
	49,							/* 1  49 -> 1  49 */
	50,							/* 2  50 -> 2  50 */
	51,							/* 3  51 -> 3  51 */
	52,							/* 4  52 -> 4  52 */
	53,							/* 5  53 -> 5  53 */
	54,							/* 6  54 -> 6  54 */
	55,							/* 7  55 -> 7  55 */
	56,							/* 8  56 -> 8  56 */
	57,							/* 9  57 -> 9  57 */
	58,							/* :  58 -> :  58 */
	59,							/* ;  59 -> ;  59 */
	60,							/* <  60 -> <  60 */
	61,							/* =  61 -> =  61 */
	62,							/* >  62 -> >  62 */
	63,							/* ?  63 -> ?  63 */
	64,							/* @  64 -> @  64 */
	97,							/* A  65 -> a  97 */
	98,							/* B  66 -> b  98 */
	99,							/* C  67 -> c  99 */
	100,						/* D  68 -> d 100 */
	101,						/* E  69 -> e 101 */
	102,						/* F  70 -> f 102 */
	103,						/* G  71 -> g 103 */
	104,						/* H  72 -> h 104 */
	105,						/* I  73 -> i 105 */
	106,						/* J  74 -> j 106 */
	107,						/* K  75 -> k 107 */
	108,						/* L  76 -> l 108 */
	109,						/* M  77 -> m 109 */
	110,						/* N  78 -> n 110 */
	111,						/* O  79 -> o 111 */
	112,						/* P  80 -> p 112 */
	113,						/* Q  81 -> q 113 */
	114,						/* R  82 -> r 114 */
	115,						/* S  83 -> s 115 */
	116,						/* T  84 -> t 116 */
	117,						/* U  85 -> u 117 */
	118,						/* V  86 -> v 118 */
	119,						/* W  87 -> w 119 */
	120,						/* X  88 -> x 120 */
	121,						/* Y  89 -> y 121 */
	122,						/* Z  90 -> z 122 */
	91,							/* [  91 -> [  91 */
	92,							/* \  92 -> \  92 */
	93,							/* ]  93 -> ]  93 */
	94,							/* ^  94 -> ^  94 */
	95,							/* _  95 -> _  95 */
	96,							/* `  96 -> `  96 */
	97,							/* a  97 -> a  97 */
	98,							/* b  98 -> b  98 */
	99,							/* c  99 -> c  99 */
	100,						/* d 100 -> d 100 */
	101,						/* e 101 -> e 101 */
	102,						/* f 102 -> f 102 */
	103,						/* g 103 -> g 103 */
	104,						/* h 104 -> h 104 */
	105,						/* i 105 -> i 105 */
	106,						/* j 106 -> j 106 */
	107,						/* k 107 -> k 107 */
	108,						/* l 108 -> l 108 */
	109,						/* m 109 -> m 109 */
	110,						/* n 110 -> n 110 */
	111,						/* o 111 -> o 111 */
	112,						/* p 112 -> p 112 */
	113,						/* q 113 -> q 113 */
	114,						/* r 114 -> r 114 */
	115,						/* s 115 -> s 115 */
	116,						/* t 116 -> t 116 */
	117,						/* u 117 -> u 117 */
	118,						/* v 118 -> v 118 */
	119,						/* w 119 -> w 119 */
	120,						/* x 120 -> x 120 */
	121,						/* y 121 -> y 121 */
	122,						/* z 122 -> z 122 */
	123,						/* { 123 -> { 123 */
	124,						/* | 124 -> | 124 */
	125,						/* } 125 -> } 125 */
	126,						/* ~ 126 -> ~ 126 */
	127,						/*   127 ->   127 */
	128,						/*   128 ->   128 */
	129,						/*   129 ->   129 */
	130,						/*   130 ->   130 */
	131,						/*   131 ->   131 */
	132,						/*   132 ->   132 */
	133,						/*   133 ->   133 */
	134,						/*   134 ->   134 */
	135,						/*   135 ->   135 */
	136,						/*   136 ->   136 */
	137,						/*   137 ->   137 */
	139,						/*   138 ->   139 */
	139,						/*   139 ->   139 */
	140,						/*   140 ->   140 */
	161,						/*   141 -> ¡ 161 */
	142,						/*   142 ->   142 */
	160,						/*   143 ->   160 */
	130,						/*   144 ->   130 */
	145,						/*   145 ->   145 */
	146,						/*   146 ->   146 */
	147,						/*   147 ->   147 */
	148,						/*   148 ->   148 */
	162,						/*   149 -> ¢ 162 */
	150,						/*   150 ->   150 */
	163,						/*   151 -> £ 163 */
	150,						/*   152 ->   150 */
	148,						/*   153 ->   148 */
	129,						/*   154 ->   129 */
	155,						/*   155 ->   155 */
	156,						/*   156 ->   156 */
	157,						/*   157 ->   157 */
	158,						/*   158 ->   158 */
	159,						/*   159 ->   159 */
	160,						/*   160 ->   160 */
	161,						/* ¡ 161 -> ¡ 161 */
	162,						/* ¢ 162 -> ¢ 162 */
	163,						/* £ 163 -> £ 163 */
	164,						/* ¤ 164 -> ¤ 164 */
	165,						/* ¥ 165 -> ¥ 165 */
	166,						/* ¦ 166 -> ¦ 166 */
	147,						/* § 167 ->   147 */
	168,						/* ¨ 168 -> ¨ 168 */
	169,						/* © 169 -> © 169 */
	170,						/* ª 170 -> ª 170 */
	171,						/* « 171 -> « 171 */
	172,						/* ¬ 172 -> ¬ 172 */
	173,						/* ­ 173 -> ­ 173 */
	174,						/* ® 174 -> ® 174 */
	175,						/* ¯ 175 -> ¯ 175 */
	176,						/* ° 176 -> ° 176 */
	177,						/* ± 177 -> ± 177 */
	178,						/* ² 178 -> ² 178 */
	179,						/* ³ 179 -> ³ 179 */
	180,						/* ´ 180 -> ´ 180 */
	160,						/* µ 181 ->   160 */
	182,						/* ¶ 182 -> ¶ 182 */
	183,						/* · 183 -> · 183 */
	184,						/* ¸ 184 -> ¸ 184 */
	185,						/* ¹ 185 -> ¹ 185 */
	186,						/* º 186 -> º 186 */
	187,						/* » 187 -> » 187 */
	188,						/* ¼ 188 -> ¼ 188 */
	189,						/* ½ 189 -> ½ 189 */
	190,						/* ¾ 190 -> ¾ 190 */
	191,						/* ¿ 191 -> ¿ 191 */
	192,						/* À 192 -> À 192 */
	193,						/* Á 193 -> Á 193 */
	194,						/* Â 194 -> Â 194 */
	195,						/* Ã 195 -> Ã 195 */
	196,						/* Ä 196 -> Ä 196 */
	197,						/* Å 197 -> Å 197 */
	198,						/* Æ 198 -> Æ 198 */
	199,						/* Ç 199 -> Ç 199 */
	200,						/* È 200 -> È 200 */
	201,						/* É 201 -> É 201 */
	202,						/* Ê 202 -> Ê 202 */
	203,						/* Ë 203 -> Ë 203 */
	204,						/* Ì 204 -> Ì 204 */
	205,						/* Í 205 -> Í 205 */
	206,						/* Î 206 -> Î 206 */
	207,						/* Ï 207 -> Ï 207 */
	208,						/* Ð 208 -> Ð 208 */
	209,						/* Ñ 209 -> Ñ 209 */
	210,						/* Ò 210 -> Ò 210 */
	211,						/* Ó 211 -> Ó 211 */
	212,						/* Ô 212 -> Ô 212 */
	213,						/* Õ 213 -> Õ 213 */
	161,						/* Ö 214 -> ¡ 161 */
	215,						/* × 215 -> × 215 */
	216,						/* Ø 216 -> Ø 216 */
	217,						/* Ù 217 -> Ù 217 */
	218,						/* Ú 218 -> Ú 218 */
	219,						/* Û 219 -> Û 219 */
	220,						/* Ü 220 -> Ü 220 */
	221,						/* Ý 221 -> Ý 221 */
	222,						/* Þ 222 -> Þ 222 */
	223,						/* ß 223 -> ß 223 */
	162,						/* à 224 -> ¢ 162 */
	225,						/* á 225 -> á 225 */
	226,						/* â 226 -> â 226 */
	227,						/* ã 227 -> ã 227 */
	228,						/* ä 228 -> ä 228 */
	229,						/* å 229 -> å 229 */
	230,						/* æ 230 -> æ 230 */
	231,						/* ç 231 -> ç 231 */
	232,						/* è 232 -> è 232 */
	163,						/* é 233 -> £ 163 */
	234,						/* ê 234 -> ê 234 */
	251,						/* ë 235 -> û 251 */
	236,						/* ì 236 -> ì 236 */
	237,						/* í 237 -> í 237 */
	238,						/* î 238 -> î 238 */
	239,						/* ï 239 -> ï 239 */
	240,						/* ð 240 -> ð 240 */
	241,						/* ñ 241 -> ñ 241 */
	242,						/* ò 242 -> ò 242 */
	243,						/* ó 243 -> ó 243 */
	244,						/* ô 244 -> ô 244 */
	245,						/* õ 245 -> õ 245 */
	246,						/* ö 246 -> ö 246 */
	247,						/* ÷ 247 -> ÷ 247 */
	248,						/* ø 248 -> ø 248 */
	249,						/* ù 249 -> ù 249 */
	250,						/* ú 250 -> ú 250 */
	251,						/* û 251 -> û 251 */
	252,						/* ü 252 -> ü 252 */
	253,						/* ý 253 -> ý 253 */
	254,						/* þ 254 -> þ 254 */
	255							/*   255 ->   255 */
};

static const ExpandChar ExpansionTbl[NUM_EXPAND_CHARS + 1] = {
	{146, 65, 69},				/* ’ -> AE */
	{145, 97, 101},				/* ‘ -> ae */
	{225, 115, 115},			/* á -> ss */
	{0, 0, 0}					/* END OF TABLE */
};

static const CompressPair CompressTbl[NUM_COMPRESS_CHARS + 1] = {

		{{99, 115},
	 {FIRST_PRIMARY + 68, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 68, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0}},	/* cs */
	{{67, 115},
	 {FIRST_PRIMARY + 68, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 68, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0}},	/* Cs */
	{{67, 83}, {FIRST_PRIMARY + 68, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 68, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0}},	/* CS */
	{{100, 122},
	 {FIRST_PRIMARY + 70, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 70, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0}},	/* dz */
	{{68, 122},
	 {FIRST_PRIMARY + 70, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 70, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0}},	/* Dz */
	{{68, 90}, {FIRST_PRIMARY + 70, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 70, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0}},	/* DZ */
	{{103, 121},
	 {FIRST_PRIMARY + 74, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 74, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0}},	/* gy */
	{{71, 121},
	 {FIRST_PRIMARY + 74, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 74, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0}},	/* Gy */
	{{71, 89}, {FIRST_PRIMARY + 74, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 74, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0}},	/* GY */
	{{108, 121},
	 {FIRST_PRIMARY + 80, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 80, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0}},	/* ly */
	{{76, 121},
	 {FIRST_PRIMARY + 80, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 80, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0}},	/* Ly */
	{{76, 89}, {FIRST_PRIMARY + 80, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 80, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0}},	/* LY */
	{{110, 121},
	 {FIRST_PRIMARY + 83, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 83, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0}},	/* ny */
	{{78, 121},
	 {FIRST_PRIMARY + 83, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 83, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0}},	/* Ny */
	{{78, 89}, {FIRST_PRIMARY + 83, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 83, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0}},	/* NY */
	{{115, 122},
	 {FIRST_PRIMARY + 89, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 89, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0}},	/* sz */
	{{83, 122},
	 {FIRST_PRIMARY + 89, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 89, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0}},	/* Sz */
	{{83, 90}, {FIRST_PRIMARY + 89, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 89, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0}},	/* SZ */
	{{116, 121},
	 {FIRST_PRIMARY + 91, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 91, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0}},	/* ty */
	{{84, 121},
	 {FIRST_PRIMARY + 91, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 91, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0}},	/* Ty */
	{{84, 89}, {FIRST_PRIMARY + 91, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 91, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0}},	/* TY */
	{{122, 115},
	 {FIRST_PRIMARY + 98, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 98, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0}},	/* zs */
	{{90, 115},
	 {FIRST_PRIMARY + 98, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 98, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0}},	/* Zs */
	{{90, 83}, {FIRST_PRIMARY + 98, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},
	 {FIRST_PRIMARY + 98, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0}},	/* ZS */
	{{0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}	/*END OF TABLE */
};

static const SortOrderTblEntry NoCaseOrderTbl[NOCASESORT_LEN] = {
	{FIRST_PRIMARY + 0, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*   0   */
	{FIRST_PRIMARY + 1, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*   1   */
	{FIRST_PRIMARY + 2, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*   2   */
	{FIRST_PRIMARY + 3, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*   3   */
	{FIRST_PRIMARY + 4, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*   4   */
	{FIRST_PRIMARY + 5, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*   5   */
	{FIRST_PRIMARY + 6, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*   6   */
	{FIRST_PRIMARY + 7, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*   7   */
	{FIRST_PRIMARY + 8, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*   8   */
	{FIRST_PRIMARY + 9, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*   9   */
	{FIRST_PRIMARY + 10, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  10   */
	{FIRST_PRIMARY + 11, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  11   */
	{FIRST_PRIMARY + 12, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  12   */
	{FIRST_PRIMARY + 13, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  13   */
	{FIRST_PRIMARY + 14, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  14   */
	{FIRST_PRIMARY + 15, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  15   */
	{FIRST_PRIMARY + 16, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  16   */
	{FIRST_PRIMARY + 17, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  17   */
	{FIRST_PRIMARY + 18, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  18   */
	{FIRST_PRIMARY + 19, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  19   */
	{FIRST_PRIMARY + 20, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  20   */
	{FIRST_PRIMARY + 21, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  21   */
	{FIRST_PRIMARY + 22, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  22   */
	{FIRST_PRIMARY + 23, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  23   */
	{FIRST_PRIMARY + 24, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  24   */
	{FIRST_PRIMARY + 25, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  25   */
	{FIRST_PRIMARY + 26, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  26   */
	{FIRST_PRIMARY + 27, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  27   */
	{FIRST_PRIMARY + 28, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  28   */
	{FIRST_PRIMARY + 29, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  29   */
	{FIRST_PRIMARY + 30, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  30   */
	{FIRST_PRIMARY + 31, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  31   */
	{FIRST_PRIMARY + 32, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  32   */
	{FIRST_PRIMARY + 33, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  33 ! */
	{FIRST_PRIMARY + 34, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  34 " */
	{FIRST_PRIMARY + 35, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  35 # */
	{FIRST_PRIMARY + 36, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  36 $ */
	{FIRST_PRIMARY + 37, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  37 % */
	{FIRST_PRIMARY + 38, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  38 & */
	{FIRST_PRIMARY + 39, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  39 ' */
	{FIRST_PRIMARY + 40, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  40 ( */
	{FIRST_PRIMARY + 41, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  41 ) */
	{FIRST_PRIMARY + 42, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  42 * */
	{FIRST_PRIMARY + 43, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  43 + */
	{FIRST_PRIMARY + 44, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  44 , */
	{FIRST_PRIMARY + 45, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  45 - */
	{FIRST_PRIMARY + 46, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  46 . */
	{FIRST_PRIMARY + 47, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  47 / */
	{FIRST_PRIMARY + 48, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  48 0 */
	{FIRST_PRIMARY + 49, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  49 1 */
	{FIRST_PRIMARY + 50, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  50 2 */
	{FIRST_PRIMARY + 51, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  51 3 */
	{FIRST_PRIMARY + 52, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  52 4 */
	{FIRST_PRIMARY + 53, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  53 5 */
	{FIRST_PRIMARY + 54, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  54 6 */
	{FIRST_PRIMARY + 55, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  55 7 */
	{FIRST_PRIMARY + 56, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  56 8 */
	{FIRST_PRIMARY + 57, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  57 9 */
	{FIRST_PRIMARY + 58, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  58 : */
	{FIRST_PRIMARY + 59, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  59 ; */
	{FIRST_PRIMARY + 60, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  60 < */
	{FIRST_PRIMARY + 61, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  61 = */
	{FIRST_PRIMARY + 62, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  62 > */
	{FIRST_PRIMARY + 63, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  63 ? */
	{FIRST_PRIMARY + 64, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  64 @ */
	{FIRST_PRIMARY + 65, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  65 A */
	{FIRST_PRIMARY + 66, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  66 B */
	{FIRST_PRIMARY + 67, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 1},	/*  67 C */
	{FIRST_PRIMARY + 69, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 1},	/*  68 D */
	{FIRST_PRIMARY + 71, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  69 E */
	{FIRST_PRIMARY + 72, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  70 F */
	{FIRST_PRIMARY + 73, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 1},	/*  71 G */
	{FIRST_PRIMARY + 75, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  72 H */
	{FIRST_PRIMARY + 76, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  73 I */
	{FIRST_PRIMARY + 77, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  74 J */
	{FIRST_PRIMARY + 78, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  75 K */
	{FIRST_PRIMARY + 79, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 1},	/*  76 L */
	{FIRST_PRIMARY + 81, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  77 M */
	{FIRST_PRIMARY + 82, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 1},	/*  78 N */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  79 O */
	{FIRST_PRIMARY + 85, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  80 P */
	{FIRST_PRIMARY + 86, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  81 Q */
	{FIRST_PRIMARY + 87, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  82 R */
	{FIRST_PRIMARY + 88, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 1},	/*  83 S */
	{FIRST_PRIMARY + 90, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 1},	/*  84 T */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  85 U */
	{FIRST_PRIMARY + 93, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  86 V */
	{FIRST_PRIMARY + 94, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  87 W */
	{FIRST_PRIMARY + 95, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  88 X */
	{FIRST_PRIMARY + 96, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/*  89 Y */
	{FIRST_PRIMARY + 97, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 1},	/*  90 Z */
	{FIRST_PRIMARY + 99, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  91 [ */
	{FIRST_PRIMARY + 100, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  92 \ */
	{FIRST_PRIMARY + 101, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  93 ] */
	{FIRST_PRIMARY + 102, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  94 ^ */
	{FIRST_PRIMARY + 103, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  95 _ */
	{FIRST_PRIMARY + 104, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  96 ` */
	{FIRST_PRIMARY + 65, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/*  97 a */
	{FIRST_PRIMARY + 66, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/*  98 b */
	{FIRST_PRIMARY + 67, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 1},	/*  99 c */
	{FIRST_PRIMARY + 69, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 1},	/* 100 d */
	{FIRST_PRIMARY + 71, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 101 e */
	{FIRST_PRIMARY + 72, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 102 f */
	{FIRST_PRIMARY + 73, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 1},	/* 103 g */
	{FIRST_PRIMARY + 75, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 104 h */
	{FIRST_PRIMARY + 76, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 105 i */
	{FIRST_PRIMARY + 77, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 106 j */
	{FIRST_PRIMARY + 78, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 107 k */
	{FIRST_PRIMARY + 79, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 1},	/* 108 l */
	{FIRST_PRIMARY + 81, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 109 m */
	{FIRST_PRIMARY + 82, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 1},	/* 110 n */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 111 o */
	{FIRST_PRIMARY + 85, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 112 p */
	{FIRST_PRIMARY + 86, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 113 q */
	{FIRST_PRIMARY + 87, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 114 r */
	{FIRST_PRIMARY + 88, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 1},	/* 115 s */
	{FIRST_PRIMARY + 90, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 1},	/* 116 t */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 117 u */
	{FIRST_PRIMARY + 93, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 118 v */
	{FIRST_PRIMARY + 94, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 119 w */
	{FIRST_PRIMARY + 95, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 120 x */
	{FIRST_PRIMARY + 96, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/* 121 y */
	{FIRST_PRIMARY + 97, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 1},	/* 122 z */
	{FIRST_PRIMARY + 105, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 123 { */
	{FIRST_PRIMARY + 106, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 124 | */
	{FIRST_PRIMARY + 107, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 125 } */
	{FIRST_PRIMARY + 108, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 126 ~ */
	{FIRST_PRIMARY + 109, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 127   */
	{FIRST_PRIMARY + 110, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 128   */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 5, NULL_TERTIARY, 0, 0},	/* 129   */
	{FIRST_PRIMARY + 71, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},	/* 130   */
	{FIRST_PRIMARY + 111, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 131   */
	{FIRST_PRIMARY + 112, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 132   */
	{FIRST_PRIMARY + 113, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 133   */
	{FIRST_PRIMARY + 114, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 134   */
	{FIRST_PRIMARY + 115, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 135   */
	{FIRST_PRIMARY + 116, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 136   */
	{FIRST_PRIMARY + 117, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 137   */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 9, NULL_TERTIARY, 0, 0},	/* 138   */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 6, NULL_TERTIARY, 0, 0},	/* 139   */
	{FIRST_PRIMARY + 118, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 140   */
	{FIRST_PRIMARY + 76, FIRST_SECONDARY + 4, NULL_TERTIARY, 0, 0},	/* 141   */
	{FIRST_PRIMARY + 119, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 142   */
	{FIRST_PRIMARY + 65, FIRST_SECONDARY + 4, NULL_TERTIARY, 0, 0},	/* 143   */
	{FIRST_PRIMARY + 71, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0},	/* 144   */
	{FIRST_PRIMARY + 120, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 145   */
	{FIRST_PRIMARY + 121, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 146   */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 7, NULL_TERTIARY, 0, 0},	/* 147   */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 5, NULL_TERTIARY, 0, 0},	/* 148   */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 4, NULL_TERTIARY, 0, 0},	/* 149   */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 7, NULL_TERTIARY, 0, 0},	/* 150   */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 4, NULL_TERTIARY, 0, 0},	/* 151   */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 10, NULL_TERTIARY, 0, 0},	/* 152   */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 8, NULL_TERTIARY, 0, 0},	/* 153   */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 8, NULL_TERTIARY, 0, 0},	/* 154   */
	{FIRST_PRIMARY + 122, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 155   */
	{FIRST_PRIMARY + 123, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 156   */
	{FIRST_PRIMARY + 124, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 157   */
	{FIRST_PRIMARY + 125, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 158   */
	{FIRST_PRIMARY + 126, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 159   */
	{FIRST_PRIMARY + 65, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},	/* 160   */
	{FIRST_PRIMARY + 76, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},	/* 161 ¡ */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},	/* 162 ¢ */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},	/* 163 £ */
	{FIRST_PRIMARY + 127, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 164 ¤ */
	{FIRST_PRIMARY + 128, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 165 ¥ */
	{FIRST_PRIMARY + 129, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 166 ¦ */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 10, NULL_TERTIARY, 0, 0},	/* 167 § */
	{FIRST_PRIMARY + 130, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 168 ¨ */
	{FIRST_PRIMARY + 131, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 169 © */
	{FIRST_PRIMARY + 132, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 170 ª */
	{FIRST_PRIMARY + 133, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 171 « */
	{FIRST_PRIMARY + 134, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 172 ¬ */
	{FIRST_PRIMARY + 135, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 173 ­ */
	{FIRST_PRIMARY + 136, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 174 ® */
	{FIRST_PRIMARY + 137, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 175 ¯ */
	{FIRST_PRIMARY + 138, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 176 ° */
	{FIRST_PRIMARY + 139, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 177 ± */
	{FIRST_PRIMARY + 140, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 178 ² */
	{FIRST_PRIMARY + 141, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 179 ³ */
	{FIRST_PRIMARY + 142, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 180 ´ */
	{FIRST_PRIMARY + 65, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0},	/* 181 µ */
	{FIRST_PRIMARY + 143, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 182 ¶ */
	{FIRST_PRIMARY + 144, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 183 · */
	{FIRST_PRIMARY + 145, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 184 ¸ */
	{FIRST_PRIMARY + 146, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 185 ¹ */
	{FIRST_PRIMARY + 147, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 186 º */
	{FIRST_PRIMARY + 148, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 187 » */
	{FIRST_PRIMARY + 149, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 188 ¼ */
	{FIRST_PRIMARY + 150, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 189 ½ */
	{FIRST_PRIMARY + 151, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 190 ¾ */
	{FIRST_PRIMARY + 152, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 191 ¿ */
	{FIRST_PRIMARY + 153, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 192 À */
	{FIRST_PRIMARY + 154, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 193 Á */
	{FIRST_PRIMARY + 155, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 194 Â */
	{FIRST_PRIMARY + 156, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 195 Ã */
	{FIRST_PRIMARY + 157, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 196 Ä */
	{FIRST_PRIMARY + 158, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 197 Å */
	{FIRST_PRIMARY + 159, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 198 Æ */
	{FIRST_PRIMARY + 160, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 199 Ç */
	{FIRST_PRIMARY + 161, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 200 È */
	{FIRST_PRIMARY + 162, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 201 É */
	{FIRST_PRIMARY + 163, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 202 Ê */
	{FIRST_PRIMARY + 164, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 203 Ë */
	{FIRST_PRIMARY + 165, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 204 Ì */
	{FIRST_PRIMARY + 166, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 205 Í */
	{FIRST_PRIMARY + 167, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 206 Î */
	{FIRST_PRIMARY + 168, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 207 Ï */
	{FIRST_PRIMARY + 169, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 208 Ð */
	{FIRST_PRIMARY + 170, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 209 Ñ */
	{FIRST_PRIMARY + 171, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 210 Ò */
	{FIRST_PRIMARY + 172, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 211 Ó */
	{FIRST_PRIMARY + 173, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 212 Ô */
	{FIRST_PRIMARY + 174, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 213 Õ */
	{FIRST_PRIMARY + 76, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0},	/* 214 Ö */
	{FIRST_PRIMARY + 175, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 215 × */
	{FIRST_PRIMARY + 176, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 216 Ø */
	{FIRST_PRIMARY + 177, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 217 Ù */
	{FIRST_PRIMARY + 178, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 218 Ú */
	{FIRST_PRIMARY + 179, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 219 Û */
	{FIRST_PRIMARY + 180, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 220 Ü */
	{FIRST_PRIMARY + 181, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 221 Ý */
	{FIRST_PRIMARY + 182, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 222 Þ */
	{FIRST_PRIMARY + 183, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 223 ß */
	{FIRST_PRIMARY + 84, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0},	/* 224 à */
	{FIRST_PRIMARY + 184, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 225 á */
	{FIRST_PRIMARY + 185, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 226 â */
	{FIRST_PRIMARY + 186, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 227 ã */
	{FIRST_PRIMARY + 187, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 228 ä */
	{FIRST_PRIMARY + 188, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 229 å */
	{FIRST_PRIMARY + 189, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 230 æ */
	{FIRST_PRIMARY + 190, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 231 ç */
	{FIRST_PRIMARY + 191, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 232 è */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0},	/* 233 é */
	{FIRST_PRIMARY + 192, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 234 ê */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 9, NULL_TERTIARY, 0, 0},	/* 235 ë */
	{FIRST_PRIMARY + 193, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 236 ì */
	{FIRST_PRIMARY + 194, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 237 í */
	{FIRST_PRIMARY + 195, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 238 î */
	{FIRST_PRIMARY + 196, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 239 ï */
	{FIRST_PRIMARY + 197, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 240 ð */
	{FIRST_PRIMARY + 198, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 241 ñ */
	{FIRST_PRIMARY + 199, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 242 ò */
	{FIRST_PRIMARY + 200, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 243 ó */
	{FIRST_PRIMARY + 201, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 244 ô */
	{FIRST_PRIMARY + 202, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 245 õ */
	{FIRST_PRIMARY + 203, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 246 ö */
	{FIRST_PRIMARY + 204, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 247 ÷ */
	{FIRST_PRIMARY + 205, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 248 ø */
	{FIRST_PRIMARY + 206, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 249 ù */
	{FIRST_PRIMARY + 207, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 250 ú */
	{FIRST_PRIMARY + 92, FIRST_SECONDARY + 6, NULL_TERTIARY, 0, 0},	/* 251 û */
	{FIRST_PRIMARY + 208, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 252 ü */
	{FIRST_PRIMARY + 209, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 253 ý */
	{FIRST_PRIMARY + 210, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 254 þ */
	{FIRST_PRIMARY + 211, NULL_SECONDARY, NULL_TERTIARY, 0, 0}	/* 255   */
};

/* End of File : Language driver hun852dc */
