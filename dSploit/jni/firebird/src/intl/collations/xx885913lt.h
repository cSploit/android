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
 * Contributor: Jonas Jasas
 */

const int NUM_EXPAND_CHARS		= 0;
const int NUM_COMPRESS_CHARS	= 0;
const int LOWERCASE_LEN			= 256;
const int UPPERCASE_LEN			= 256;
const int NOCASESORT_LEN		= 256;
const int LDRV_TIEBREAK			= SECONDARY;

//const int MAX_NCO_PRIMARY		= 229;
const int MAX_NCO_SECONDARY		= 0;
const int MAX_NCO_TERTIARY		= 1;
//const int MAX_NCO_IGNORE		= 0;
const int NULL_SECONDARY		= 0;
const int NULL_TERTIARY			= 0;
const int FIRST_IGNORE			= 1;
const int FIRST_TERTIARY		= 1;
const int FIRST_SECONDARY		= (FIRST_TERTIARY + MAX_NCO_TERTIARY + 1);
const int FIRST_PRIMARY			= (FIRST_SECONDARY + MAX_NCO_SECONDARY + 1);

static const BYTE ToUpperConversionTbl[UPPERCASE_LEN] = {
	0,	/*     0 ->     0 */
	1,	/*     1 ->     1 */
	2,	/*     2 ->     2 */
	3,	/*     3 ->     3 */
	4,	/*     4 ->     4 */
	5,	/*     5 ->     5 */
	6,	/*     6 ->     6 */
	7,	/*     7 ->     7 */
	8,	/*     8 ->     8 */
	9,	/*     9 ->     9 */
	10,	/*    10 ->    10 */
	11,	/*    11 ->    11 */
	12,	/*    12 ->    12 */
	13,	/*    13 ->    13 */
	14,	/*    14 ->    14 */
	15,	/*    15 ->    15 */
	16,	/*    16 ->    16 */
	17,	/*    17 ->    17 */
	18,	/*    18 ->    18 */
	19,	/*    19 ->    19 */
	20,	/*    20 ->    20 */
	21,	/*    21 ->    21 */
	22,	/*    22 ->    22 */
	23,	/*    23 ->    23 */
	24,	/*    24 ->    24 */
	25,	/*    25 ->    25 */
	26,	/*    26 ->    26 */
	27,	/*    27 ->    27 */
	28,	/*    28 ->    28 */
	29,	/*    29 ->    29 */
	30,	/*    30 ->    30 */
	31,	/*    31 ->    31 */
	32,	/*    32 ->    32 */
	33,	/* !  33 -> !  33 */
	34,	/* "  34 -> "  34 */
	35,	/* #  35 -> #  35 */
	36,	/* $  36 -> $  36 */
	37,	/* %  37 -> %  37 */
	38,	/* &  38 -> &  38 */
	39,	/* '  39 -> '  39 */
	40,	/* (  40 -> (  40 */
	41,	/* )  41 -> )  41 */
	42,	/* *  42 -> *  42 */
	43,	/* +  43 -> +  43 */
	44,	/* ,  44 -> ,  44 */
	45,	/* -  45 -> -  45 */
	46,	/* .  46 -> .  46 */
	47,	/* /  47 -> /  47 */
	48,	/* 0  48 -> 0  48 */
	49,	/* 1  49 -> 1  49 */
	50,	/* 2  50 -> 2  50 */
	51,	/* 3  51 -> 3  51 */
	52,	/* 4  52 -> 4  52 */
	53,	/* 5  53 -> 5  53 */
	54,	/* 6  54 -> 6  54 */
	55,	/* 7  55 -> 7  55 */
	56,	/* 8  56 -> 8  56 */
	57,	/* 9  57 -> 9  57 */
	58,	/* :  58 -> :  58 */
	59,	/* ;  59 -> ;  59 */
	60,	/* <  60 -> <  60 */
	61,	/* =  61 -> =  61 */
	62,	/* >  62 -> >  62 */
	63,	/* ?  63 -> ?  63 */
	64,	/* @  64 -> @  64 */
	65,	/* A  65 -> A  65 */
	66,	/* B  66 -> B  66 */
	67,	/* C  67 -> C  67 */
	68,	/* D  68 -> D  68 */
	69,	/* E  69 -> E  69 */
	70,	/* F  70 -> F  70 */
	71,	/* G  71 -> G  71 */
	72,	/* H  72 -> H  72 */
	73,	/* I  73 -> I  73 */
	74,	/* J  74 -> J  74 */
	75,	/* K  75 -> K  75 */
	76,	/* L  76 -> L  76 */
	77,	/* M  77 -> M  77 */
	78,	/* N  78 -> N  78 */
	79,	/* O  79 -> O  79 */
	80,	/* P  80 -> P  80 */
	81,	/* Q  81 -> Q  81 */
	82,	/* R  82 -> R  82 */
	83,	/* S  83 -> S  83 */
	84,	/* T  84 -> T  84 */
	85,	/* U  85 -> U  85 */
	86,	/* V  86 -> V  86 */
	87,	/* W  87 -> W  87 */
	88,	/* X  88 -> X  88 */
	89,	/* Y  89 -> Y  89 */
	90,	/* Z  90 -> Z  90 */
	91,	/* [  91 -> [  91 */
	92,	/* \  92 -> \  92 */
	93,	/* ]  93 -> ]  93 */
	94,	/* ^  94 -> ^  94 */
	95,	/* _  95 -> _  95 */
	96,	/* `  96 -> `  96 */
	65,	/* a  97 -> A  65 */
	66,	/* b  98 -> B  66 */
	67,	/* c  99 -> C  67 */
	68,	/* d 100 -> D  68 */
	69,	/* e 101 -> E  69 */
	70,	/* f 102 -> F  70 */
	71,	/* g 103 -> G  71 */
	72,	/* h 104 -> H  72 */
	73,	/* i 105 -> I  73 */
	74,	/* j 106 -> J  74 */
	75,	/* k 107 -> K  75 */
	76,	/* l 108 -> L  76 */
	77,	/* m 109 -> M  77 */
	78,	/* n 110 -> N  78 */
	79,	/* o 111 -> O  79 */
	80,	/* p 112 -> P  80 */
	81,	/* q 113 -> Q  81 */
	82,	/* r 114 -> R  82 */
	83,	/* s 115 -> S  83 */
	84,	/* t 116 -> T  84 */
	85,	/* u 117 -> U  85 */
	86,	/* v 118 -> V  86 */
	87,	/* w 119 -> W  87 */
	88,	/* x 120 -> X  88 */
	89,	/* y 121 -> Y  89 */
	90,	/* z 122 -> Z  90 */
	123,	/* { 123 -> { 123 */
	124,	/* | 124 -> | 124 */
	125,	/* } 125 -> } 125 */
	126,	/* ~ 126 -> ~ 126 */
	127,	/*   127 ->   127 */
	128,	/*   128 ->   128 */
	129,	/*   129 ->   129 */
	130,	/*   130 ->   130 */
	131,	/*   131 ->   131 */
	132,	/*   132 ->   132 */
	133,	/*   133 ->   133 */
	134,	/*   134 ->   134 */
	135,	/*   135 ->   135 */
	136,	/*   136 ->   136 */
	137,	/*   137 ->   137 */
	138,	/*   138 ->   138 */
	139,	/*   139 ->   139 */
	140,	/*   140 ->   140 */
	141,	/*   141 ->   141 */
	142,	/*   142 ->   142 */
	143,	/*   143 ->   143 */
	144,	/*   144 ->   144 */
	145,	/*   145 ->   145 */
	146,	/*   146 ->   146 */
	147,	/*   147 ->   147 */
	148,	/*   148 ->   148 */
	149,	/*   149 ->   149 */
	150,	/*   150 ->   150 */
	151,	/*   151 ->   151 */
	152,	/*   152 ->   152 */
	153,	/*   153 ->   153 */
	154,	/*   154 ->   154 */
	155,	/*   155 ->   155 */
	156,	/*   156 ->   156 */
	157,	/*   157 ->   157 */
	158,	/*   158 ->   158 */
	159,	/*   159 ->   159 */
	160,	/*   160 ->   160 */
	161,	/* ¡ 161 -> ¡ 161 */
	162,	/* ¢ 162 -> ¢ 162 */
	163,	/* £ 163 -> £ 163 */
	164,	/* ¤ 164 -> ¤ 164 */
	165,	/* ¥ 165 -> ¥ 165 */
	166,	/* ¦ 166 -> ¦ 166 */
	167,	/* § 167 -> § 167 */
	168,	/* ¨ 168 -> ¨ 168 */
	169,	/* © 169 -> © 169 */
	170,	/* ª 170 -> ª 170 */
	171,	/* « 171 -> « 171 */
	172,	/* ¬ 172 -> ¬ 172 */
	173,	/* ­ 173 -> ­ 173 */
	174,	/* ® 174 -> ® 174 */
	175,	/* ¯ 175 -> ¯ 175 */
	176,	/* ° 176 -> ° 176 */
	177,	/* ± 177 -> ± 177 */
	178,	/* ² 178 -> ² 178 */
	179,	/* ³ 179 -> ³ 179 */
	180,	/* ´ 180 -> ´ 180 */
	181,	/* µ 181 -> µ 181 */
	182,	/* ¶ 182 -> ¶ 182 */
	183,	/* · 183 -> · 183 */
	184,	/* ¸ 184 -> ¸ 184 */
	185,	/* ¹ 185 -> ¹ 185 */
	186,	/* º 186 -> º 186 */
	187,	/* » 187 -> » 187 */
	188,	/* ¼ 188 -> ¼ 188 */
	189,	/* ½ 189 -> ½ 189 */
	190,	/* ¾ 190 -> ¾ 190 */
	191,	/* ¿ 191 -> ¿ 191 */
	192,	/* À 192 -> À 192 */
	193,	/* Á 193 -> Á 193 */
	194,	/* Â 194 -> Â 194 */
	195,	/* Ã 195 -> Ã 195 */
	196,	/* Ä 196 -> Ä 196 */
	197,	/* Å 197 -> Å 197 */
	198,	/* Æ 198 -> Æ 198 */
	199,	/* Ç 199 -> Ç 199 */
	200,	/* È 200 -> È 200 */
	201,	/* É 201 -> É 201 */
	202,	/* Ê 202 -> Ê 202 */
	203,	/* Ë 203 -> Ë 203 */
	204,	/* Ì 204 -> Ì 204 */
	205,	/* Í 205 -> Í 205 */
	206,	/* Î 206 -> Î 206 */
	207,	/* Ï 207 -> Ï 207 */
	208,	/* Ð 208 -> Ð 208 */
	209,	/* Ñ 209 -> Ñ 209 */
	210,	/* Ò 210 -> Ò 210 */
	211,	/* Ó 211 -> Ó 211 */
	212,	/* Ô 212 -> Ô 212 */
	213,	/* Õ 213 -> Õ 213 */
	214,	/* Ö 214 -> Ö 214 */
	215,	/* × 215 -> × 215 */
	216,	/* Ø 216 -> Ø 216 */
	217,	/* Ù 217 -> Ù 217 */
	218,	/* Ú 218 -> Ú 218 */
	219,	/* Û 219 -> Û 219 */
	220,	/* Ü 220 -> Ü 220 */
	221,	/* Ý 221 -> Ý 221 */
	222,	/* Þ 222 -> Þ 222 */
	223,	/* ß 223 -> ß 223 */
	192,	/* à 224 -> À 192 *LT*/
	193,	/* á 225 -> Á 193 *LT*/
	226,	/* â 226 -> â 226 */
	227,	/* ã 227 -> ã 227 */
	228,	/* ä 228 -> ä 228 */
	229,	/* å 229 -> å 229 */
	198,	/* æ 230 -> Æ 198 *LT*/
	231,	/* ç 231 -> ç 231 */
	200,	/* è 232 -> È 200 *LT*/
	233,	/* é 233 -> é 233 */
	234,	/* ê 234 -> ê 234 */
	203,	/* ë 235 -> Ë 203 *LT*/
	236,	/* ì 236 -> ì 236 */
	237,	/* í 237 -> í 237 */
	238,	/* î 238 -> î 238 */
	239,	/* ï 239 -> ï 239 */
	208,	/* ð 240 -> Ð 208 *LT*/
	241,	/* ñ 241 -> ñ 241 */
	242,	/* ò 242 -> ò 242 */
	243,	/* ó 243 -> ó 243 */
	244,	/* ô 244 -> ô 244 */
	245,	/* õ 245 -> õ 245 */
	246,	/* ö 246 -> ö 246 */
	247,	/* ÷ 247 -> ÷ 247 */
	216,	/* ø 248 -> Ø 216 *LT*/
	249,	/* ù 249 -> ù 249 */
	250,	/* ú 250 -> ú 250 */
	219,	/* û 251 -> Û 219 *LT*/
	252,	/* ü 252 -> ü 252 */
	253,	/* ý 253 -> ý 253 */
	222,	/* þ 254 -> Þ 222 *LT*/
	255	/*   255 ->   255 */
};

static const BYTE ToLowerConversionTbl[LOWERCASE_LEN] = {
	0,	/*     0 ->     0 */
	1,	/*     1 ->     1 */
	2,	/*     2 ->     2 */
	3,	/*     3 ->     3 */
	4,	/*     4 ->     4 */
	5,	/*     5 ->     5 */
	6,	/*     6 ->     6 */
	7,	/*     7 ->     7 */
	8,	/*     8 ->     8 */
	9,	/*     9 ->     9 */
	10,	/*    10 ->    10 */
	11,	/*    11 ->    11 */
	12,	/*    12 ->    12 */
	13,	/*    13 ->    13 */
	14,	/*    14 ->    14 */
	15,	/*    15 ->    15 */
	16,	/*    16 ->    16 */
	17,	/*    17 ->    17 */
	18,	/*    18 ->    18 */
	19,	/*    19 ->    19 */
	20,	/*    20 ->    20 */
	21,	/*    21 ->    21 */
	22,	/*    22 ->    22 */
	23,	/*    23 ->    23 */
	24,	/*    24 ->    24 */
	25,	/*    25 ->    25 */
	26,	/*    26 ->    26 */
	27,	/*    27 ->    27 */
	28,	/*    28 ->    28 */
	29,	/*    29 ->    29 */
	30,	/*    30 ->    30 */
	31,	/*    31 ->    31 */
	32,	/*    32 ->    32 */
	33,	/* !  33 -> !  33 */
	34,	/* "  34 -> "  34 */
	35,	/* #  35 -> #  35 */
	36,	/* $  36 -> $  36 */
	37,	/* %  37 -> %  37 */
	38,	/* &  38 -> &  38 */
	39,	/* '  39 -> '  39 */
	40,	/* (  40 -> (  40 */
	41,	/* )  41 -> )  41 */
	42,	/* *  42 -> *  42 */
	43,	/* +  43 -> +  43 */
	44,	/* ,  44 -> ,  44 */
	45,	/* -  45 -> -  45 */
	46,	/* .  46 -> .  46 */
	47,	/* /  47 -> /  47 */
	48,	/* 0  48 -> 0  48 */
	49,	/* 1  49 -> 1  49 */
	50,	/* 2  50 -> 2  50 */
	51,	/* 3  51 -> 3  51 */
	52,	/* 4  52 -> 4  52 */
	53,	/* 5  53 -> 5  53 */
	54,	/* 6  54 -> 6  54 */
	55,	/* 7  55 -> 7  55 */
	56,	/* 8  56 -> 8  56 */
	57,	/* 9  57 -> 9  57 */
	58,	/* :  58 -> :  58 */
	59,	/* ;  59 -> ;  59 */
	60,	/* <  60 -> <  60 */
	61,	/* =  61 -> =  61 */
	62,	/* >  62 -> >  62 */
	63,	/* ?  63 -> ?  63 */
	64,	/* @  64 -> @  64 */
	97,	/* A  65 -> a  97 */
	98,	/* B  66 -> b  98 */
	99,	/* C  67 -> c  99 */
	100,	/* D  68 -> d 100 */
	101,	/* E  69 -> e 101 */
	102,	/* F  70 -> f 102 */
	103,	/* G  71 -> g 103 */
	104,	/* H  72 -> h 104 */
	105,	/* I  73 -> i 105 */
	106,	/* J  74 -> j 106 */
	107,	/* K  75 -> k 107 */
	108,	/* L  76 -> l 108 */
	109,	/* M  77 -> m 109 */
	110,	/* N  78 -> n 110 */
	111,	/* O  79 -> o 111 */
	112,	/* P  80 -> p 112 */
	113,	/* Q  81 -> q 113 */
	114,	/* R  82 -> r 114 */
	115,	/* S  83 -> s 115 */
	116,	/* T  84 -> t 116 */
	117,	/* U  85 -> u 117 */
	118,	/* V  86 -> v 118 */
	119,	/* W  87 -> w 119 */
	120,	/* X  88 -> x 120 */
	121,	/* Y  89 -> y 121 */
	122,	/* Z  90 -> z 122 */
	91,	/* [  91 -> [  91 */
	92,	/* \  92 -> \  92 */
	93,	/* ]  93 -> ]  93 */
	94,	/* ^  94 -> ^  94 */
	95,	/* _  95 -> _  95 */
	96,	/* `  96 -> `  96 */
	97,	/* a  97 -> a  97 */
	98,	/* b  98 -> b  98 */
	99,	/* c  99 -> c  99 */
	100,	/* d 100 -> d 100 */
	101,	/* e 101 -> e 101 */
	102,	/* f 102 -> f 102 */
	103,	/* g 103 -> g 103 */
	104,	/* h 104 -> h 104 */
	105,	/* i 105 -> i 105 */
	106,	/* j 106 -> j 106 */
	107,	/* k 107 -> k 107 */
	108,	/* l 108 -> l 108 */
	109,	/* m 109 -> m 109 */
	110,	/* n 110 -> n 110 */
	111,	/* o 111 -> o 111 */
	112,	/* p 112 -> p 112 */
	113,	/* q 113 -> q 113 */
	114,	/* r 114 -> r 114 */
	115,	/* s 115 -> s 115 */
	116,	/* t 116 -> t 116 */
	117,	/* u 117 -> u 117 */
	118,	/* v 118 -> v 118 */
	119,	/* w 119 -> w 119 */
	120,	/* x 120 -> x 120 */
	121,	/* y 121 -> y 121 */
	122,	/* z 122 -> z 122 */
	123,	/* { 123 -> { 123 */
	124,	/* | 124 -> | 124 */
	125,	/* } 125 -> } 125 */
	126,	/* ~ 126 -> ~ 126 */
	127,	/*   127 ->   127 */
	128,	/*   128 ->   128 */
	129,	/*   129 ->   129 */
	130,	/*   130 ->   130 */
	131,	/*   131 ->   131 */
	132,	/*   132 ->   132 */
	133,	/*   133 ->   133 */
	134,	/*   134 ->   134 */
	135,	/*   135 ->   135 */
	136,	/*   136 ->   136 */
	137,	/*   137 ->   137 */
	138,	/*   138 ->   138 */
	139,	/*   139 ->   139 */
	140,	/*   140 ->   140 */
	141,	/*   141 ->   141 */
	142,	/*   142 ->   142 */
	143,	/*   143 ->   143 */
	144,	/*   144 ->   144 */
	145,	/*   145 ->   145 */
	146,	/*   146 ->   146 */
	147,	/*   147 ->   147 */
	148,	/*   148 ->   148 */
	149,	/*   149 ->   149 */
	150,	/*   150 ->   150 */
	151,	/*   151 ->   151 */
	152,	/*   152 ->   152 */
	153,	/*   153 ->   153 */
	154,	/*   154 ->   154 */
	155,	/*   155 ->   155 */
	156,	/*   156 ->   156 */
	157,	/*   157 ->   157 */
	158,	/*   158 ->   158 */
	159,	/*   159 ->   159 */
	160,	/*   160 ->   160 */
	161,	/* ¡ 161 -> ¡ 161 */
	162,	/* ¢ 162 -> ¢ 162 */
	163,	/* £ 163 -> £ 163 */
	164,	/* ¤ 164 -> ¤ 164 */
	165,	/* ¥ 165 -> ¥ 165 */
	166,	/* ¦ 166 -> ¦ 166 */
	167,	/* § 167 -> § 167 */
	168,	/* ¨ 168 -> ¨ 168 */
	169,	/* © 169 -> © 169 */
	170,	/* ª 170 -> ª 170 */
	171,	/* « 171 -> « 171 */
	172,	/* ¬ 172 -> ¬ 172 */
	173,	/* ­ 173 -> ­ 173 */
	174,	/* ® 174 -> ® 174 */
	175,	/* ¯ 175 -> ¯ 175 */
	176,	/* ° 176 -> ° 176 */
	177,	/* ± 177 -> ± 177 */
	178,	/* ² 178 -> ² 178 */
	179,	/* ³ 179 -> ³ 179 */
	180,	/* ´ 180 -> ´ 180 */
	181,	/* µ 181 -> µ 181 */
	182,	/* ¶ 182 -> ¶ 182 */
	183,	/* · 183 -> · 183 */
	184,	/* ¸ 184 -> ¸ 184 */
	185,	/* ¹ 185 -> ¹ 185 */
	186,	/* º 186 -> º 186 */
	187,	/* » 187 -> » 187 */
	188,	/* ¼ 188 -> ¼ 188 */
	189,	/* ½ 189 -> ½ 189 */
	190,	/* ¾ 190 -> ¾ 190 */
	191,	/* ¿ 191 -> ¿ 191 */
	192,	/* À 192 -> à 224 *LT*/
	193,	/* Á 193 -> á 225 *LT*/
	194,	/* Â 194 -> Â 194 */
	195,	/* Ã 195 -> Ã 195 */
	196,	/* Ä 196 -> Ä 196 */
	197,	/* Å 197 -> Å 197 */
	198,	/* Æ 198 -> æ 230 *LT*/
	199,	/* Ç 199 -> Ç 199 */
	200,	/* È 200 -> è 232 *LT*/
	201,	/* É 201 -> É 201 */
	202,	/* Ê 202 -> Ê 202 */
	203,	/* Ë 203 -> ë 235 *LT*/
	204,	/* Ì 204 -> Ì 204 */
	205,	/* Í 205 -> Í 205 */
	206,	/* Î 206 -> Î 206 */
	207,	/* Ï 207 -> Ï 207 */
	208,	/* Ð 208 -> ð 240 *LT*/
	209,	/* Ñ 209 -> Ñ 209 */
	210,	/* Ò 210 -> Ò 210 */
	211,	/* Ó 211 -> Ó 211 */
	212,	/* Ô 212 -> Ô 212 */
	213,	/* Õ 213 -> Õ 213 */
	214,	/* Ö 214 -> Ö 214 */
	215,	/* × 215 -> × 215 */
	216,	/* Ø 216 -> ø 248 *LT*/
	217,	/* Ù 217 -> Ù 217 */
	218,	/* Ú 218 -> Ú 218 */
	219,	/* Û 219 -> û 251 *LT*/
	220,	/* Ü 220 -> Ü 220 */
	221,	/* Ý 221 -> Ý 221 */
	222,	/* Þ 222 -> þ 254 *LT*/
	223,	/* ß 223 -> ß 223 */
	224,	/* à 224 -> à 224 */
	225,	/* á 225 -> á 225 */
	226,	/* â 226 -> â 226 */
	227,	/* ã 227 -> ã 227 */
	228,	/* ä 228 -> ä 228 */
	229,	/* å 229 -> å 229 */
	230,	/* æ 230 -> æ 230 */
	231,	/* ç 231 -> ç 231 */
	232,	/* è 232 -> è 232 */
	233,	/* é 233 -> é 233 */
	234,	/* ê 234 -> ê 234 */
	235,	/* ë 235 -> ë 235 */
	236,	/* ì 236 -> ì 236 */
	237,	/* í 237 -> í 237 */
	238,	/* î 238 -> î 238 */
	239,	/* ï 239 -> ï 239 */
	240,	/* ð 240 -> ð 240 */
	241,	/* ñ 241 -> ñ 241 */
	242,	/* ò 242 -> ò 242 */
	243,	/* ó 243 -> ó 243 */
	244,	/* ô 244 -> ô 244 */
	245,	/* õ 245 -> õ 245 */
	246,	/* ö 246 -> ö 246 */
	247,	/* ÷ 247 -> ÷ 247 */
	248,	/* ø 248 -> ø 248 */
	249,	/* ù 249 -> ù 249 */
	250,	/* ú 250 -> ú 250 */
	251,	/* û 251 -> û 251 */
	252,	/* ü 252 -> ü 252 */
	253,	/* ý 253 -> ý 253 */
	254,	/* þ 254 -> þ 254 */
	255	/*   255 ->   255 */
};

static const struct ExpandChar ExpansionTbl[NUM_EXPAND_CHARS + 1] = {
	{0, 0, 0}					/* END OF TABLE */
};

static const struct CompressPair CompressTbl [ NUM_COMPRESS_CHARS + 1 ] = {
{ {0, 0}, {   0,   0,   0,   0,   0 }, {   0,   0,   0,   0,   0 } } /*END OF TABLE */
};

static const struct SortOrderTblEntry NoCaseOrderTbl[NOCASESORT_LEN] = {
{FIRST_IGNORE + 0, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 00 : U+0000 */
{FIRST_IGNORE + 1, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 01 : U+0001 */
{FIRST_IGNORE + 2, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 02 : U+0002 */
{FIRST_IGNORE + 3, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 03 : U+0003 */
{FIRST_IGNORE + 4, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 04 : U+0004 */
{FIRST_IGNORE + 5, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 05 : U+0005 */
{FIRST_IGNORE + 6, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 06 : U+0006 */
{FIRST_IGNORE + 7, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 07 : U+0007 */
{FIRST_IGNORE + 8, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 08 : U+0008 */
{FIRST_IGNORE + 9, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 09 : U+0009 */
{FIRST_IGNORE + 10, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 0a : U+000a */
{FIRST_IGNORE + 11, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 0b : U+000b */
{FIRST_IGNORE + 12, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 0c : U+000c */
{FIRST_IGNORE + 13, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 0d : U+000d */
{FIRST_IGNORE + 14, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 0e : U+000e */
{FIRST_IGNORE + 15, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 0f : U+000f */
{FIRST_IGNORE + 16, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 10 : U+0010 */
{FIRST_IGNORE + 17, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 11 : U+0011 */
{FIRST_IGNORE + 18, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 12 : U+0012 */
{FIRST_IGNORE + 19, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 13 : U+0013 */
{FIRST_IGNORE + 20, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 14 : U+0014 */
{FIRST_IGNORE + 21, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 15 : U+0015 */
{FIRST_IGNORE + 22, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 16 : U+0016 */
{FIRST_IGNORE + 23, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 17 : U+0017 */
{FIRST_IGNORE + 24, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 18 : U+0018 */
{FIRST_IGNORE + 25, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 19 : U+0019 */
{FIRST_IGNORE + 26, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 1a : U+001a */
{FIRST_IGNORE + 27, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 1b : U+001b */
{FIRST_IGNORE + 28, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 1c : U+001c */
{FIRST_IGNORE + 29, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 1d : U+001d */
{FIRST_IGNORE + 30, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 1e : U+001e */
{FIRST_IGNORE + 31, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 1f : U+001f */
{FIRST_PRIMARY + 1, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x20   : U+0020   */
{FIRST_PRIMARY + 3, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x21 ! : U+0021 ! */
{FIRST_PRIMARY + 4, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x22 " : U+0022 " */
{FIRST_PRIMARY + 5, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x23 # : U+0023 # */
{FIRST_PRIMARY + 6, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x24 $ : U+0024 $ */
{FIRST_PRIMARY + 7, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x25 % : U+0025 % */
{FIRST_PRIMARY + 8, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x26 & : U+0026 & */
{FIRST_PRIMARY + 0, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x27 ' : U+0027 ' */
{FIRST_PRIMARY + 9, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x28 ( : U+0028 ( */
{FIRST_PRIMARY + 10, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x29 ) : U+0029 ) */
{FIRST_PRIMARY + 11, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x2a * : U+002a * */
{FIRST_PRIMARY + 34, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x2b + : U+002b + */
{FIRST_PRIMARY + 12, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x2c , : U+002c , */
{FIRST_PRIMARY + 0, NULL_SECONDARY, FIRST_TERTIARY + 2, 0, 0},		/* 0x2d - : U+002d - */
{FIRST_PRIMARY + 13, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x2e . : U+002e . */
{FIRST_PRIMARY + 14, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x2f / : U+002f / */
{FIRST_PRIMARY + 54, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x30 0 : U+0030 0 */
{FIRST_PRIMARY + 58, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x31 1 : U+0031 1 */
{FIRST_PRIMARY + 59, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x32 2 : U+0032 2 */
{FIRST_PRIMARY + 60, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x33 3 : U+0033 3 */
{FIRST_PRIMARY + 61, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x34 4 : U+0034 4 */
{FIRST_PRIMARY + 62, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x35 5 : U+0035 5 */
{FIRST_PRIMARY + 63, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x36 6 : U+0036 6 */
{FIRST_PRIMARY + 64, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x37 7 : U+0037 7 */
{FIRST_PRIMARY + 65, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x38 8 : U+0038 8 */
{FIRST_PRIMARY + 66, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x39 9 : U+0039 9 */
{FIRST_PRIMARY + 15, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x3a : : U+003a : */
{FIRST_PRIMARY + 16, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x3b ; : U+003b ; */
{FIRST_PRIMARY + 35, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x3c < : U+003c < */
{FIRST_PRIMARY + 36, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x3d = : U+003d = */
{FIRST_PRIMARY + 37, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x3e > : U+003e > */
{FIRST_PRIMARY + 17, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x3f ? : U+003f ? */
{FIRST_PRIMARY + 18, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x40 @ : U+0040 @ */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x41 A : U+0041 A */
{FIRST_PRIMARY + 68, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x42 B : U+0042 B */
{FIRST_PRIMARY + 69, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x43 C : U+0043 C */
{FIRST_PRIMARY + 71, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x44 D : U+0044 D */
{FIRST_PRIMARY + 72, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x45 E : U+0045 E */
{FIRST_PRIMARY + 73, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x46 F : U+0046 F */
{FIRST_PRIMARY + 74, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x47 G : U+0047 G */
{FIRST_PRIMARY + 75, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x48 H : U+0048 H */
{FIRST_PRIMARY + 76, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x49 I : U+0049 I */
{FIRST_PRIMARY + 78, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x4a J : U+004a J */
{FIRST_PRIMARY + 79, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x4b K : U+004b K */
{FIRST_PRIMARY + 80, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x4c L : U+004c L */
{FIRST_PRIMARY + 81, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x4d M : U+004d M */
{FIRST_PRIMARY + 82, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x4e N : U+004e N */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x4f O : U+004f O */
{FIRST_PRIMARY + 84, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x50 P : U+0050 P */
{FIRST_PRIMARY + 85, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x51 Q : U+0051 Q */
{FIRST_PRIMARY + 86, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x52 R : U+0052 R */
{FIRST_PRIMARY + 87, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x53 S : U+0053 S */
{FIRST_PRIMARY + 89, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x54 T : U+0054 T */
{FIRST_PRIMARY + 90, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x55 U : U+0055 U */
{FIRST_PRIMARY + 91, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x56 V : U+0056 V */
{FIRST_PRIMARY + 92, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x57 W : U+0057 W */
{FIRST_PRIMARY + 93, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x58 X : U+0058 X */
{FIRST_PRIMARY + 76, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 0x59 Y : U+0059 Y */
{FIRST_PRIMARY + 94, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x5a Z : U+005a Z */
{FIRST_PRIMARY + 19, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x5b [ : U+005b [ */
{FIRST_PRIMARY + 20, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x5c \ : U+005c \ */
{FIRST_PRIMARY + 21, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x5d ] : U+005d ] */
{FIRST_PRIMARY + 22, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x5e ^ : U+005e ^ */
{FIRST_PRIMARY + 23, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x5f _ : U+005f _ */
{FIRST_PRIMARY + 24, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x60 ` : U+0060 ` */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x61 a : U+0061 a */
{FIRST_PRIMARY + 68, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x62 b : U+0062 b */
{FIRST_PRIMARY + 69, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 0x63 c : U+0063 c */
{FIRST_PRIMARY + 71, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x64 d : U+0064 d */
{FIRST_PRIMARY + 72, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x65 e : U+0065 e */
{FIRST_PRIMARY + 73, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x66 f : U+0066 f */
{FIRST_PRIMARY + 74, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x67 g : U+0067 g */
{FIRST_PRIMARY + 75, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x68 h : U+0068 h */
{FIRST_PRIMARY + 76, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x69 i : U+0069 i */
{FIRST_PRIMARY + 78, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x6a j : U+006a j */
{FIRST_PRIMARY + 79, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x6b k : U+006b k */
{FIRST_PRIMARY + 80, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x6c l : U+006c l */
{FIRST_PRIMARY + 81, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x6d m : U+006d m */
{FIRST_PRIMARY + 82, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x6e n : U+006e n */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x6f o : U+006f o */
{FIRST_PRIMARY + 84, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x70 p : U+0070 p */
{FIRST_PRIMARY + 85, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x71 q : U+0071 q */
{FIRST_PRIMARY + 86, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x72 r : U+0072 r */
{FIRST_PRIMARY + 87, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x73 s : U+0073 s */
{FIRST_PRIMARY + 89, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x74 t : U+0074 t */
{FIRST_PRIMARY + 90, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x75 u : U+0075 u */
{FIRST_PRIMARY + 91, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x76 v : U+0076 v */
{FIRST_PRIMARY + 92, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x77 w : U+0077 w */
{FIRST_PRIMARY + 93, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0x78 x : U+0078 x */
{FIRST_PRIMARY + 76, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 0x79 y : U+0079 y */
{FIRST_PRIMARY + 94, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/* 0x7a z : U+007a z */
{FIRST_PRIMARY + 25, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x7b { : U+007b { */
{FIRST_PRIMARY + 26, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x7c | : U+007c | */
{FIRST_PRIMARY + 27, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x7d } : U+007d } */
{FIRST_PRIMARY + 28, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0x7e ~ : U+007e ~ */
{FIRST_PRIMARY + 0, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0x7f : U+007f */
{FIRST_IGNORE + 32, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 80 : U+0080 */
{FIRST_IGNORE + 33, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 81 : U+0081 */
{FIRST_IGNORE + 34, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 82 : U+0082 */
{FIRST_IGNORE + 35, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 83 : U+0083 */
{FIRST_IGNORE + 36, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 84 : U+0084 */
{FIRST_IGNORE + 37, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 85 : U+0085 */
{FIRST_IGNORE + 38, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 86 : U+0086 */
{FIRST_IGNORE + 39, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 87 : U+0087 */
{FIRST_IGNORE + 40, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 88 : U+0088 */
{FIRST_IGNORE + 41, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 89 : U+0089 */
{FIRST_IGNORE + 42, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 8a : U+008a */
{FIRST_IGNORE + 43, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 8b : U+008b */
{FIRST_IGNORE + 44, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 8c : U+008c */
{FIRST_IGNORE + 45, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 8d : U+008d */
{FIRST_IGNORE + 46, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 8e : U+008e */
{FIRST_IGNORE + 47, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 8f : U+008f */
{FIRST_IGNORE + 48, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 90 : U+0090 */
{FIRST_IGNORE + 49, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 91 : U+0091 */
{FIRST_IGNORE + 50, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 92 : U+0092 */
{FIRST_IGNORE + 51, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 93 : U+0093 */
{FIRST_IGNORE + 52, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 94 : U+0094 */
{FIRST_IGNORE + 53, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 95 : U+0095 */
{FIRST_IGNORE + 54, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 96 : U+0096 */
{FIRST_IGNORE + 55, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 97 : U+0097 */
{FIRST_IGNORE + 56, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 98 : U+0098 */
{FIRST_IGNORE + 57, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 99 : U+0099 */
{FIRST_IGNORE + 58, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 9a : U+009a */
{FIRST_IGNORE + 59, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 9b : U+009b */
{FIRST_IGNORE + 60, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 9c : U+009c */
{FIRST_IGNORE + 61, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 9d : U+009d */
{FIRST_IGNORE + 62, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 9e : U+009e */
{FIRST_IGNORE + 63, NULL_SECONDARY, NULL_TERTIARY, 1, 1},		/* 9f : U+009f */
{FIRST_PRIMARY + 2, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xa0   : U+00a0 Â  */
{FIRST_PRIMARY + 32, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xa1 ¡ : U+201d â */
{FIRST_PRIMARY + 43, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xa2 ¢ : U+00a2 Â¢ */
{FIRST_PRIMARY + 44, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xa3 £ : U+00a3 Â£ */
{FIRST_PRIMARY + 45, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xa4 ¤ : U+00a4 Â¤ */
{FIRST_PRIMARY + 33, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xa5 ¥ : U+201e â */
{FIRST_PRIMARY + 29, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xa6 ¦ : U+00a6 Â¦ */
{FIRST_PRIMARY + 46, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xa7 § : U+00a7 Â§ */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 5, FIRST_TERTIARY + 1, 0, 0},	/* 0xa8 ¨ : U+00d8 Ã */
{FIRST_PRIMARY + 47, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xa9 © : U+00a9 Â© */
{FIRST_PRIMARY + 86, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xaa ª : U+0156 Å */
{FIRST_PRIMARY + 39, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xab « : U+00ab Â« */
{FIRST_PRIMARY + 48, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xac ¬ : U+00ac Â¬ */
{FIRST_PRIMARY + 0, NULL_SECONDARY, FIRST_TERTIARY + 3, 0, 0},		/* 0xad ­ : U+00ad Â­ */
{FIRST_PRIMARY + 49, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xae ® : U+00ae Â® */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 5, FIRST_TERTIARY + 1, 0, 0},	/* 0xaf ¯ : U+00c6 Ã */
{FIRST_PRIMARY + 50, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xb0 ° : U+00b0 Â° */
{FIRST_PRIMARY + 38, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xb1 ± : U+00b1 Â± */
{FIRST_PRIMARY + 59, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0xb2 ² : U+00b2 Â² */
{FIRST_PRIMARY + 60, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0xb3 ³ : U+00b3 Â³ */
{FIRST_PRIMARY + 31, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xb4 ´ : U+201c â */
{FIRST_PRIMARY + 51, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xb5 µ : U+00b5 Âµ */
{FIRST_PRIMARY + 52, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xb6 ¶ : U+00b6 Â¶ */
{FIRST_PRIMARY + 53, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xb7 · : U+00b7 Â· */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 5, FIRST_TERTIARY + 0, 0, 0},	/* 0xb8 ¸ : U+00f8 Ã¸ */
{FIRST_PRIMARY + 58, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0xb9 ¹ : U+00b9 Â¹ */
{FIRST_PRIMARY + 86, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xba º : U+0157 Å */
{FIRST_PRIMARY + 40, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xbb » : U+00bb Â» */
{FIRST_PRIMARY + 55, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xbc ¼ : U+00bc Â¼ */
{FIRST_PRIMARY + 56, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xbd ½ : U+00bd Â½ */
{FIRST_PRIMARY + 57, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xbe ¾ : U+00be Â¾ */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 5, FIRST_TERTIARY + 0, 0, 0},	/* 0xbf ¿ : U+00e6 Ã¦ */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xc0 À : U+0104 Ä */
{FIRST_PRIMARY + 76, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xc1 Á : U+012e Ä® */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 0xc2 Â : U+0100 Ä */
{FIRST_PRIMARY + 69, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xc3 Ã : U+0106 Ä */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 0xc4 Ä : U+00c4 Ã */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 0xc5 Å : U+00c5 Ã */
{FIRST_PRIMARY + 72, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 0xc6 Æ : U+0118 Ä */
{FIRST_PRIMARY + 72, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 0xc7 Ç : U+0112 Ä */
{FIRST_PRIMARY + 70, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0xc8 È : U+010c Ä */
{FIRST_PRIMARY + 72, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xc9 É : U+00c9 Ã */
{FIRST_PRIMARY + 94, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xca Ê : U+0179 Å¹ */
{FIRST_PRIMARY + 72, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 0xcb Ë : U+0116 Ä */
{FIRST_PRIMARY + 74, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xcc Ì : U+0122 Ä¢ */
{FIRST_PRIMARY + 79, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xcd Í : U+0136 Ä¶ */
{FIRST_PRIMARY + 76, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 0xce Î : U+012a Äª */
{FIRST_PRIMARY + 80, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xcf Ï : U+013b Ä» */
{FIRST_PRIMARY + 88, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0xd0 Ð : U+0160 Å  */
{FIRST_PRIMARY + 82, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xd1 Ñ : U+0143 Å */
{FIRST_PRIMARY + 82, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 0xd2 Ò : U+0145 Å */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xd3 Ó : U+00d3 Ã */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 0xd4 Ô : U+014c Å */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 0xd5 Õ : U+00d5 Ã */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 0xd6 Ö : U+00d6 Ã */
{FIRST_PRIMARY + 41, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xd7 × : U+00d7 Ã */
{FIRST_PRIMARY + 90, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xd8 Ø : U+0172 Å² */
{FIRST_PRIMARY + 80, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 0xd9 Ù : U+0141 Å */
{FIRST_PRIMARY + 87, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 0xda Ú : U+015a Å */
{FIRST_PRIMARY + 90, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 0xdb Û : U+016a Åª */
{FIRST_PRIMARY + 90, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 0xdc Ü : U+00dc Ã */
{FIRST_PRIMARY + 94, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 0xdd Ý : U+017b Å» */
{FIRST_PRIMARY + 95, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},		/* 0xde Þ : U+017d Å½ */
{FIRST_PRIMARY + 87, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},		/* 0xdf ß : U+00df Ã */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xe0 à : U+0105 Ä */
{FIRST_PRIMARY + 76, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xe1 á : U+012f Ä¯ */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 3, FIRST_TERTIARY + 1, 0, 0},	/* 0xe2 â : U+0101 Ä */
{FIRST_PRIMARY + 69, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xe3 ã : U+0107 Ä */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 0xe4 ä : U+00e4 Ã¤ */
{FIRST_PRIMARY + 67, FIRST_SECONDARY + 4, FIRST_TERTIARY + 1, 0, 0},	/* 0xe5 å : U+00e5 Ã¥ */
{FIRST_PRIMARY + 72, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 0xe6 æ : U+0119 Ä */
{FIRST_PRIMARY + 72, FIRST_SECONDARY + 4, FIRST_TERTIARY + 1, 0, 0},	/* 0xe7 ç : U+0113 Ä */
{FIRST_PRIMARY + 70, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0xe8 è : U+010d Ä */
{FIRST_PRIMARY + 72, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xe9 é : U+00e9 Ã© */
{FIRST_PRIMARY + 94, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xea ê : U+017a Åº */
{FIRST_PRIMARY + 72, FIRST_SECONDARY + 3, FIRST_TERTIARY + 1, 0, 0},	/* 0xeb ë : U+0117 Ä */
{FIRST_PRIMARY + 74, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xec ì : U+0123 Ä£ */
{FIRST_PRIMARY + 79, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xed í : U+0137 Ä· */
{FIRST_PRIMARY + 76, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 0xee î : U+012b Ä« */
{FIRST_PRIMARY + 80, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xef ï : U+013c Ä¼ */
{FIRST_PRIMARY + 88, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0xf0 ð : U+0161 Å¡ */
{FIRST_PRIMARY + 82, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xf1 ñ : U+0144 Å */
{FIRST_PRIMARY + 82, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 0xf2 ò : U+0146 Å */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xf3 ó : U+00f3 Ã³ */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 3, FIRST_TERTIARY + 1, 0, 0},	/* 0xf4 ô : U+014d Å */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 4, FIRST_TERTIARY + 1, 0, 0},	/* 0xf5 õ : U+00f5 Ãµ */
{FIRST_PRIMARY + 83, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 0xf6 ö : U+00f6 Ã¶ */
{FIRST_PRIMARY + 42, NULL_SECONDARY, NULL_TERTIARY, 0, 0},		/* 0xf7 ÷ : U+00f7 Ã· */
{FIRST_PRIMARY + 90, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xf8 ø : U+0173 Å³ */
{FIRST_PRIMARY + 80, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 0xf9 ù : U+0142 Å */
{FIRST_PRIMARY + 87, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 0xfa ú : U+015b Å */
{FIRST_PRIMARY + 90, FIRST_SECONDARY + 3, FIRST_TERTIARY + 1, 0, 0},	/* 0xfb û : U+016b Å« */
{FIRST_PRIMARY + 90, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 0xfc ü : U+00fc Ã¼ */
{FIRST_PRIMARY + 94, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 0xfd ý : U+017c Å¼ */
{FIRST_PRIMARY + 95, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},		/* 0xfe þ : U+017e Å¾ */
{FIRST_PRIMARY + 30, NULL_SECONDARY, NULL_TERTIARY, 0, 0}		/* 0xff ÿ : U+2019 â */
};

/* End of File : Language driver xx885913lt (Lithuanian) */
