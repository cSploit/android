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

const int NUM_EXPAND_CHARS		= 9;
const int NUM_COMPRESS_CHARS	= 0;
const int LOWERCASE_LEN			= 256;
const int UPPERCASE_LEN			= 256;
const int NOCASESORT_LEN		= 256;
const int LDRV_TIEBREAK			= SECONDARY + REVERSE;

//const int MAX_NCO_PRIMARY		= 36;
const int MAX_NCO_SECONDARY		= 8;
const int MAX_NCO_TERTIARY		= 1;
//const int MAX_NCO_IGNORE		= 115;
const int NULL_SECONDARY		= 0;
const int NULL_TERTIARY			= 0;
const int FIRST_IGNORE			= 1;
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
	129,						/*   129 ->   129 */
	130,						/*   130 ->   130 */
	131,						/*   131 ->   131 */
	132,						/*   132 ->   132 */
	133,						/*   133 ->   133 */
	134,						/*   134 ->   134 */
	135,						/*   135 ->   135 */
	136,						/*   136 ->   136 */
	137,						/*   137 ->   137 */
	138,						/*   138 ->   138 */
	139,						/*   139 ->   139 */
	140,						/*   140 ->   140 */
	141,						/*   141 ->   141 */
	142,						/*   142 ->   142 */
	143,						/*   143 ->   143 */
	144,						/*   144 ->   144 */
	145,						/*   145 ->   145 */
	146,						/*   146 ->   146 */
	147,						/*   147 ->   147 */
	148,						/*   148 ->   148 */
	149,						/*   149 ->   149 */
	150,						/*   150 ->   150 */
	151,						/*   151 ->   151 */
	152,						/*   152 ->   152 */
	153,						/*   153 ->   153 */
	154,						/*   154 ->   154 */
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
	65,							/* Õ 213 -> A  65 */
	65,							/* Ö 214 -> A  65 */
	65,							/* × 215 -> A  65 */
	132,						/* Ø 216 ->   132 */
	133,						/* Ù 217 ->   133 */
	134,						/* Ú 218 ->   134 */
	67,							/* Û 219 -> C  67 */
	69,							/* Ü 220 -> E  69 */
	69,							/* Ý 221 -> E  69 */
	69,							/* Þ 222 -> E  69 */
	69,							/* ß 223 -> E  69 */
	73,							/* à 224 -> I  73 */
	225,						/* á 225 -> á 225 */
	73,							/* â 226 -> I  73 */
	227,						/* ã 227 -> ã 227 */
	73,							/* ä 228 -> I  73 */
	73,							/* å 229 -> I  73 */
	144,						/* æ 230 ->   144 */
	145,						/* ç 231 ->   145 */
	232,						/* è 232 -> è 232 */
	233,						/* é 233 -> é 233 */
	234,						/* ê 234 -> ê 234 */
	235,						/* ë 235 -> ë 235 */
	79,							/* ì 236 -> O  79 */
	79,							/* í 237 -> O  79 */
	79,							/* î 238 -> O  79 */
	149,						/* ï 239 ->   149 */
	150,						/* ð 240 ->   150 */
	225,						/* ñ 241 -> á 225 */
	85,							/* ò 242 -> U  85 */
	85,							/* ó 243 -> U  85 */
	85,							/* ô 244 -> U  85 */
	245,						/* õ 245 -> õ 245 */
	85,							/* ö 246 -> U  85 */
	155,						/* ÷ 247 ->   155 */
	248,						/* ø 248 -> ø 248 */
	233,						/* ù 249 -> é 233 */
	250,						/* ú 250 -> ú 250 */
	251,						/* û 251 -> û 251 */
	156,						/* ü 252 ->   156 */
	89,							/* ý 253 -> Y  89 */
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
	213,						/*   129 -> Õ 213 */
	214,						/*   130 -> Ö 214 */
	215,						/*   131 -> × 215 */
	216,						/*   132 -> Ø 216 */
	217,						/*   133 -> Ù 217 */
	218,						/*   134 -> Ú 218 */
	219,						/*   135 -> Û 219 */
	220,						/*   136 -> Ü 220 */
	221,						/*   137 -> Ý 221 */
	222,						/*   138 -> Þ 222 */
	223,						/*   139 -> ß 223 */
	224,						/*   140 -> à 224 */
	226,						/*   141 -> â 226 */
	228,						/*   142 -> ä 228 */
	229,						/*   143 -> å 229 */
	230,						/*   144 -> æ 230 */
	231,						/*   145 -> ç 231 */
	236,						/*   146 -> ì 236 */
	237,						/*   147 -> í 237 */
	238,						/*   148 -> î 238 */
	239,						/*   149 -> ï 239 */
	240,						/*   150 -> ð 240 */
	242,						/*   151 -> ò 242 */
	243,						/*   152 -> ó 243 */
	244,						/*   153 -> ô 244 */
	246,						/*   154 -> ö 246 */
	247,						/*   155 -> ÷ 247 */
	252,						/*   156 -> ü 252 */
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
	241,						/* á 225 -> ñ 241 */
	226,						/* â 226 -> â 226 */
	227,						/* ã 227 -> ã 227 */
	228,						/* ä 228 -> ä 228 */
	229,						/* å 229 -> å 229 */
	230,						/* æ 230 -> æ 230 */
	231,						/* ç 231 -> ç 231 */
	248,						/* è 232 -> ø 248 */
	249,						/* é 233 -> ù 249 */
	250,						/* ê 234 -> ú 250 */
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
	251,						/* û 251 -> û 251 */
	252,						/* ü 252 -> ü 252 */
	253,						/* ý 253 -> ý 253 */
	254,						/* þ 254 -> þ 254 */
	255							/*   255 ->   255 */
};

static const ExpandChar ExpansionTbl[NUM_EXPAND_CHARS + 1] = {
	{241, 97, 101},				/* ñ -> ae */
	{225, 65, 69},				/* á -> AE */
	{174, 102, 105},			/* ® -> fi */
	{175, 102, 108},			/* ¯ -> fl */
	{250, 111, 101},			/* ú -> oe */
	{234, 79, 69},				/* ê -> OE */
	{251, 115, 115},			/* û -> ss */
	{252, 116, 104},			/* ü -> th */
	{156, 84, 72},				/* œ -> TH */
	{0, 0, 0}					/* END OF TABLE */
};

static const CompressPair CompressTbl[NUM_COMPRESS_CHARS + 1] = {
	{{0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}	/*END OF TABLE */
};

static const SortOrderTblEntry NoCaseOrderTbl[NOCASESORT_LEN] = {
	{FIRST_IGNORE + 0, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*   0   */
	{FIRST_IGNORE + 1, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*   1   */
	{FIRST_IGNORE + 2, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*   2   */
	{FIRST_IGNORE + 3, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*   3   */
	{FIRST_IGNORE + 4, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*   4   */
	{FIRST_IGNORE + 5, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*   5   */
	{FIRST_IGNORE + 6, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*   6   */
	{FIRST_IGNORE + 7, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*   7   */
	{FIRST_IGNORE + 8, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*   8   */
	{FIRST_IGNORE + 9, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*   9   */
	{FIRST_IGNORE + 10, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  10   */
	{FIRST_IGNORE + 11, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  11   */
	{FIRST_IGNORE + 12, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  12   */
	{FIRST_IGNORE + 13, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  13   */
	{FIRST_IGNORE + 14, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  14   */
	{FIRST_IGNORE + 15, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  15   */
	{FIRST_IGNORE + 16, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  16   */
	{FIRST_IGNORE + 17, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  17   */
	{FIRST_IGNORE + 18, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  18   */
	{FIRST_IGNORE + 19, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  19   */
	{FIRST_IGNORE + 20, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  20   */
	{FIRST_IGNORE + 21, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  21   */
	{FIRST_IGNORE + 22, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  22   */
	{FIRST_IGNORE + 23, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  23   */
	{FIRST_IGNORE + 24, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  24   */
	{FIRST_IGNORE + 25, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  25   */
	{FIRST_IGNORE + 26, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  26   */
	{FIRST_IGNORE + 27, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  27   */
	{FIRST_IGNORE + 28, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  28   */
	{FIRST_IGNORE + 29, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  29   */
	{FIRST_IGNORE + 30, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  30   */
	{FIRST_IGNORE + 31, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  31   */
	{FIRST_IGNORE + 32, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  32   */
	{FIRST_IGNORE + 40, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  33 ! */
	{FIRST_IGNORE + 54, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  34 " */
	{FIRST_IGNORE + 84, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  35 # */
	{FIRST_IGNORE + 75, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  36 $ */
	{FIRST_IGNORE + 85, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  37 % */
	{FIRST_IGNORE + 83, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  38 & */
	{FIRST_IGNORE + 51, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  39 ' */
	{FIRST_IGNORE + 62, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  40 ( */
	{FIRST_IGNORE + 63, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  41 ) */
	{FIRST_IGNORE + 79, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  42 * */
	{FIRST_IGNORE + 87, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  43 + */
	{FIRST_IGNORE + 37, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  44 , */
	{FIRST_IGNORE + 34, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  45 - */
	{FIRST_IGNORE + 44, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  46 . */
	{FIRST_IGNORE + 81, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  47 / */
	{FIRST_PRIMARY + 1, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0},	/*  48 0 */
	{FIRST_PRIMARY + 2, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/*  49 1 */
	{FIRST_PRIMARY + 3, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/*  50 2 */
	{FIRST_PRIMARY + 4, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/*  51 3 */
	{FIRST_PRIMARY + 5, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  52 4 */
	{FIRST_PRIMARY + 6, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  53 5 */
	{FIRST_PRIMARY + 7, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  54 6 */
	{FIRST_PRIMARY + 8, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  55 7 */
	{FIRST_PRIMARY + 9, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  56 8 */
	{FIRST_PRIMARY + 10, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/*  57 9 */
	{FIRST_IGNORE + 39, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  58 : */
	{FIRST_IGNORE + 38, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  59 ; */
	{FIRST_IGNORE + 91, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  60 < */
	{FIRST_IGNORE + 92, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  61 = */
	{FIRST_IGNORE + 93, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  62 > */
	{FIRST_IGNORE + 42, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  63 ? */
	{FIRST_IGNORE + 72, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  64 @ */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  65 A */
	{FIRST_PRIMARY + 12, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  66 B */
	{FIRST_PRIMARY + 13, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  67 C */
	{FIRST_PRIMARY + 14, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  68 D */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  69 E */
	{FIRST_PRIMARY + 16, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  70 F */
	{FIRST_PRIMARY + 17, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  71 G */
	{FIRST_PRIMARY + 18, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  72 H */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  73 I */
	{FIRST_PRIMARY + 20, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  74 J */
	{FIRST_PRIMARY + 21, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  75 K */
	{FIRST_PRIMARY + 22, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  76 L */
	{FIRST_PRIMARY + 23, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  77 M */
	{FIRST_PRIMARY + 24, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  78 N */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  79 O */
	{FIRST_PRIMARY + 26, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  80 P */
	{FIRST_PRIMARY + 27, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  81 Q */
	{FIRST_PRIMARY + 28, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  82 R */
	{FIRST_PRIMARY + 29, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  83 S */
	{FIRST_PRIMARY + 30, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  84 T */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  85 U */
	{FIRST_PRIMARY + 32, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  86 V */
	{FIRST_PRIMARY + 33, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  87 W */
	{FIRST_PRIMARY + 34, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  88 X */
	{FIRST_PRIMARY + 35, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0},	/*  89 Y */
	{FIRST_PRIMARY + 36, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/*  90 Z */
	{FIRST_IGNORE + 64, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  91 [ */
	{FIRST_IGNORE + 80, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  92 \ */
	{FIRST_IGNORE + 65, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  93 ] */
	{FIRST_IGNORE + 47, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  94 ^ */
	{FIRST_IGNORE + 33, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  95 _ */
	{FIRST_IGNORE + 46, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/*  96 ` */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/*  97 a */
	{FIRST_PRIMARY + 12, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/*  98 b */
	{FIRST_PRIMARY + 13, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/*  99 c */
	{FIRST_PRIMARY + 14, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 100 d */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 101 e */
	{FIRST_PRIMARY + 16, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 102 f */
	{FIRST_PRIMARY + 17, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 103 g */
	{FIRST_PRIMARY + 18, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 104 h */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 105 i */
	{FIRST_PRIMARY + 20, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 106 j */
	{FIRST_PRIMARY + 21, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 107 k */
	{FIRST_PRIMARY + 22, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 108 l */
	{FIRST_PRIMARY + 23, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 109 m */
	{FIRST_PRIMARY + 24, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 110 n */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 111 o */
	{FIRST_PRIMARY + 26, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 112 p */
	{FIRST_PRIMARY + 27, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 113 q */
	{FIRST_PRIMARY + 28, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 114 r */
	{FIRST_PRIMARY + 29, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 115 s */
	{FIRST_PRIMARY + 30, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 116 t */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 117 u */
	{FIRST_PRIMARY + 32, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 118 v */
	{FIRST_PRIMARY + 33, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 119 w */
	{FIRST_PRIMARY + 34, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 120 x */
	{FIRST_PRIMARY + 35, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0},	/* 121 y */
	{FIRST_PRIMARY + 36, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0},	/* 122 z */
	{FIRST_IGNORE + 66, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 123 { */
	{FIRST_IGNORE + 95, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 124 | */
	{FIRST_IGNORE + 67, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 125 } */
	{FIRST_IGNORE + 48, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 126 ~ */
	{FIRST_IGNORE + 113, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 127   */
	{FIRST_PRIMARY + 0, NULL_SECONDARY, NULL_TERTIARY, 0, 0},	/* 128   */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 4, FIRST_TERTIARY + 1, 0, 0},	/* 129   */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 3, FIRST_TERTIARY + 1, 0, 0},	/* 130   */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 5, FIRST_TERTIARY + 1, 0, 0},	/* 131   */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 8, FIRST_TERTIARY + 1, 0, 0},	/* 132   */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 7, FIRST_TERTIARY + 1, 0, 0},	/* 133   */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 6, FIRST_TERTIARY + 1, 0, 0},	/* 134   */
	{FIRST_PRIMARY + 13, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 135   */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 136   */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 137   */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 3, FIRST_TERTIARY + 1, 0, 0},	/* 138   */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 4, FIRST_TERTIARY + 1, 0, 0},	/* 139   */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 3, FIRST_TERTIARY + 1, 0, 0},	/* 140   */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 141   */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 4, FIRST_TERTIARY + 1, 0, 0},	/* 142   */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 5, FIRST_TERTIARY + 1, 0, 0},	/* 143   */
	{FIRST_PRIMARY + 14, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 144   */
	{FIRST_PRIMARY + 24, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 145   */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 4, FIRST_TERTIARY + 1, 0, 0},	/* 146   */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 3, FIRST_TERTIARY + 1, 0, 0},	/* 147   */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 5, FIRST_TERTIARY + 1, 0, 0},	/* 148   */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 7, FIRST_TERTIARY + 1, 0, 0},	/* 149   */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 6, FIRST_TERTIARY + 1, 0, 0},	/* 150   */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 0, 0},	/* 151   */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 152   */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 3, FIRST_TERTIARY + 1, 0, 0},	/* 153   */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 4, FIRST_TERTIARY + 1, 0, 0},	/* 154   */
	{FIRST_PRIMARY + 35, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 155   */
	{FIRST_PRIMARY + 30, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 1, 0},	/* 156   */
	{FIRST_IGNORE + 97, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 157   */
	{FIRST_IGNORE + 90, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 158   */
	{FIRST_IGNORE + 89, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 159   */
	{FIRST_IGNORE + 70, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 160   */
	{FIRST_IGNORE + 41, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 161 ¡ */
	{FIRST_IGNORE + 74, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 162 ¢ */
	{FIRST_IGNORE + 76, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 163 £ */
	{FIRST_IGNORE + 82, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 164 ¤ */
	{FIRST_IGNORE + 77, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 165 ¥ */
	{FIRST_IGNORE + 78, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 166 ¦ */
	{FIRST_IGNORE + 68, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 167 § */
	{FIRST_IGNORE + 73, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 168 ¨ */
	{FIRST_IGNORE + 52, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 169 © */
	{FIRST_IGNORE + 57, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 170 ª */
	{FIRST_IGNORE + 55, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 171 « */
	{FIRST_IGNORE + 60, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 172 ¬ */
	{FIRST_IGNORE + 61, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 173 ­ */
	{FIRST_PRIMARY + 16, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 1, 0},	/* 174 ® */
	{FIRST_PRIMARY + 16, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 1, 0},	/* 175 ¯ */
	{FIRST_IGNORE + 71, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 176 ° */
	{FIRST_IGNORE + 35, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 177 ± */
	{FIRST_IGNORE + 98, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 178 ² */
	{FIRST_IGNORE + 99, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 179 ³ */
	{FIRST_IGNORE + 49, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 180 ´ */
	{FIRST_IGNORE + 96, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 181 µ */
	{FIRST_IGNORE + 69, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 182 ¶ */
	{FIRST_IGNORE + 50, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 183 · */
	{FIRST_IGNORE + 53, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 184 ¸ */
	{FIRST_IGNORE + 59, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 185 ¹ */
	{FIRST_IGNORE + 58, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 186 º */
	{FIRST_IGNORE + 56, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 187 » */
	{FIRST_IGNORE + 45, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 188 ¼ */
	{FIRST_IGNORE + 86, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 189 ½ */
	{FIRST_IGNORE + 94, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 190 ¾ */
	{FIRST_IGNORE + 43, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 191 ¿ */
	{FIRST_PRIMARY + 2, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/* 192 À */
	{FIRST_IGNORE + 100, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 193 Á */
	{FIRST_IGNORE + 101, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 194 Â */
	{FIRST_IGNORE + 102, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 195 Ã */
	{FIRST_IGNORE + 103, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 196 Ä */
	{FIRST_IGNORE + 104, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 197 Å */
	{FIRST_IGNORE + 105, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 198 Æ */
	{FIRST_IGNORE + 106, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 199 Ç */
	{FIRST_IGNORE + 107, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 200 È */
	{FIRST_PRIMARY + 3, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/* 201 É */
	{FIRST_IGNORE + 108, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 202 Ê */
	{FIRST_IGNORE + 109, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 203 Ë */
	{FIRST_PRIMARY + 4, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/* 204 Ì */
	{FIRST_IGNORE + 110, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 205 Í */
	{FIRST_IGNORE + 111, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 206 Î */
	{FIRST_IGNORE + 112, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 207 Ï */
	{FIRST_IGNORE + 36, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 208 Ð */
	{FIRST_IGNORE + 88, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 209 Ñ */
	{FIRST_PRIMARY + 1, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/* 210 Ò */
	{FIRST_PRIMARY + 1, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},	/* 211 Ó */
	{FIRST_PRIMARY + 1, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0},	/* 212 Ô */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 213 Õ */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 214 Ö */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 5, FIRST_TERTIARY + 0, 0, 0},	/* 215 × */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 8, FIRST_TERTIARY + 0, 0, 0},	/* 216 Ø */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 7, FIRST_TERTIARY + 0, 0, 0},	/* 217 Ù */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 6, FIRST_TERTIARY + 0, 0, 0},	/* 218 Ú */
	{FIRST_PRIMARY + 13, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 219 Û */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 220 Ü */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 221 Ý */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 222 Þ */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 223 ß */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 224 à */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 1, 0},	/* 225 á */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 226 â */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 227 ã */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 228 ä */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 5, FIRST_TERTIARY + 0, 0, 0},	/* 229 å */
	{FIRST_PRIMARY + 14, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 230 æ */
	{FIRST_PRIMARY + 24, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 231 ç */
	{FIRST_PRIMARY + 22, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 232 è */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 8, FIRST_TERTIARY + 1, 0, 0},	/* 233 é */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 1, 0},	/* 234 ê */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 235 ë */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 236 ì */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 237 í */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 5, FIRST_TERTIARY + 0, 0, 0},	/* 238 î */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 7, FIRST_TERTIARY + 0, 0, 0},	/* 239 ï */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 6, FIRST_TERTIARY + 0, 0, 0},	/* 240 ð */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 1, 0},	/* 241 ñ */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 242 ò */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 243 ó */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 244 ô */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 245 õ */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 246 ö */
	{FIRST_PRIMARY + 35, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 247 ÷ */
	{FIRST_PRIMARY + 22, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 248 ø */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 8, FIRST_TERTIARY + 0, 0, 0},	/* 249 ù */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 1, 0},	/* 250 ú */
	{FIRST_PRIMARY + 29, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 1, 0},	/* 251 û */
	{FIRST_PRIMARY + 30, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 1, 0},	/* 252 ü */
	{FIRST_PRIMARY + 35, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 253 ý */
	{FIRST_IGNORE + 114, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 254 þ */
	{FIRST_IGNORE + 115, NULL_SECONDARY, NULL_TERTIARY, 1, 1}	/* 255   */
};

/* End of File : Language driver blnxtfr0 */
