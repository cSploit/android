/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Aleksey Karyakin
 *  for the Yaffil project.
 *
 *  Copyright (c) 2005 Aleksey Karyakin <aleksey.karyakin@mail.ru>
 *  and all contributors signed below.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

const int NUM_EXPAND_CHARS		= 0;
const int NUM_COMPRESS_CHARS	= 0;
const int LOWERCASE_LEN			= 256;
const int UPPERCASE_LEN			= 256;
const int NOCASESORT_LEN		= 256;
const int LDRV_TIEBREAK			= 0; // TODO

//const int MAX_NCO_PRIMARY		= 159;
const int MAX_NCO_SECONDARY		= 3;
const int MAX_NCO_TERTIARY		= 1;
//const int MAX_NCO_IGNORE		= 0;
const int NULL_SECONDARY		= 0;
const int NULL_TERTIARY			= 0;
//const int FIRST_IGNORE			= 1;
const int FIRST_TERTIARY		= 1;
const int FIRST_SECONDARY		= (FIRST_TERTIARY + MAX_NCO_TERTIARY + 1);
const int FIRST_PRIMARY			= (FIRST_SECONDARY + MAX_NCO_SECONDARY + 1);

static const BYTE ToUpperConversionTbl [ UPPERCASE_LEN ] = {
	  0, /*     0 ->     0 */
	  1, /*     1 ->     1 */
	  2, /*     2 ->     2 */
	  3, /*     3 ->     3 */
	  4, /*     4 ->     4 */
	  5, /*     5 ->     5 */
	  6, /*     6 ->     6 */
	  7, /*     7 ->     7 */
	  8, /*     8 ->     8 */
	  9, /*     9 ->     9 */
	 10, /*    10 ->    10 */
	 11, /*    11 ->    11 */
	 12, /*    12 ->    12 */
	 13, /*    13 ->    13 */
	 14, /*    14 ->    14 */
	 15, /*    15 ->    15 */
	 16, /*    16 ->    16 */
	 17, /*    17 ->    17 */
	 18, /*    18 ->    18 */
	 19, /*    19 ->    19 */
	 20, /*    20 ->    20 */
	 21, /*    21 ->    21 */
	 22, /*    22 ->    22 */
	 23, /*    23 ->    23 */
	 24, /*    24 ->    24 */
	 25, /*    25 ->    25 */
	 26, /*    26 ->    26 */
	 27, /*    27 ->    27 */
	 28, /*    28 ->    28 */
	 29, /*    29 ->    29 */
	 30, /*    30 ->    30 */
	 31, /*    31 ->    31 */
	 32, /*    32 ->    32 */
	 33, /* !  33 -> !  33 */
	 34, /* "  34 -> "  34 */
	 35, /* #  35 -> #  35 */
	 36, /* $  36 -> $  36 */
	 37, /* %  37 -> %  37 */
	 38, /* &  38 -> &  38 */
	 39, /* '  39 -> '  39 */
	 40, /* (  40 -> (  40 */
	 41, /* )  41 -> )  41 */
	 42, /* *  42 -> *  42 */
	 43, /* +  43 -> +  43 */
	 44, /* ,  44 -> ,  44 */
	 45, /* -  45 -> -  45 */
	 46, /* .  46 -> .  46 */
	 47, /* /  47 -> /  47 */
	 48, /* 0  48 -> 0  48 */
	 49, /* 1  49 -> 1  49 */
	 50, /* 2  50 -> 2  50 */
	 51, /* 3  51 -> 3  51 */
	 52, /* 4  52 -> 4  52 */
	 53, /* 5  53 -> 5  53 */
	 54, /* 6  54 -> 6  54 */
	 55, /* 7  55 -> 7  55 */
	 56, /* 8  56 -> 8  56 */
	 57, /* 9  57 -> 9  57 */
	 58, /* :  58 -> :  58 */
	 59, /* ;  59 -> ;  59 */
	 60, /* <  60 -> <  60 */
	 61, /* =  61 -> =  61 */
	 62, /* >  62 -> >  62 */
	 63, /* ?  63 -> ?  63 */
	 64, /* @  64 -> @  64 */
	 65, /* A  65 -> A  65 */
	 66, /* B  66 -> B  66 */
	 67, /* C  67 -> C  67 */
	 68, /* D  68 -> D  68 */
	 69, /* E  69 -> E  69 */
	 70, /* F  70 -> F  70 */
	 71, /* G  71 -> G  71 */
	 72, /* H  72 -> H  72 */
	 73, /* I  73 -> I  73 */
	 74, /* J  74 -> J  74 */
	 75, /* K  75 -> K  75 */
	 76, /* L  76 -> L  76 */
	 77, /* M  77 -> M  77 */
	 78, /* N  78 -> N  78 */
	 79, /* O  79 -> O  79 */
	 80, /* P  80 -> P  80 */
	 81, /* Q  81 -> Q  81 */
	 82, /* R  82 -> R  82 */
	 83, /* S  83 -> S  83 */
	 84, /* T  84 -> T  84 */
	 85, /* U  85 -> U  85 */
	 86, /* V  86 -> V  86 */
	 87, /* W  87 -> W  87 */
	 88, /* X  88 -> X  88 */
	 89, /* Y  89 -> Y  89 */
	 90, /* Z  90 -> Z  90 */
	 91, /* [  91 -> [  91 */
	 92, /* \  92 -> \  92 */
	 93, /* ]  93 -> ]  93 */
	 94, /* ^  94 -> ^  94 */
	 95, /* _  95 -> _  95 */
	 96, /* `  96 -> `  96 */
	 65, /* a  97 -> A  65 */
	 66, /* b  98 -> B  66 */
	 67, /* c  99 -> C  67 */
	 68, /* d 100 -> D  68 */
	 69, /* e 101 -> E  69 */
	 70, /* f 102 -> F  70 */
	 71, /* g 103 -> G  71 */
	 72, /* h 104 -> H  72 */
	 73, /* i 105 -> I  73 */
	 74, /* j 106 -> J  74 */
	 75, /* k 107 -> K  75 */
	 76, /* l 108 -> L  76 */
	 77, /* m 109 -> M  77 */
	 78, /* n 110 -> N  78 */
	 79, /* o 111 -> O  79 */
	 80, /* p 112 -> P  80 */
	 81, /* q 113 -> Q  81 */
	 82, /* r 114 -> R  82 */
	 83, /* s 115 -> S  83 */
	 84, /* t 116 -> T  84 */
	 85, /* u 117 -> U  85 */
	 86, /* v 118 -> V  86 */
	 87, /* w 119 -> W  87 */
	 88, /* x 120 -> X  88 */
	 89, /* y 121 -> Y  89 */
	 90, /* z 122 -> Z  90 */
	123, /* { 123 -> { 123 */
	124, /* | 124 -> | 124 */
	125, /* } 125 -> } 125 */
	126, /* ~ 126 -> ~ 126 */
	127, /*  127 ->  127 */
	128, /* Ä 128 -> Ä 128 */
	129, /* Å 129 -> Å 129 */
	130, /* Ç 130 -> Ç 130 */
	131, /* É 131 -> É 131 */
	132, /* Ñ 132 -> Ñ 132 */
	133, /* Ö 133 -> Ö 133 */
	134, /* Ü 134 -> Ü 134 */
	135, /* á 135 -> á 135 */
	136, /* à 136 -> à 136 */
	137, /* â 137 -> â 137 */
	138, /* ä 138 -> ä 138 */
	139, /* ã 139 -> ã 139 */
	140, /* å 140 -> å 140 */
	141, /* ç 141 -> ç 141 */
	142, /* é 142 -> é 142 */
	143, /* è 143 -> è 143 */
	144, /* ê 144 -> ê 144 */
	145, /* ë 145 -> ë 145 */
	146, /* í 146 -> í 146 */
	147, /* ì 147 -> ì 147 */
	148, /* î 148 -> î 148 */
	149, /* ï 149 -> ï 149 */
	150, /* ñ 150 -> ñ 150 */
	151, /* ó 151 -> ó 151 */
	152, /* ò 152 -> ò 152 */
	153, /* ô 153 -> ô 153 */
	154, /* ö 154 -> ö 154 */
	155, /* õ 155 -> õ 155 */
	156, /* ú 156 -> ú 156 */
	157, /* ù 157 -> ù 157 */
	158, /* û 158 -> û 158 */
	159, /* ü 159 -> ü 159 */
	160, /* † 160 -> † 160 */
	161, /* ° 161 -> ° 161 */
	162, /* ¢ 162 -> ¢ 162 */
	179, /* £ 163 -> ≥ 179 */
	164, /* § 164 -> § 164 */
	165, /* • 165 -> • 165 */
	166, /* ¶ 166 -> ¶ 166 */
	167, /* ß 167 -> ß 167 */
	168, /* ® 168 -> ® 168 */
	169, /* © 169 -> © 169 */
	170, /* ™ 170 -> ™ 170 */
	171, /* ´ 171 -> ´ 171 */
	172, /* ¨ 172 -> ¨ 172 */
	173, /* ≠ 173 -> ≠ 173 */
	174, /* Æ 174 -> Æ 174 */
	175, /* Ø 175 -> Ø 175 */
	176, /* ∞ 176 -> ∞ 176 */
	177, /* ± 177 -> ± 177 */
	178, /* ≤ 178 -> ≤ 178 */
	179, /* ≥ 179 -> ≥ 179 */
	180, /* ¥ 180 -> ¥ 180 */
	181, /* µ 181 -> µ 181 */
	182, /* ∂ 182 -> ∂ 182 */
	183, /* ∑ 183 -> ∑ 183 */
	184, /* ∏ 184 -> ∏ 184 */
	185, /* π 185 -> π 185 */
	186, /* ∫ 186 -> ∫ 186 */
	187, /* ª 187 -> ª 187 */
	188, /* º 188 -> º 188 */
	189, /* Ω 189 -> Ω 189 */
	190, /* æ 190 -> æ 190 */
	191, /* ø 191 -> ø 191 */
	224, /* ¿ 192 -> ‡ 224 */
	225, /* ¡ 193 -> · 225 */
	226, /* ¬ 194 -> ‚ 226 */
	227, /* √ 195 -> „ 227 */
	228, /* ƒ 196 -> ‰ 228 */
	229, /* ≈ 197 -> Â 229 */
	230, /* ∆ 198 -> Ê 230 */
	231, /* « 199 -> Á 231 */
	232, /* » 200 -> Ë 232 */
	233, /* … 201 -> È 233 */
	234, /*   202 -> Í 234 */
	235, /* À 203 -> Î 235 */
	236, /* Ã 204 -> Ï 236 */
	237, /* Õ 205 -> Ì 237 */
	238, /* Œ 206 -> Ó 238 */
	239, /* œ 207 -> Ô 239 */
	240, /* – 208 ->  240 */
	241, /* — 209 -> Ò 241 */
	242, /* “ 210 -> Ú 242 */
	243, /* ” 211 -> Û 243 */
	244, /* ‘ 212 -> Ù 244 */
	245, /* ’ 213 -> ı 245 */
	246, /* ÷ 214 -> ˆ 246 */
	247, /* ◊ 215 -> ˜ 247 */
	248, /* ÿ 216 -> ¯ 248 */
	249, /* Ÿ 217 -> ˘ 249 */
	250, /* ⁄ 218 -> ˙ 250 */
	251, /* € 219 -> ˚ 251 */
	252, /* ‹ 220 -> ¸ 252 */
	253, /* › 221 -> ˝ 253 */
	254, /* ﬁ 222 -> ˛ 254 */
	255, /* ﬂ 223 -> ˇ 255 */
	224, /* ‡ 224 -> ‡ 224 */
	225, /* · 225 -> · 225 */
	226, /* ‚ 226 -> ‚ 226 */
	227, /* „ 227 -> „ 227 */
	228, /* ‰ 228 -> ‰ 228 */
	229, /* Â 229 -> Â 229 */
	230, /* Ê 230 -> Ê 230 */
	231, /* Á 231 -> Á 231 */
	232, /* Ë 232 -> Ë 232 */
	233, /* È 233 -> È 233 */
	234, /* Í 234 -> Í 234 */
	235, /* Î 235 -> Î 235 */
	236, /* Ï 236 -> Ï 236 */
	237, /* Ì 237 -> Ì 237 */
	238, /* Ó 238 -> Ó 238 */
	239, /* Ô 239 -> Ô 239 */
	240, /*  240 ->  240 */
	241, /* Ò 241 -> Ò 241 */
	242, /* Ú 242 -> Ú 242 */
	243, /* Û 243 -> Û 243 */
	244, /* Ù 244 -> Ù 244 */
	245, /* ı 245 -> ı 245 */
	246, /* ˆ 246 -> ˆ 246 */
	247, /* ˜ 247 -> ˜ 247 */
	248, /* ¯ 248 -> ¯ 248 */
	249, /* ˘ 249 -> ˘ 249 */
	250, /* ˙ 250 -> ˙ 250 */
	251, /* ˚ 251 -> ˚ 251 */
	252, /* ¸ 252 -> ¸ 252 */
	253, /* ˝ 253 -> ˝ 253 */
	254, /* ˛ 254 -> ˛ 254 */
	255  /* ˇ 255 -> ˇ 255 */
};

static const BYTE ToLowerConversionTbl [ LOWERCASE_LEN ] = {
	  0, /*     0 ->     0 */
	  1, /*     1 ->     1 */
	  2, /*     2 ->     2 */
	  3, /*     3 ->     3 */
	  4, /*     4 ->     4 */
	  5, /*     5 ->     5 */
	  6, /*     6 ->     6 */
	  7, /*     7 ->     7 */
	  8, /*     8 ->     8 */
	  9, /*     9 ->     9 */
	 10, /*    10 ->    10 */
	 11, /*    11 ->    11 */
	 12, /*    12 ->    12 */
	 13, /*    13 ->    13 */
	 14, /*    14 ->    14 */
	 15, /*    15 ->    15 */
	 16, /*    16 ->    16 */
	 17, /*    17 ->    17 */
	 18, /*    18 ->    18 */
	 19, /*    19 ->    19 */
	 20, /*    20 ->    20 */
	 21, /*    21 ->    21 */
	 22, /*    22 ->    22 */
	 23, /*    23 ->    23 */
	 24, /*    24 ->    24 */
	 25, /*    25 ->    25 */
	 26, /*    26 ->    26 */
	 27, /*    27 ->    27 */
	 28, /*    28 ->    28 */
	 29, /*    29 ->    29 */
	 30, /*    30 ->    30 */
	 31, /*    31 ->    31 */
	 32, /*    32 ->    32 */
	 33, /* !  33 -> !  33 */
	 34, /* "  34 -> "  34 */
	 35, /* #  35 -> #  35 */
	 36, /* $  36 -> $  36 */
	 37, /* %  37 -> %  37 */
	 38, /* &  38 -> &  38 */
	 39, /* '  39 -> '  39 */
	 40, /* (  40 -> (  40 */
	 41, /* )  41 -> )  41 */
	 42, /* *  42 -> *  42 */
	 43, /* +  43 -> +  43 */
	 44, /* ,  44 -> ,  44 */
	 45, /* -  45 -> -  45 */
	 46, /* .  46 -> .  46 */
	 47, /* /  47 -> /  47 */
	 48, /* 0  48 -> 0  48 */
	 49, /* 1  49 -> 1  49 */
	 50, /* 2  50 -> 2  50 */
	 51, /* 3  51 -> 3  51 */
	 52, /* 4  52 -> 4  52 */
	 53, /* 5  53 -> 5  53 */
	 54, /* 6  54 -> 6  54 */
	 55, /* 7  55 -> 7  55 */
	 56, /* 8  56 -> 8  56 */
	 57, /* 9  57 -> 9  57 */
	 58, /* :  58 -> :  58 */
	 59, /* ;  59 -> ;  59 */
	 60, /* <  60 -> <  60 */
	 61, /* =  61 -> =  61 */
	 62, /* >  62 -> >  62 */
	 63, /* ?  63 -> ?  63 */
	 64, /* @  64 -> @  64 */
	 97, /* A  65 -> a  97 */
	 98, /* B  66 -> b  98 */
	 99, /* C  67 -> c  99 */
	100, /* D  68 -> d 100 */
	101, /* E  69 -> e 101 */
	102, /* F  70 -> f 102 */
	103, /* G  71 -> g 103 */
	104, /* H  72 -> h 104 */
	105, /* I  73 -> i 105 */
	106, /* J  74 -> j 106 */
	107, /* K  75 -> k 107 */
	108, /* L  76 -> l 108 */
	109, /* M  77 -> m 109 */
	110, /* N  78 -> n 110 */
	111, /* O  79 -> o 111 */
	112, /* P  80 -> p 112 */
	113, /* Q  81 -> q 113 */
	114, /* R  82 -> r 114 */
	115, /* S  83 -> s 115 */
	116, /* T  84 -> t 116 */
	117, /* U  85 -> u 117 */
	118, /* V  86 -> v 118 */
	119, /* W  87 -> w 119 */
	120, /* X  88 -> x 120 */
	121, /* Y  89 -> y 121 */
	122, /* Z  90 -> z 122 */
	 91, /* [  91 -> [  91 */
	 92, /* \  92 -> \  92 */
	 93, /* ]  93 -> ]  93 */
	 94, /* ^  94 -> ^  94 */
	 95, /* _  95 -> _  95 */
	 96, /* `  96 -> `  96 */
	 97, /* a  97 -> a  97 */
	 98, /* b  98 -> b  98 */
	 99, /* c  99 -> c  99 */
	100, /* d 100 -> d 100 */
	101, /* e 101 -> e 101 */
	102, /* f 102 -> f 102 */
	103, /* g 103 -> g 103 */
	104, /* h 104 -> h 104 */
	105, /* i 105 -> i 105 */
	106, /* j 106 -> j 106 */
	107, /* k 107 -> k 107 */
	108, /* l 108 -> l 108 */
	109, /* m 109 -> m 109 */
	110, /* n 110 -> n 110 */
	111, /* o 111 -> o 111 */
	112, /* p 112 -> p 112 */
	113, /* q 113 -> q 113 */
	114, /* r 114 -> r 114 */
	115, /* s 115 -> s 115 */
	116, /* t 116 -> t 116 */
	117, /* u 117 -> u 117 */
	118, /* v 118 -> v 118 */
	119, /* w 119 -> w 119 */
	120, /* x 120 -> x 120 */
	121, /* y 121 -> y 121 */
	122, /* z 122 -> z 122 */
	123, /* { 123 -> { 123 */
	124, /* | 124 -> | 124 */
	125, /* } 125 -> } 125 */
	126, /* ~ 126 -> ~ 126 */
	127, /*  127 ->  127 */
	128, /* Ä 128 -> Ä 128 */
	129, /* Å 129 -> Å 129 */
	130, /* Ç 130 -> Ç 130 */
	131, /* É 131 -> É 131 */
	132, /* Ñ 132 -> Ñ 132 */
	133, /* Ö 133 -> Ö 133 */
	134, /* Ü 134 -> Ü 134 */
	135, /* á 135 -> á 135 */
	136, /* à 136 -> à 136 */
	137, /* â 137 -> â 137 */
	138, /* ä 138 -> ä 138 */
	139, /* ã 139 -> ã 139 */
	140, /* å 140 -> å 140 */
	141, /* ç 141 -> ç 141 */
	142, /* é 142 -> é 142 */
	143, /* è 143 -> è 143 */
	144, /* ê 144 -> ê 144 */
	145, /* ë 145 -> ë 145 */
	146, /* í 146 -> í 146 */
	147, /* ì 147 -> ì 147 */
	148, /* î 148 -> î 148 */
	149, /* ï 149 -> ï 149 */
	150, /* ñ 150 -> ñ 150 */
	151, /* ó 151 -> ó 151 */
	152, /* ò 152 -> ò 152 */
	153, /* ô 153 -> ô 153 */
	154, /* ö 154 -> ö 154 */
	155, /* õ 155 -> õ 155 */
	156, /* ú 156 -> ú 156 */
	157, /* ù 157 -> ù 157 */
	158, /* û 158 -> û 158 */
	159, /* ü 159 -> ü 159 */
	160, /* † 160 -> † 160 */
	161, /* ° 161 -> ° 161 */
	162, /* ¢ 162 -> ¢ 162 */
	163, /* £ 163 -> £ 163 */
	164, /* § 164 -> § 164 */
	165, /* • 165 -> • 165 */
	166, /* ¶ 166 -> ¶ 166 */
	167, /* ß 167 -> ß 167 */
	168, /* ® 168 -> ® 168 */
	169, /* © 169 -> © 169 */
	170, /* ™ 170 -> ™ 170 */
	171, /* ´ 171 -> ´ 171 */
	172, /* ¨ 172 -> ¨ 172 */
	173, /* ≠ 173 -> ≠ 173 */
	174, /* Æ 174 -> Æ 174 */
	175, /* Ø 175 -> Ø 175 */
	176, /* ∞ 176 -> ∞ 176 */
	177, /* ± 177 -> ± 177 */
	178, /* ≤ 178 -> ≤ 178 */
	163, /* ≥ 179 -> £ 163 */
	180, /* ¥ 180 -> ¥ 180 */
	181, /* µ 181 -> µ 181 */
	182, /* ∂ 182 -> ∂ 182 */
	183, /* ∑ 183 -> ∑ 183 */
	184, /* ∏ 184 -> ∏ 184 */
	185, /* π 185 -> π 185 */
	186, /* ∫ 186 -> ∫ 186 */
	187, /* ª 187 -> ª 187 */
	188, /* º 188 -> º 188 */
	189, /* Ω 189 -> Ω 189 */
	190, /* æ 190 -> æ 190 */
	191, /* ø 191 -> ø 191 */
	192, /* ¿ 192 -> ¿ 192 */
	193, /* ¡ 193 -> ¡ 193 */
	194, /* ¬ 194 -> ¬ 194 */
	195, /* √ 195 -> √ 195 */
	196, /* ƒ 196 -> ƒ 196 */
	197, /* ≈ 197 -> ≈ 197 */
	198, /* ∆ 198 -> ∆ 198 */
	199, /* « 199 -> « 199 */
	200, /* » 200 -> » 200 */
	201, /* … 201 -> … 201 */
	202, /*   202 ->   202 */
	203, /* À 203 -> À 203 */
	204, /* Ã 204 -> Ã 204 */
	205, /* Õ 205 -> Õ 205 */
	206, /* Œ 206 -> Œ 206 */
	207, /* œ 207 -> œ 207 */
	208, /* – 208 -> – 208 */
	209, /* — 209 -> — 209 */
	210, /* “ 210 -> “ 210 */
	211, /* ” 211 -> ” 211 */
	212, /* ‘ 212 -> ‘ 212 */
	213, /* ’ 213 -> ’ 213 */
	214, /* ÷ 214 -> ÷ 214 */
	215, /* ◊ 215 -> ◊ 215 */
	216, /* ÿ 216 -> ÿ 216 */
	217, /* Ÿ 217 -> Ÿ 217 */
	218, /* ⁄ 218 -> ⁄ 218 */
	219, /* € 219 -> € 219 */
	220, /* ‹ 220 -> ‹ 220 */
	221, /* › 221 -> › 221 */
	222, /* ﬁ 222 -> ﬁ 222 */
	223, /* ﬂ 223 -> ﬂ 223 */
	192, /* ‡ 224 -> ¿ 192 */
	193, /* · 225 -> ¡ 193 */
	194, /* ‚ 226 -> ¬ 194 */
	195, /* „ 227 -> √ 195 */
	196, /* ‰ 228 -> ƒ 196 */
	197, /* Â 229 -> ≈ 197 */
	198, /* Ê 230 -> ∆ 198 */
	199, /* Á 231 -> « 199 */
	200, /* Ë 232 -> » 200 */
	201, /* È 233 -> … 201 */
	202, /* Í 234 ->   202 */
	203, /* Î 235 -> À 203 */
	204, /* Ï 236 -> Ã 204 */
	205, /* Ì 237 -> Õ 205 */
	206, /* Ó 238 -> Œ 206 */
	207, /* Ô 239 -> œ 207 */
	208, /*  240 -> – 208 */
	209, /* Ò 241 -> — 209 */
	210, /* Ú 242 -> “ 210 */
	211, /* Û 243 -> ” 211 */
	212, /* Ù 244 -> ‘ 212 */
	213, /* ı 245 -> ’ 213 */
	214, /* ˆ 246 -> ÷ 214 */
	215, /* ˜ 247 -> ◊ 215 */
	216, /* ¯ 248 -> ÿ 216 */
	217, /* ˘ 249 -> Ÿ 217 */
	218, /* ˙ 250 -> ⁄ 218 */
	219, /* ˚ 251 -> € 219 */
	220, /* ¸ 252 -> ‹ 220 */
	221, /* ˝ 253 -> › 221 */
	222, /* ˛ 254 -> ﬁ 222 */
	223  /* ˇ 255 -> ﬂ 223 */
};

static const SortOrderTblEntry NoCaseOrderTbl [ NOCASESORT_LEN ] = {
	{ FIRST_PRIMARY +   0, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*   0   */
	{ FIRST_PRIMARY +   1, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*   1   */
	{ FIRST_PRIMARY +   2, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*   2   */
	{ FIRST_PRIMARY +   3, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*   3   */
	{ FIRST_PRIMARY +   4, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*   4   */
	{ FIRST_PRIMARY +   5, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*   5   */
	{ FIRST_PRIMARY +   6, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*   6   */
	{ FIRST_PRIMARY +   7, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*   7   */
	{ FIRST_PRIMARY +   8, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*   8   */
	{ FIRST_PRIMARY +  32, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*   9   */
	{ FIRST_PRIMARY +  33, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  10   */
	{ FIRST_PRIMARY +  34, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  11   */
	{ FIRST_PRIMARY +  35, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  12   */
	{ FIRST_PRIMARY +  36, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  13   */
	{ FIRST_PRIMARY +   9, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  14   */
	{ FIRST_PRIMARY +  10, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  15   */
	{ FIRST_PRIMARY +  11, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  16   */
	{ FIRST_PRIMARY +  12, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  17   */
	{ FIRST_PRIMARY +  13, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  18   */
	{ FIRST_PRIMARY +  14, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  19   */
	{ FIRST_PRIMARY +  15, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  20   */
	{ FIRST_PRIMARY +  16, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  21   */
	{ FIRST_PRIMARY +  17, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  22   */
	{ FIRST_PRIMARY +  18, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  23   */
	{ FIRST_PRIMARY +  19, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  24   */
	{ FIRST_PRIMARY +  20, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  25   */
	{ FIRST_PRIMARY +  21, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  26   */
	{ FIRST_PRIMARY +  22, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  27   */
	{ FIRST_PRIMARY +  23, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  28   */
	{ FIRST_PRIMARY +  24, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  29   */
	{ FIRST_PRIMARY +  25, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  30   */
	{ FIRST_PRIMARY +  26, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  31   */
	{ FIRST_PRIMARY +  30, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  32   */
	{ FIRST_PRIMARY +  37, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  33 ! */
	{ FIRST_PRIMARY +  38, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  34 " */
	{ FIRST_PRIMARY +  39, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  35 # */
	{ FIRST_PRIMARY +  40, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  36 $ */
	{ FIRST_PRIMARY +  41, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  37 % */
	{ FIRST_PRIMARY +  42, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  38 & */
	{ FIRST_PRIMARY +  28, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  39 ' */
	{ FIRST_PRIMARY +  43, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  40 ( */
	{ FIRST_PRIMARY +  44, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  41 ) */
	{ FIRST_PRIMARY +  45, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  42 * */
	{ FIRST_PRIMARY +  63, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  43 + */
	{ FIRST_PRIMARY +  46, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  44 , */
	{ FIRST_PRIMARY +  29, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  45 - */
	{ FIRST_PRIMARY +  47, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  46 . */
	{ FIRST_PRIMARY +  48, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  47 / */
	{ FIRST_PRIMARY +  92, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  48 0 */
	{ FIRST_PRIMARY +  93, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  49 1 */
	{ FIRST_PRIMARY +  94, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /*  50 2 */
	{ FIRST_PRIMARY +  95, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  51 3 */
	{ FIRST_PRIMARY +  96, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  52 4 */
	{ FIRST_PRIMARY +  97, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  53 5 */
	{ FIRST_PRIMARY +  98, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  54 6 */
	{ FIRST_PRIMARY +  99, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  55 7 */
	{ FIRST_PRIMARY + 100, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  56 8 */
	{ FIRST_PRIMARY + 101, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  57 9 */
	{ FIRST_PRIMARY +  49, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  58 : */
	{ FIRST_PRIMARY +  50, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  59 ; */
	{ FIRST_PRIMARY +  65, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  60 < */
	{ FIRST_PRIMARY +  66, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  61 = */
	{ FIRST_PRIMARY +  67, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  62 > */
	{ FIRST_PRIMARY +  51, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  63 ? */
	{ FIRST_PRIMARY +  52, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  64 @ */
	{ FIRST_PRIMARY + 102, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  65 A */
	{ FIRST_PRIMARY + 103, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  66 B */
	{ FIRST_PRIMARY + 104, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  67 C */
	{ FIRST_PRIMARY + 105, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  68 D */
	{ FIRST_PRIMARY + 106, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  69 E */
	{ FIRST_PRIMARY + 107, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  70 F */
	{ FIRST_PRIMARY + 108, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  71 G */
	{ FIRST_PRIMARY + 109, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  72 H */
	{ FIRST_PRIMARY + 110, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  73 I */
	{ FIRST_PRIMARY + 111, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  74 J */
	{ FIRST_PRIMARY + 112, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  75 K */
	{ FIRST_PRIMARY + 113, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  76 L */
	{ FIRST_PRIMARY + 114, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  77 M */
	{ FIRST_PRIMARY + 115, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  78 N */
	{ FIRST_PRIMARY + 116, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  79 O */
	{ FIRST_PRIMARY + 117, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  80 P */
	{ FIRST_PRIMARY + 118, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  81 Q */
	{ FIRST_PRIMARY + 119, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  82 R */
	{ FIRST_PRIMARY + 120, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  83 S */
	{ FIRST_PRIMARY + 121, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  84 T */
	{ FIRST_PRIMARY + 122, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  85 U */
	{ FIRST_PRIMARY + 123, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  86 V */
	{ FIRST_PRIMARY + 124, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  87 W */
	{ FIRST_PRIMARY + 125, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  88 X */
	{ FIRST_PRIMARY + 126, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  89 Y */
	{ FIRST_PRIMARY + 127, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /*  90 Z */
	{ FIRST_PRIMARY +  53, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  91 [ */
	{ FIRST_PRIMARY +  54, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  92 \ */
	{ FIRST_PRIMARY +  55, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  93 ] */
	{ FIRST_PRIMARY +  56, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  94 ^ */
	{ FIRST_PRIMARY +  57, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  95 _ */
	{ FIRST_PRIMARY +  58, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /*  96 ` */
	{ FIRST_PRIMARY + 102, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /*  97 a */
	{ FIRST_PRIMARY + 103, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /*  98 b */
	{ FIRST_PRIMARY + 104, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /*  99 c */
	{ FIRST_PRIMARY + 105, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 100 d */
	{ FIRST_PRIMARY + 106, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 101 e */
	{ FIRST_PRIMARY + 107, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 102 f */
	{ FIRST_PRIMARY + 108, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 103 g */
	{ FIRST_PRIMARY + 109, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 104 h */
	{ FIRST_PRIMARY + 110, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 105 i */
	{ FIRST_PRIMARY + 111, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 106 j */
	{ FIRST_PRIMARY + 112, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 107 k */
	{ FIRST_PRIMARY + 113, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 108 l */
	{ FIRST_PRIMARY + 114, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 109 m */
	{ FIRST_PRIMARY + 115, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 110 n */
	{ FIRST_PRIMARY + 116, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 111 o */
	{ FIRST_PRIMARY + 117, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 112 p */
	{ FIRST_PRIMARY + 118, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 113 q */
	{ FIRST_PRIMARY + 119, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 114 r */
	{ FIRST_PRIMARY + 120, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 115 s */
	{ FIRST_PRIMARY + 121, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 116 t */
	{ FIRST_PRIMARY + 122, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 117 u */
	{ FIRST_PRIMARY + 123, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 118 v */
	{ FIRST_PRIMARY + 124, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 119 w */
	{ FIRST_PRIMARY + 125, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 120 x */
	{ FIRST_PRIMARY + 126, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 121 y */
	{ FIRST_PRIMARY + 127, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 122 z */
	{ FIRST_PRIMARY +  59, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 123 { */
	{ FIRST_PRIMARY +  60, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 124 | */
	{ FIRST_PRIMARY +  61, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 125 } */
	{ FIRST_PRIMARY +  62, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 126 ~ */
	{ FIRST_PRIMARY +  27, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 127  */
	{ FIRST_PRIMARY +  76, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 128 Ä */
	{ FIRST_PRIMARY +  77, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 129 Å */
	{ FIRST_PRIMARY +  78, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 130 Ç */
	{ FIRST_PRIMARY +  79, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 131 É */
	{ FIRST_PRIMARY +  80, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 132 Ñ */
	{ FIRST_PRIMARY +  81, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 133 Ö */
	{ FIRST_PRIMARY +  82, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 134 Ü */
	{ FIRST_PRIMARY +  83, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 135 á */
	{ FIRST_PRIMARY +  84, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 136 à */
	{ FIRST_PRIMARY +  85, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 137 â */
	{ FIRST_PRIMARY +  86, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 138 ä */
	{ FIRST_PRIMARY +  87, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 139 ã */
	{ FIRST_PRIMARY +  87, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 140 å */
	{ FIRST_PRIMARY +  88, FIRST_SECONDARY + 0, NULL_TERTIARY, 0, 0 }, /* 141 ç */
	{ FIRST_PRIMARY +  87, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 142 é */
	{ FIRST_PRIMARY +  87, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 143 è */
	{ FIRST_PRIMARY +  88, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 144 ê */
	{ FIRST_PRIMARY +  88, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 145 ë */
	{ FIRST_PRIMARY +  88, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 146 í */
	{ FIRST_PRIMARY +  73, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 147 ì */
	{ FIRST_PRIMARY +  75, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 148 î */
	{ FIRST_PRIMARY +  64, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 149 ï */
	{ FIRST_PRIMARY +  69, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 150 ñ */
	{ FIRST_PRIMARY +  70, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 151 ó */
	{ FIRST_PRIMARY +  71, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 152 ò */
	{ FIRST_PRIMARY +  72, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 153 ô */
	{ FIRST_PRIMARY +  31, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 154 ö */
	{ FIRST_PRIMARY +  74, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 155 õ */
	{ FIRST_PRIMARY +  90, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 156 ú */
	{ FIRST_PRIMARY +  94, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 157 ù */
	{ FIRST_PRIMARY +  91, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 158 û */
	{ FIRST_PRIMARY +  68, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 159 ü */
	{ FIRST_PRIMARY +  76, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 160 † */
	{ FIRST_PRIMARY +  77, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 161 ° */
	{ FIRST_PRIMARY +  78, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 162 ¢ */
	{ FIRST_PRIMARY + 133, FIRST_SECONDARY + 1, FIRST_TERTIARY + 0, 0, 0 }, /* 163 £ */
	{ FIRST_PRIMARY +  78, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 164 § */
	{ FIRST_PRIMARY +  78, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 165 • */
	{ FIRST_PRIMARY +  79, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 166 ¶ */
	{ FIRST_PRIMARY +  79, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 167 ß */
	{ FIRST_PRIMARY +  79, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 168 ® */
	{ FIRST_PRIMARY +  80, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 169 © */
	{ FIRST_PRIMARY +  80, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 170 ™ */
	{ FIRST_PRIMARY +  80, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 171 ´ */
	{ FIRST_PRIMARY +  81, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 172 ¨ */
	{ FIRST_PRIMARY +  81, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 173 ≠ */
	{ FIRST_PRIMARY +  81, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 174 Æ */
	{ FIRST_PRIMARY +  82, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 175 Ø */
	{ FIRST_PRIMARY +  82, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 176 ∞ */
	{ FIRST_PRIMARY +  82, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 177 ± */
	{ FIRST_PRIMARY +  83, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 178 ≤ */
	{ FIRST_PRIMARY + 133, FIRST_SECONDARY + 1, FIRST_TERTIARY + 1, 0, 0 }, /* 179 ≥ */
	{ FIRST_PRIMARY +  83, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 180 ¥ */
	{ FIRST_PRIMARY +  83, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 181 µ */
	{ FIRST_PRIMARY +  84, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 182 ∂ */
	{ FIRST_PRIMARY +  84, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 183 ∑ */
	{ FIRST_PRIMARY +  84, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 184 ∏ */
	{ FIRST_PRIMARY +  85, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 185 π */
	{ FIRST_PRIMARY +  85, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 186 ∫ */
	{ FIRST_PRIMARY +  85, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 187 ª */
	{ FIRST_PRIMARY +  86, FIRST_SECONDARY + 1, NULL_TERTIARY, 0, 0 }, /* 188 º */
	{ FIRST_PRIMARY +  86, FIRST_SECONDARY + 2, NULL_TERTIARY, 0, 0 }, /* 189 Ω */
	{ FIRST_PRIMARY +  86, FIRST_SECONDARY + 3, NULL_TERTIARY, 0, 0 }, /* 190 æ */
	{ FIRST_PRIMARY +  89, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 191 ø */
	{ FIRST_PRIMARY + 158, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 192 ¿ */
	{ FIRST_PRIMARY + 128, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 193 ¡ */
	{ FIRST_PRIMARY + 129, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 194 ¬ */
	{ FIRST_PRIMARY + 150, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 195 √ */
	{ FIRST_PRIMARY + 132, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 196 ƒ */
	{ FIRST_PRIMARY + 133, FIRST_SECONDARY + 0, FIRST_TERTIARY + 0, 0, 0 }, /* 197 ≈ */
	{ FIRST_PRIMARY + 148, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 198 ∆ */
	{ FIRST_PRIMARY + 131, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 199 « */
	{ FIRST_PRIMARY + 149, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 200 » */
	{ FIRST_PRIMARY + 136, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 201 … */
	{ FIRST_PRIMARY + 137, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 202   */
	{ FIRST_PRIMARY + 138, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 203 À */
	{ FIRST_PRIMARY + 139, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 204 Ã */
	{ FIRST_PRIMARY + 140, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 205 Õ */
	{ FIRST_PRIMARY + 141, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 206 Œ */
	{ FIRST_PRIMARY + 142, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 207 œ */
	{ FIRST_PRIMARY + 143, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 208 – */
	{ FIRST_PRIMARY + 159, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 209 — */
	{ FIRST_PRIMARY + 144, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 210 “ */
	{ FIRST_PRIMARY + 145, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 211 ” */
	{ FIRST_PRIMARY + 146, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 212 ‘ */
	{ FIRST_PRIMARY + 147, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 213 ’ */
	{ FIRST_PRIMARY + 134, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 214 ÷ */
	{ FIRST_PRIMARY + 130, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 215 ◊ */
	{ FIRST_PRIMARY + 156, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 216 ÿ */
	{ FIRST_PRIMARY + 155, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 217 Ÿ */
	{ FIRST_PRIMARY + 135, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 218 ⁄ */
	{ FIRST_PRIMARY + 152, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 219 € */
	{ FIRST_PRIMARY + 157, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 220 ‹ */
	{ FIRST_PRIMARY + 153, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 221 › */
	{ FIRST_PRIMARY + 151, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 222 ﬁ */
	{ FIRST_PRIMARY + 154, NULL_SECONDARY, FIRST_TERTIARY + 0, 0, 0 }, /* 223 ﬂ */
	{ FIRST_PRIMARY + 158, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 224 ‡ */
	{ FIRST_PRIMARY + 128, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 225 · */
	{ FIRST_PRIMARY + 129, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 226 ‚ */
	{ FIRST_PRIMARY + 150, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 227 „ */
	{ FIRST_PRIMARY + 132, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 228 ‰ */
	{ FIRST_PRIMARY + 133, FIRST_SECONDARY + 0, FIRST_TERTIARY + 1, 0, 0 }, /* 229 Â */
	{ FIRST_PRIMARY + 148, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 230 Ê */
	{ FIRST_PRIMARY + 131, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 231 Á */
	{ FIRST_PRIMARY + 149, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 232 Ë */
	{ FIRST_PRIMARY + 136, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 233 È */
	{ FIRST_PRIMARY + 137, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 234 Í */
	{ FIRST_PRIMARY + 138, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 235 Î */
	{ FIRST_PRIMARY + 139, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 236 Ï */
	{ FIRST_PRIMARY + 140, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 237 Ì */
	{ FIRST_PRIMARY + 141, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 238 Ó */
	{ FIRST_PRIMARY + 142, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 239 Ô */
	{ FIRST_PRIMARY + 143, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 240  */
	{ FIRST_PRIMARY + 159, NULL_SECONDARY, NULL_TERTIARY, 0, 0 }, /* 241 Ò */
	{ FIRST_PRIMARY + 144, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 242 Ú */
	{ FIRST_PRIMARY + 145, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 243 Û */
	{ FIRST_PRIMARY + 146, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 244 Ù */
	{ FIRST_PRIMARY + 147, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 245 ı */
	{ FIRST_PRIMARY + 134, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 246 ˆ */
	{ FIRST_PRIMARY + 130, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 247 ˜ */
	{ FIRST_PRIMARY + 156, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 248 ¯ */
	{ FIRST_PRIMARY + 155, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 249 ˘ */
	{ FIRST_PRIMARY + 135, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 250 ˙ */
	{ FIRST_PRIMARY + 152, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 251 ˚ */
	{ FIRST_PRIMARY + 157, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 252 ¸ */
	{ FIRST_PRIMARY + 153, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 253 ˝ */
	{ FIRST_PRIMARY + 151, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }, /* 254 ˛ */
	{ FIRST_PRIMARY + 154, NULL_SECONDARY, FIRST_TERTIARY + 1, 0, 0 }  /* 255 ˇ */
};

static const ExpandChar ExpansionTbl [ NUM_EXPAND_CHARS + 1 ] = {
{ 0, 0, 0 } /* END OF TABLE */
};

static const CompressPair CompressTbl [ NUM_COMPRESS_CHARS + 1 ] = {
{ {0, 0}, {   0,   0,   0,   0,   0 }, {   0,   0,   0,   0,   0 } } /*END OF TABLE */
};
