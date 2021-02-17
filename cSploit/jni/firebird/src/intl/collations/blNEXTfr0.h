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
	160,						/* � 160 -> � 160 */
	161,						/* � 161 -> � 161 */
	162,						/* � 162 -> � 162 */
	163,						/* � 163 -> � 163 */
	164,						/* � 164 -> � 164 */
	165,						/* � 165 -> � 165 */
	166,						/* � 166 -> � 166 */
	167,						/* � 167 -> � 167 */
	168,						/* � 168 -> � 168 */
	169,						/* � 169 -> � 169 */
	170,						/* � 170 -> � 170 */
	171,						/* � 171 -> � 171 */
	172,						/* � 172 -> � 172 */
	173,						/* � 173 -> � 173 */
	174,						/* � 174 -> � 174 */
	175,						/* � 175 -> � 175 */
	176,						/* � 176 -> � 176 */
	177,						/* � 177 -> � 177 */
	178,						/* � 178 -> � 178 */
	179,						/* � 179 -> � 179 */
	180,						/* � 180 -> � 180 */
	181,						/* � 181 -> � 181 */
	182,						/* � 182 -> � 182 */
	183,						/* � 183 -> � 183 */
	184,						/* � 184 -> � 184 */
	185,						/* � 185 -> � 185 */
	186,						/* � 186 -> � 186 */
	187,						/* � 187 -> � 187 */
	188,						/* � 188 -> � 188 */
	189,						/* � 189 -> � 189 */
	190,						/* � 190 -> � 190 */
	191,						/* � 191 -> � 191 */
	192,						/* � 192 -> � 192 */
	193,						/* � 193 -> � 193 */
	194,						/* � 194 -> � 194 */
	195,						/* � 195 -> � 195 */
	196,						/* � 196 -> � 196 */
	197,						/* � 197 -> � 197 */
	198,						/* � 198 -> � 198 */
	199,						/* � 199 -> � 199 */
	200,						/* � 200 -> � 200 */
	201,						/* � 201 -> � 201 */
	202,						/* � 202 -> � 202 */
	203,						/* � 203 -> � 203 */
	204,						/* � 204 -> � 204 */
	205,						/* � 205 -> � 205 */
	206,						/* � 206 -> � 206 */
	207,						/* � 207 -> � 207 */
	208,						/* � 208 -> � 208 */
	209,						/* � 209 -> � 209 */
	210,						/* � 210 -> � 210 */
	211,						/* � 211 -> � 211 */
	212,						/* � 212 -> � 212 */
	65,							/* � 213 -> A  65 */
	65,							/* � 214 -> A  65 */
	65,							/* � 215 -> A  65 */
	132,						/* � 216 ->   132 */
	133,						/* � 217 ->   133 */
	134,						/* � 218 ->   134 */
	67,							/* � 219 -> C  67 */
	69,							/* � 220 -> E  69 */
	69,							/* � 221 -> E  69 */
	69,							/* � 222 -> E  69 */
	69,							/* � 223 -> E  69 */
	73,							/* � 224 -> I  73 */
	225,						/* � 225 -> � 225 */
	73,							/* � 226 -> I  73 */
	227,						/* � 227 -> � 227 */
	73,							/* � 228 -> I  73 */
	73,							/* � 229 -> I  73 */
	144,						/* � 230 ->   144 */
	145,						/* � 231 ->   145 */
	232,						/* � 232 -> � 232 */
	233,						/* � 233 -> � 233 */
	234,						/* � 234 -> � 234 */
	235,						/* � 235 -> � 235 */
	79,							/* � 236 -> O  79 */
	79,							/* � 237 -> O  79 */
	79,							/* � 238 -> O  79 */
	149,						/* � 239 ->   149 */
	150,						/* � 240 ->   150 */
	225,						/* � 241 -> � 225 */
	85,							/* � 242 -> U  85 */
	85,							/* � 243 -> U  85 */
	85,							/* � 244 -> U  85 */
	245,						/* � 245 -> � 245 */
	85,							/* � 246 -> U  85 */
	155,						/* � 247 ->   155 */
	248,						/* � 248 -> � 248 */
	233,						/* � 249 -> � 233 */
	250,						/* � 250 -> � 250 */
	251,						/* � 251 -> � 251 */
	156,						/* � 252 ->   156 */
	89,							/* � 253 -> Y  89 */
	254,						/* � 254 -> � 254 */
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
	213,						/*   129 -> � 213 */
	214,						/*   130 -> � 214 */
	215,						/*   131 -> � 215 */
	216,						/*   132 -> � 216 */
	217,						/*   133 -> � 217 */
	218,						/*   134 -> � 218 */
	219,						/*   135 -> � 219 */
	220,						/*   136 -> � 220 */
	221,						/*   137 -> � 221 */
	222,						/*   138 -> � 222 */
	223,						/*   139 -> � 223 */
	224,						/*   140 -> � 224 */
	226,						/*   141 -> � 226 */
	228,						/*   142 -> � 228 */
	229,						/*   143 -> � 229 */
	230,						/*   144 -> � 230 */
	231,						/*   145 -> � 231 */
	236,						/*   146 -> � 236 */
	237,						/*   147 -> � 237 */
	238,						/*   148 -> � 238 */
	239,						/*   149 -> � 239 */
	240,						/*   150 -> � 240 */
	242,						/*   151 -> � 242 */
	243,						/*   152 -> � 243 */
	244,						/*   153 -> � 244 */
	246,						/*   154 -> � 246 */
	247,						/*   155 -> � 247 */
	252,						/*   156 -> � 252 */
	157,						/*   157 ->   157 */
	158,						/*   158 ->   158 */
	159,						/*   159 ->   159 */
	160,						/* � 160 -> � 160 */
	161,						/* � 161 -> � 161 */
	162,						/* � 162 -> � 162 */
	163,						/* � 163 -> � 163 */
	164,						/* � 164 -> � 164 */
	165,						/* � 165 -> � 165 */
	166,						/* � 166 -> � 166 */
	167,						/* � 167 -> � 167 */
	168,						/* � 168 -> � 168 */
	169,						/* � 169 -> � 169 */
	170,						/* � 170 -> � 170 */
	171,						/* � 171 -> � 171 */
	172,						/* � 172 -> � 172 */
	173,						/* � 173 -> � 173 */
	174,						/* � 174 -> � 174 */
	175,						/* � 175 -> � 175 */
	176,						/* � 176 -> � 176 */
	177,						/* � 177 -> � 177 */
	178,						/* � 178 -> � 178 */
	179,						/* � 179 -> � 179 */
	180,						/* � 180 -> � 180 */
	181,						/* � 181 -> � 181 */
	182,						/* � 182 -> � 182 */
	183,						/* � 183 -> � 183 */
	184,						/* � 184 -> � 184 */
	185,						/* � 185 -> � 185 */
	186,						/* � 186 -> � 186 */
	187,						/* � 187 -> � 187 */
	188,						/* � 188 -> � 188 */
	189,						/* � 189 -> � 189 */
	190,						/* � 190 -> � 190 */
	191,						/* � 191 -> � 191 */
	192,						/* � 192 -> � 192 */
	193,						/* � 193 -> � 193 */
	194,						/* � 194 -> � 194 */
	195,						/* � 195 -> � 195 */
	196,						/* � 196 -> � 196 */
	197,						/* � 197 -> � 197 */
	198,						/* � 198 -> � 198 */
	199,						/* � 199 -> � 199 */
	200,						/* � 200 -> � 200 */
	201,						/* � 201 -> � 201 */
	202,						/* � 202 -> � 202 */
	203,						/* � 203 -> � 203 */
	204,						/* � 204 -> � 204 */
	205,						/* � 205 -> � 205 */
	206,						/* � 206 -> � 206 */
	207,						/* � 207 -> � 207 */
	208,						/* � 208 -> � 208 */
	209,						/* � 209 -> � 209 */
	210,						/* � 210 -> � 210 */
	211,						/* � 211 -> � 211 */
	212,						/* � 212 -> � 212 */
	213,						/* � 213 -> � 213 */
	214,						/* � 214 -> � 214 */
	215,						/* � 215 -> � 215 */
	216,						/* � 216 -> � 216 */
	217,						/* � 217 -> � 217 */
	218,						/* � 218 -> � 218 */
	219,						/* � 219 -> � 219 */
	220,						/* � 220 -> � 220 */
	221,						/* � 221 -> � 221 */
	222,						/* � 222 -> � 222 */
	223,						/* � 223 -> � 223 */
	224,						/* � 224 -> � 224 */
	241,						/* � 225 -> � 241 */
	226,						/* � 226 -> � 226 */
	227,						/* � 227 -> � 227 */
	228,						/* � 228 -> � 228 */
	229,						/* � 229 -> � 229 */
	230,						/* � 230 -> � 230 */
	231,						/* � 231 -> � 231 */
	248,						/* � 232 -> � 248 */
	249,						/* � 233 -> � 249 */
	250,						/* � 234 -> � 250 */
	235,						/* � 235 -> � 235 */
	236,						/* � 236 -> � 236 */
	237,						/* � 237 -> � 237 */
	238,						/* � 238 -> � 238 */
	239,						/* � 239 -> � 239 */
	240,						/* � 240 -> � 240 */
	241,						/* � 241 -> � 241 */
	242,						/* � 242 -> � 242 */
	243,						/* � 243 -> � 243 */
	244,						/* � 244 -> � 244 */
	245,						/* � 245 -> � 245 */
	246,						/* � 246 -> � 246 */
	247,						/* � 247 -> � 247 */
	248,						/* � 248 -> � 248 */
	249,						/* � 249 -> � 249 */
	250,						/* � 250 -> � 250 */
	251,						/* � 251 -> � 251 */
	252,						/* � 252 -> � 252 */
	253,						/* � 253 -> � 253 */
	254,						/* � 254 -> � 254 */
	255							/*   255 ->   255 */
};

static const ExpandChar ExpansionTbl[NUM_EXPAND_CHARS + 1] = {
	{241, 97, 101},				/* � -> ae */
	{225, 65, 69},				/* � -> AE */
	{174, 102, 105},			/* � -> fi */
	{175, 102, 108},			/* � -> fl */
	{250, 111, 101},			/* � -> oe */
	{234, 79, 69},				/* � -> OE */
	{251, 115, 115},			/* � -> ss */
	{252, 116, 104},			/* � -> th */
	{156, 84, 72},				/* � -> TH */
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
	{FIRST_IGNORE + 70, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 160 � */
	{FIRST_IGNORE + 41, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 161 � */
	{FIRST_IGNORE + 74, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 162 � */
	{FIRST_IGNORE + 76, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 163 � */
	{FIRST_IGNORE + 82, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 164 � */
	{FIRST_IGNORE + 77, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 165 � */
	{FIRST_IGNORE + 78, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 166 � */
	{FIRST_IGNORE + 68, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 167 � */
	{FIRST_IGNORE + 73, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 168 � */
	{FIRST_IGNORE + 52, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 169 � */
	{FIRST_IGNORE + 57, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 170 � */
	{FIRST_IGNORE + 55, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 171 � */
	{FIRST_IGNORE + 60, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 172 � */
	{FIRST_IGNORE + 61, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 173 � */
	{FIRST_PRIMARY + 16, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 1, 0},	/* 174 � */
	{FIRST_PRIMARY + 16, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 1, 0},	/* 175 � */
	{FIRST_IGNORE + 71, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 176 � */
	{FIRST_IGNORE + 35, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 177 � */
	{FIRST_IGNORE + 98, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 178 � */
	{FIRST_IGNORE + 99, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 179 � */
	{FIRST_IGNORE + 49, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 180 � */
	{FIRST_IGNORE + 96, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 181 � */
	{FIRST_IGNORE + 69, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 182 � */
	{FIRST_IGNORE + 50, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 183 � */
	{FIRST_IGNORE + 53, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 184 � */
	{FIRST_IGNORE + 59, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 185 � */
	{FIRST_IGNORE + 58, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 186 � */
	{FIRST_IGNORE + 56, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 187 � */
	{FIRST_IGNORE + 45, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 188 � */
	{FIRST_IGNORE + 86, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 189 � */
	{FIRST_IGNORE + 94, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 190 � */
	{FIRST_IGNORE + 43, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 191 � */
	{FIRST_PRIMARY + 2, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/* 192 � */
	{FIRST_IGNORE + 100, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 193 � */
	{FIRST_IGNORE + 101, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 194 � */
	{FIRST_IGNORE + 102, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 195 � */
	{FIRST_IGNORE + 103, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 196 � */
	{FIRST_IGNORE + 104, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 197 � */
	{FIRST_IGNORE + 105, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 198 � */
	{FIRST_IGNORE + 106, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 199 � */
	{FIRST_IGNORE + 107, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 200 � */
	{FIRST_PRIMARY + 3, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/* 201 � */
	{FIRST_IGNORE + 108, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 202 � */
	{FIRST_IGNORE + 109, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 203 � */
	{FIRST_PRIMARY + 4, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0},	/* 204 � */
	{FIRST_IGNORE + 110, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 205 � */
	{FIRST_IGNORE + 111, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 206 � */
	{FIRST_IGNORE + 112, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 207 � */
	{FIRST_IGNORE + 36, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 208 � */
	{FIRST_IGNORE + 88, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 209 � */
	{FIRST_PRIMARY + 1, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0},	/* 210 � */
	{FIRST_PRIMARY + 1, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0},	/* 211 � */
	{FIRST_PRIMARY + 1, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0},	/* 212 � */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 213 � */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 214 � */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 5, FIRST_TERTIARY + 0, 0, 0},	/* 215 � */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 8, FIRST_TERTIARY + 0, 0, 0},	/* 216 � */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 7, FIRST_TERTIARY + 0, 0, 0},	/* 217 � */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 6, FIRST_TERTIARY + 0, 0, 0},	/* 218 � */
	{FIRST_PRIMARY + 13, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 219 � */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 220 � */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 221 � */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 222 � */
	{FIRST_PRIMARY + 15, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 223 � */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 224 � */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 1, 0},	/* 225 � */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 226 � */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 227 � */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 228 � */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 5, FIRST_TERTIARY + 0, 0, 0},	/* 229 � */
	{FIRST_PRIMARY + 14, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 230 � */
	{FIRST_PRIMARY + 24, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 231 � */
	{FIRST_PRIMARY + 22, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0},	/* 232 � */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 8, FIRST_TERTIARY + 1, 0, 0},	/* 233 � */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 2, FIRST_TERTIARY + 1, 1, 0},	/* 234 � */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 235 � */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 236 � */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 237 � */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 5, FIRST_TERTIARY + 0, 0, 0},	/* 238 � */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 7, FIRST_TERTIARY + 0, 0, 0},	/* 239 � */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 6, FIRST_TERTIARY + 0, 0, 0},	/* 240 � */
	{FIRST_PRIMARY + 11, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 1, 0},	/* 241 � */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 242 � */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 243 � */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 3, FIRST_TERTIARY + 0, 0, 0},	/* 244 � */
	{FIRST_PRIMARY + 19, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 245 � */
	{FIRST_PRIMARY + 31, FIRST_SECONDARY + 4, FIRST_TERTIARY + 0, 0, 0},	/* 246 � */
	{FIRST_PRIMARY + 35, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 247 � */
	{FIRST_PRIMARY + 22, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0},	/* 248 � */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 8, FIRST_TERTIARY + 0, 0, 0},	/* 249 � */
	{FIRST_PRIMARY + 25, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 1, 0},	/* 250 � */
	{FIRST_PRIMARY + 29, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 1, 0},	/* 251 � */
	{FIRST_PRIMARY + 30, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 1, 0},	/* 252 � */
	{FIRST_PRIMARY + 35, FIRST_SECONDARY + 2, FIRST_TERTIARY + 0, 0, 0},	/* 253 � */
	{FIRST_IGNORE + 114, NULL_SECONDARY, NULL_TERTIARY, 1, 1},	/* 254 � */
	{FIRST_IGNORE + 115, NULL_SECONDARY, NULL_TERTIARY, 1, 1}	/* 255   */
};

/* End of File : Language driver blnxtfr0 */
