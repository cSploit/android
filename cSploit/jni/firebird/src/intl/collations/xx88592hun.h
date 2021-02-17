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
 * All Rights Reserved.
 * Contributor(s): Sandor Szollosi <ssani@freemail.hu>, Gabor Boros
 */
/*  Language Driver/API version 1.1 */

const int NUM_EXPAND_CHARS		= 0;
const int NUM_COMPRESS_CHARS	= 24;
const int LOWERCASE_LEN			= 256;
const int UPPERCASE_LEN			= 256;
const int NOCASESORT_LEN		= 256;
const int LDRV_TIEBREAK			= LOCAL_EXPAND;

//const int MAX_NCO_PRIMARY		= 197;
const int MAX_NCO_SECONDARY		= 2;
const int MAX_NCO_TERTIARY		= 0;
//const int MAX_NCO_IGNORE		= 0;
const int NULL_SECONDARY		= 0;
const int NULL_TERTIARY			= 0;
//const int FIRST_IGNORE			= 1;
const int FIRST_TERTIARY		= 1;
const int FIRST_SECONDARY		= (FIRST_TERTIARY + MAX_NCO_TERTIARY + 1);
const int FIRST_PRIMARY			= (FIRST_SECONDARY + MAX_NCO_SECONDARY + 1);

static const BYTE ToUpperConversionTbl [ UPPERCASE_LEN ] = {

    0, /* 0 -> 0 */
    1, /* 1 -> 1 */
    2, /* 2 -> 2 */
    3, /* 3 -> 3 */
    4, /* 4 -> 4 */
    5, /* 5 -> 5 */
    6, /* 6 -> 6 */
    7, /* 7 -> 7 */
    8, /* 8 -> 8 */
    9, /* 9 -> 9 */
    10, /* 10 -> 10 */
    11, /* 11 -> 11 */
    12, /* 12 -> 12 */
    13, /* 13 -> 13 */
    14, /* 14 -> 14 */
    15, /* 15 -> 15 */
    16, /* 16 -> 16 */
    17, /* 17 -> 17 */
    18, /* 18 -> 18 */
    19, /* 19 -> 19 */
    20, /* 20 -> 20 */
    21, /* 21 -> 21 */
    22, /* 22 -> 22 */
    23, /* 23 -> 23 */
    24, /* 24 -> 24 */
    25, /* 25 -> 25 */
    26, /* 26 -> 26 */
    27, /* 27 -> 27 */
    28, /* 28 -> 28 */
    29, /* 29 -> 29 */
    30, /* 30 -> 30 */
    31, /* 31 -> 31 */
    32, /* 32 -> 32 */
    33, /* 33 -> 33 ( ! -> ! ) */
    34, /* 34 -> 34 ( " -> " ) */
    35, /* 35 -> 35 ( # -> # ) */
    36, /* 36 -> 36 ( $ -> $ ) */
    37, /* 37 -> 37 ( % -> % ) */
    38, /* 38 -> 38 ( & -> & ) */
    39, /* 39 -> 39 ( ' -> ' ) */
    40, /* 40 -> 40 ( ( -> ( ) */
    41, /* 41 -> 41 ( ) -> ) ) */
    42, /* 42 -> 42 ( * -> * ) */
    43, /* 43 -> 43 ( + -> + ) */
    44, /* 44 -> 44 ( , -> , ) */
    45, /* 45 -> 45 ( - -> - ) */
    46, /* 46 -> 46 ( . -> . ) */
    47, /* 47 -> 47 ( / -> / ) */
    48, /* 48 -> 48 ( 0 -> 0 ) */
    49, /* 49 -> 49 ( 1 -> 1 ) */
    50, /* 50 -> 50 ( 2 -> 2 ) */
    51, /* 51 -> 51 ( 3 -> 3 ) */
    52, /* 52 -> 52 ( 4 -> 4 ) */
    53, /* 53 -> 53 ( 5 -> 5 ) */
    54, /* 54 -> 54 ( 6 -> 6 ) */
    55, /* 55 -> 55 ( 7 -> 7 ) */
    56, /* 56 -> 56 ( 8 -> 8 ) */
    57, /* 57 -> 57 ( 9 -> 9 ) */
    58, /* 58 -> 58 ( : -> : ) */
    59, /* 59 -> 59 ( ; -> ; ) */
    60, /* 60 -> 60 ( < -> < ) */
    61, /* 61 -> 61 ( = -> = ) */
    62, /* 62 -> 62 ( > -> > ) */
    63, /* 63 -> 63 ( ? -> ? ) */
    64, /* 64 -> 64 ( @ -> @ ) */
    65, /* 65 -> 65 ( A -> A ) */
    66, /* 66 -> 66 ( B -> B ) */
    67, /* 67 -> 67 ( C -> C ) */
    68, /* 68 -> 68 ( D -> D ) */
    69, /* 69 -> 69 ( E -> E ) */
    70, /* 70 -> 70 ( F -> F ) */
    71, /* 71 -> 71 ( G -> G ) */
    72, /* 72 -> 72 ( H -> H ) */
    73, /* 73 -> 73 ( I -> I ) */
    74, /* 74 -> 74 ( J -> J ) */
    75, /* 75 -> 75 ( K -> K ) */
    76, /* 76 -> 76 ( L -> L ) */
    77, /* 77 -> 77 ( M -> M ) */
    78, /* 78 -> 78 ( N -> N ) */
    79, /* 79 -> 79 ( O -> O ) */
    80, /* 80 -> 80 ( P -> P ) */
    81, /* 81 -> 81 ( Q -> Q ) */
    82, /* 82 -> 82 ( R -> R ) */
    83, /* 83 -> 83 ( S -> S ) */
    84, /* 84 -> 84 ( T -> T ) */
    85, /* 85 -> 85 ( U -> U ) */
    86, /* 86 -> 86 ( V -> V ) */
    87, /* 87 -> 87 ( W -> W ) */
    88, /* 88 -> 88 ( X -> X ) */
    89, /* 89 -> 89 ( Y -> Y ) */
    90, /* 90 -> 90 ( Z -> Z ) */
    91, /* 91 -> 91 ( [ -> [ ) */
    92, /* 92 -> 92 ( \ -> \ ) */
    93, /* 93 -> 93 ( ] -> ] ) */
    94, /* 94 -> 94 ( ^ -> ^ ) */
    95, /* 95 -> 95 ( _ -> _ ) */
    96, /* 96 -> 96 ( ` -> ` ) */
    65, /* 97 -> 65 ( a -> A ) */
    66, /* 98 -> 66 ( b -> B ) */
    67, /* 99 -> 67 ( c -> C ) */
    68, /* 100 -> 68    ( d -> D ) */
    69, /* 101 -> 69    ( e -> E ) */
    70, /* 102 -> 70    ( f -> F ) */
    71, /* 103 -> 71    ( g -> G ) */
    72, /* 104 -> 72    ( h -> H ) */
    73, /* 105 -> 73    ( i -> I ) */
    74, /* 106 -> 74    ( j -> J ) */
    75, /* 107 -> 75    ( k -> K ) */
    76, /* 108 -> 76    ( l -> L ) */
    77, /* 109 -> 77    ( m -> M ) */
    78, /* 110 -> 78    ( n -> N ) */
    79, /* 111 -> 79    ( o -> O ) */
    80, /* 112 -> 80    ( p -> P ) */
    81, /* 113 -> 81    ( q -> Q ) */
    82, /* 114 -> 82    ( r -> R ) */
    83, /* 115 -> 83    ( s -> S ) */
    84, /* 116 -> 84    ( t -> T ) */
    85, /* 117 -> 85    ( u -> U ) */
    86, /* 118 -> 86    ( v -> V ) */
    87, /* 119 -> 87    ( w -> W ) */
    88, /* 120 -> 88    ( x -> X ) */
    89, /* 121 -> 89    ( y -> Y ) */
    90, /* 122 -> 90    ( z -> Z ) */
    123, /* 123 -> 123  ( { -> { ) */
    124, /* 124 -> 124  ( | -> | ) */
    125, /* 125 -> 125  ( } -> } ) */
    126, /* 126 -> 126  ( ~ -> ~ ) */
    127, /* 127 -> 127 */
    128, /* 128 -> 128 */
    129, /* 129 -> 129 */
    130, /* 130 -> 130 */
    131, /* 131 -> 131 */
    132, /* 132 -> 132 */
    133, /* 133 -> 133 */
    134, /* 134 -> 134 */
    135, /* 135 -> 135 */
    136, /* 136 -> 136 */
    137, /* 137 -> 137 */
    138, /* 138 -> 138 */
    139, /* 139 -> 139 */
    140, /* 140 -> 140 */
    141, /* 141 -> 141 */
    142, /* 142 -> 142 */
    143, /* 143 -> 143 */
    144, /* 144 -> 144 */
    145, /* 145 -> 145 */
    146, /* 146 -> 146 */
    147, /* 147 -> 147 */
    148, /* 148 -> 148 */
    149, /* 149 -> 149 */
    150, /* 150 -> 150 */
    151, /* 151 -> 151 */
    152, /* 152 -> 152 */
    153, /* 153 -> 153 */
    154, /* 154 -> 154 */
    155, /* 155 -> 155 */
    156, /* 156 -> 156 */
    157, /* 157 -> 157 */
    158, /* 158 -> 158 */
    159, /* 159 -> 159  ( Ÿ -> Ÿ ) */
    160, /* 160 -> 160  (   ->   ) */
    161, /* 161 -> 161  ( ¡ -> ¡ ) */
    162, /* 162 -> 162  ( ¢ -> ¢ ) */
    163, /* 163 -> 163  ( £ -> £ ) */
    164, /* 164 -> 164  ( ¤ -> ¤ ) */
    165, /* 165 -> 165  ( ¥ -> ¥ ) */
    166, /* 166 -> 166  ( ¦ -> ¦ ) */
    167, /* 167 -> 167  ( § -> § ) */
    168, /* 168 -> 168  ( ¨ -> ¨ ) */
    169, /* 169 -> 169  ( © -> © ) */
    170, /* 170 -> 170  ( ª -> ª ) */
    171, /* 171 -> 171  ( « -> « ) */
    172, /* 172 -> 172  ( ¬ -> ¬ ) */
    173, /* 173 -> 173  ( ­ -> ­ ) */
    174, /* 174 -> 174  ( ® -> ® ) */
    175, /* 175 -> 175  ( ¯ -> ¯ ) */
    176, /* 176 -> 176  ( ° -> ° ) */
    161, /* 177 -> 161  ( ± -> ¡ ) */
    178, /* 178 -> 178  ( ² -> ² ) */
    163, /* 179 -> 163  ( ³ -> £ ) */
    180, /* 180 -> 180  ( ´ -> ´ ) */
    165, /* 181 -> 165  ( µ -> ¥ ) */
    166, /* 182 -> 166  ( ¶ -> ¦ ) */
    183, /* 183 -> 183  ( · -> · ) */
    184, /* 184 -> 184  ( ¸ -> ¸ ) */
    169, /* 185 -> 169  ( ¹ -> © ) */
    170, /* 186 -> 170  ( º -> ª ) */
    171, /* 187 -> 171  ( » -> « ) */
    172, /* 188 -> 172  ( ¼ -> ¬ ) */
    189, /* 189 -> 189  ( ½ -> ½ ) */
    174, /* 190 -> 174  ( ¾ -> ® ) */
    175, /* 191 -> 175  ( ¿ -> ¯ ) */
    192, /* 192 -> 192  ( À -> À ) */
    193, /* 193 -> 193  ( Á -> Á ) */
    194, /* 194 -> 194  ( Â -> Â ) */
    195, /* 195 -> 195  ( Ã -> Ã ) */
    196, /* 196 -> 196  ( Ä -> Ä ) */
    197, /* 197 -> 197  ( Å -> Å ) */
    198, /* 198 -> 198  ( Æ -> Æ ) */
    199, /* 199 -> 199  ( Ç -> Ç ) */
    200, /* 200 -> 200  ( È -> È ) */
    201, /* 201 -> 201  ( É -> É ) */
    202, /* 202 -> 202  ( Ê -> Ê ) */
    203, /* 203 -> 203  ( Ë -> Ë ) */
    204, /* 204 -> 204  ( Ì -> Ì ) */
    205, /* 205 -> 205  ( Í -> Í ) */
    206, /* 206 -> 206  ( Î -> Î ) */
    207, /* 207 -> 207  ( Ï -> Ï ) */
    208, /* 208 -> 208  ( Ð -> Ð ) */
    209, /* 209 -> 209  ( Ñ -> Ñ ) */
    210, /* 210 -> 210  ( Ò -> Ò ) */
    211, /* 211 -> 211  ( Ó -> Ó ) */
    212, /* 212 -> 212  ( Ô -> Ô ) */
    213, /* 213 -> 213  ( Õ -> Õ ) */
    214, /* 214 -> 214  ( Ö -> Ö ) */
    215, /* 215 -> 215  ( × -> × ) */
    216, /* 216 -> 216  ( Ø -> Ø ) */
    217, /* 217 -> 217  ( Ù -> Ù ) */
    218, /* 218 -> 218  ( Ú -> Ú ) */
    219, /* 219 -> 219  ( Û -> Û ) */
    220, /* 220 -> 220  ( Ü -> Ü ) */
    221, /* 221 -> 221  ( Ý -> Ý ) */
    222, /* 222 -> 222  ( Þ -> Þ ) */
    223, /* 223 -> 223  ( ß -> ß ) */
    192, /* 224 -> 192  ( à -> À ) */
    193, /* 225 -> 193  ( á -> Á ) */
    194, /* 226 -> 194  ( â -> Â ) */
    195, /* 227 -> 195  ( ã -> Ã ) */
    196, /* 228 -> 196  ( ä -> Ä ) */
    197, /* 229 -> 197  ( å -> Å ) */
    198, /* 230 -> 198  ( æ -> Æ ) */
    199, /* 231 -> 199  ( ç -> Ç ) */
    200, /* 232 -> 200  ( è -> È ) */
    201, /* 233 -> 201  ( é -> É ) */
    202, /* 234 -> 202  ( ê -> Ê ) */
    203, /* 235 -> 203  ( ë -> Ë ) */
    204, /* 236 -> 204  ( ì -> Ì ) */
    205, /* 237 -> 205  ( í -> Í ) */
    206, /* 238 -> 206  ( î -> Î ) */
    207, /* 239 -> 207  ( ï -> Ï ) */
    208, /* 240 -> 208  ( ð -> Ð ) */
    209, /* 241 -> 209  ( ñ -> Ñ ) */
    210, /* 242 -> 210  ( ò -> Ò ) */
    211, /* 243 -> 211  ( ó -> Ó ) */
    212, /* 244 -> 212  ( ô -> Ô ) */
    213, /* 245 -> 213  ( õ -> Õ ) */
    214, /* 246 -> 214  ( ö -> Ö ) */
    247, /* 247 -> 247  ( ÷ -> ÷ ) */
    216, /* 248 -> 216  ( ø -> Ø ) */
    217, /* 249 -> 217  ( ù -> Ù ) */
    218, /* 250 -> 218  ( ú -> Ú ) */
    219, /* 251 -> 219  ( û -> Û ) */
    220, /* 252 -> 220  ( ü -> Ü ) */
    221, /* 253 -> 221  ( ý -> Ý ) */
    222, /* 254 -> 222  ( þ -> Þ ) */
    255  /* 255 -> 255  ( ÿ -> ÿ ) */
};

static const BYTE ToLowerConversionTbl [ LOWERCASE_LEN ] = {
    0, /* 0 -> 0 */
    1, /* 1 -> 1 */
    2, /* 2 -> 2 */
    3, /* 3 -> 3 */
    4, /* 4 -> 4 */
    5, /* 5 -> 5 */
    6, /* 6 -> 6 */
    7, /* 7 -> 7 */
    8, /* 8 -> 8 */
    9, /* 9 -> 9 */
    10, /* 10 -> 10 */
    11, /* 11 -> 11 */
    12, /* 12 -> 12 */
    13, /* 13 -> 13 */
    14, /* 14 -> 14 */
    15, /* 15 -> 15 */
    16, /* 16 -> 16 */
    17, /* 17 -> 17 */
    18, /* 18 -> 18 */
    19, /* 19 -> 19 */
    20, /* 20 -> 20 */
    21, /* 21 -> 21 */
    22, /* 22 -> 22 */
    23, /* 23 -> 23 */
    24, /* 24 -> 24 */
    25, /* 25 -> 25 */
    26, /* 26 -> 26 */
    27, /* 27 -> 27 */
    28, /* 28 -> 28 */
    29, /* 29 -> 29 */
    30, /* 30 -> 30 */
    31, /* 31 -> 31 */
    32, /* 32 -> 32 */
    33, /* 33 -> 33 ( ! -> ! ) */
    34, /* 34 -> 34 ( " -> " ) */
    35, /* 35 -> 35 ( # -> # ) */
    36, /* 36 -> 36 ( $ -> $ ) */
    37, /* 37 -> 37 ( % -> % ) */
    38, /* 38 -> 38 ( & -> & ) */
    39, /* 39 -> 39 ( ' -> ' ) */
    40, /* 40 -> 40 ( ( -> ( ) */
    41, /* 41 -> 41 ( ) -> ) ) */
    42, /* 42 -> 42 ( * -> * ) */
    43, /* 43 -> 43 ( + -> + ) */
    44, /* 44 -> 44 ( , -> , ) */
    45, /* 45 -> 45 ( - -> - ) */
    46, /* 46 -> 46 ( . -> . ) */
    47, /* 47 -> 47 ( / -> / ) */
    48, /* 48 -> 48 ( 0 -> 0 ) */
    49, /* 49 -> 49 ( 1 -> 1 ) */
    50, /* 50 -> 50 ( 2 -> 2 ) */
    51, /* 51 -> 51 ( 3 -> 3 ) */
    52, /* 52 -> 52 ( 4 -> 4 ) */
    53, /* 53 -> 53 ( 5 -> 5 ) */
    54, /* 54 -> 54 ( 6 -> 6 ) */
    55, /* 55 -> 55 ( 7 -> 7 ) */
    56, /* 56 -> 56 ( 8 -> 8 ) */
    57, /* 57 -> 57 ( 9 -> 9 ) */
    58, /* 58 -> 58 ( : -> : ) */
    59, /* 59 -> 59 ( ; -> ; ) */
    60, /* 60 -> 60 ( < -> < ) */
    61, /* 61 -> 61 ( = -> = ) */
    62, /* 62 -> 62 ( > -> > ) */
    63, /* 63 -> 63 ( ? -> ? ) */
    64, /* 64 -> 64 ( @ -> @ ) */
    97, /* 65 -> 97 ( A -> a ) */
    98, /* 66 -> 98 ( B -> b ) */
    99, /* 67 -> 99 ( C -> c ) */
    100, /* 68 -> 100   ( D -> d ) */
    101, /* 69 -> 101   ( E -> e ) */
    102, /* 70 -> 102   ( F -> f ) */
    103, /* 71 -> 103   ( G -> g ) */
    104, /* 72 -> 104   ( H -> h ) */
    105, /* 73 -> 105   ( I -> i ) */
    106, /* 74 -> 106   ( J -> j ) */
    107, /* 75 -> 107   ( K -> k ) */
    108, /* 76 -> 108   ( L -> l ) */
    109, /* 77 -> 109   ( M -> m ) */
    110, /* 78 -> 110   ( N -> n ) */
    111, /* 79 -> 111   ( O -> o ) */
    112, /* 80 -> 112   ( P -> p ) */
    113, /* 81 -> 113   ( Q -> q ) */
    114, /* 82 -> 114   ( R -> r ) */
    115, /* 83 -> 115   ( S -> s ) */
    116, /* 84 -> 116   ( T -> t ) */
    117, /* 85 -> 117   ( U -> u ) */
    118, /* 86 -> 118   ( V -> v ) */
    119, /* 87 -> 119   ( W -> w ) */
    120, /* 88 -> 120   ( X -> x ) */
    121, /* 89 -> 121   ( Y -> y ) */
    122, /* 90 -> 122   ( Z -> z ) */
    91, /* 91 -> 91 ( [ -> [ ) */
    92, /* 92 -> 92 ( \ -> \ ) */
    93, /* 93 -> 93 ( ] -> ] ) */
    94, /* 94 -> 94 ( ^ -> ^ ) */
    95, /* 95 -> 95 ( _ -> _ ) */
    96, /* 96 -> 96 ( ` -> ` ) */
    97, /* 97 -> 97 ( a -> a ) */
    98, /* 98 -> 98 ( b -> b ) */
    99, /* 99 -> 99 ( c -> c ) */
    100, /* 100 -> 100  ( d -> d ) */
    101, /* 101 -> 101  ( e -> e ) */
    102, /* 102 -> 102  ( f -> f ) */
    103, /* 103 -> 103  ( g -> g ) */
    104, /* 104 -> 104  ( h -> h ) */
    105, /* 105 -> 105  ( i -> i ) */
    106, /* 106 -> 106  ( j -> j ) */
    107, /* 107 -> 107  ( k -> k ) */
    108, /* 108 -> 108  ( l -> l ) */
    109, /* 109 -> 109  ( m -> m ) */
    110, /* 110 -> 110  ( n -> n ) */
    111, /* 111 -> 111  ( o -> o ) */
    112, /* 112 -> 112  ( p -> p ) */
    113, /* 113 -> 113  ( q -> q ) */
    114, /* 114 -> 114  ( r -> r ) */
    115, /* 115 -> 115  ( s -> s ) */
    116, /* 116 -> 116  ( t -> t ) */
    117, /* 117 -> 117  ( u -> u ) */
    118, /* 118 -> 118  ( v -> v ) */
    119, /* 119 -> 119  ( w -> w ) */
    120, /* 120 -> 120  ( x -> x ) */
    121, /* 121 -> 121  ( y -> y ) */
    122, /* 122 -> 122  ( z -> z ) */
    123, /* 123 -> 123  ( { -> { ) */
    124, /* 124 -> 124  ( | -> | ) */
    125, /* 125 -> 125  ( } -> } ) */
    126, /* 126 -> 126  ( ~ -> ~ ) */
    127, /* 127 -> 127 */
    128, /* 128 -> 128 */
    129, /* 129 -> 129 */
    130, /* 130 -> 130 */
    131, /* 131 -> 131 */
    132, /* 132 -> 132 */
    133, /* 133 -> 133 */
    134, /* 134 -> 134 */
    135, /* 135 -> 135 */
    136, /* 136 -> 136 */
    137, /* 137 -> 137 */
    138, /* 138 -> 138 */
    139, /* 139 -> 139 */
    140, /* 140 -> 140 */
    141, /* 141 -> 141 */
    142, /* 142 -> 142 */
    143, /* 143 -> 143 */
    144, /* 144 -> 144 */
    145, /* 145 -> 145 */
    146, /* 146 -> 146 */
    147, /* 147 -> 147 */
    148, /* 148 -> 148 */
    149, /* 149 -> 149 */
    150, /* 150 -> 150 */
    151, /* 151 -> 151 */
    152, /* 152 -> 152 */
    153, /* 153 -> 153 */
    154, /* 154 -> 154 */
    155, /* 155 -> 155 */
    156, /* 156 -> 156 */
    157, /* 157 -> 157 */
    158, /* 158 -> 158 */
    159, /* 159 -> 159  ( Ÿ -> Ÿ ) */
    160, /* 160 -> 160  (   ->   ) */
    177, /* 161 -> 177  ( ¡ -> ± ) */
    162, /* 162 -> 162  ( ¢ -> ¢ ) */
    179, /* 163 -> 179  ( £ -> ³ ) */
    164, /* 164 -> 164  ( ¤ -> ¤ ) */
    181, /* 165 -> 181  ( ¥ -> µ ) */
    182, /* 166 -> 182  ( ¦ -> ¶ ) */
    167, /* 167 -> 167  ( § -> § ) */
    168, /* 168 -> 168  ( ¨ -> ¨ ) */
    185, /* 169 -> 185  ( © -> ¹ ) */
    186, /* 170 -> 186  ( ª -> º ) */
    187, /* 171 -> 187  ( « -> » ) */
    188, /* 172 -> 188  ( ¬ -> ¼ ) */
    173, /* 173 -> 173  ( ­ -> ­ ) */
    190, /* 174 -> 190  ( ® -> ¾ ) */
    191, /* 175 -> 191  ( ¯ -> ¿ ) */
    176, /* 176 -> 176  ( ° -> ° ) */
    177, /* 177 -> 177  ( ± -> ± ) */
    178, /* 178 -> 178  ( ² -> ² ) */
    179, /* 179 -> 179  ( ³ -> ³ ) */
    180, /* 180 -> 180  ( ´ -> ´ ) */
    181, /* 181 -> 181  ( µ -> µ ) */
    182, /* 182 -> 182  ( ¶ -> ¶ ) */
    183, /* 183 -> 183  ( · -> · ) */
    184, /* 184 -> 184  ( ¸ -> ¸ ) */
    185, /* 185 -> 185  ( ¹ -> ¹ ) */
    186, /* 186 -> 186  ( º -> º ) */
    187, /* 187 -> 187  ( » -> » ) */
    188, /* 188 -> 188  ( ¼ -> ¼ ) */
    189, /* 189 -> 189  ( ½ -> ½ ) */
    190, /* 190 -> 190  ( ¾ -> ¾ ) */
    191, /* 191 -> 191  ( ¿ -> ¿ ) */
    224, /* 192 -> 224  ( À -> à ) */
    225, /* 193 -> 225  ( Á -> á ) */
    226, /* 194 -> 226  ( Â -> â ) */
    227, /* 195 -> 227  ( Ã -> ã ) */
    228, /* 196 -> 228  ( Ä -> ä ) */
    229, /* 197 -> 229  ( Å -> å ) */
    230, /* 198 -> 230  ( Æ -> æ ) */
    231, /* 199 -> 231  ( Ç -> ç ) */
    232, /* 200 -> 232  ( È -> è ) */
    233, /* 201 -> 233  ( É -> é ) */
    234, /* 202 -> 234  ( Ê -> ê ) */
    235, /* 203 -> 235  ( Ë -> ë ) */
    236, /* 204 -> 236  ( Ì -> ì ) */
    237, /* 205 -> 237  ( Í -> í ) */
    238, /* 206 -> 238  ( Î -> î ) */
    239, /* 207 -> 239  ( Ï -> ï ) */
    240, /* 208 -> 240  ( Ð -> ð ) */
    241, /* 209 -> 241  ( Ñ -> ñ ) */
    242, /* 210 -> 242  ( Ò -> ò ) */
    243, /* 211 -> 243  ( Ó -> ó ) */
    244, /* 212 -> 244  ( Ô -> ô ) */
    245, /* 213 -> 245  ( Õ -> õ ) */
    246, /* 214 -> 246  ( Ö -> ö ) */
    215, /* 215 -> 215  ( × -> × ) */
    248, /* 216 -> 248  ( Ø -> ø ) */
    249, /* 217 -> 249  ( Ù -> ù ) */
    250, /* 218 -> 250  ( Ú -> ú ) */
    251, /* 219 -> 251  ( Û -> û ) */
    252, /* 220 -> 252  ( Ü -> ü ) */
    253, /* 221 -> 253  ( Ý -> ý ) */
    254, /* 222 -> 254  ( Þ -> þ ) */
    223, /* 223 -> 223  ( ß -> ß ) */
    224, /* 224 -> 224  ( à -> à ) */
    225, /* 225 -> 225  ( á -> á ) */
    226, /* 226 -> 226  ( â -> â ) */
    227, /* 227 -> 227  ( ã -> ã ) */
    228, /* 228 -> 228  ( ä -> ä ) */
    229, /* 229 -> 229  ( å -> å ) */
    230, /* 230 -> 230  ( æ -> æ ) */
    231, /* 231 -> 231  ( ç -> ç ) */
    232, /* 232 -> 232  ( è -> è ) */
    233, /* 233 -> 233  ( é -> é ) */
    234, /* 234 -> 234  ( ê -> ê ) */
    235, /* 235 -> 235  ( ë -> ë ) */
    236, /* 236 -> 236  ( ì -> ì ) */
    237, /* 237 -> 237  ( í -> í ) */
    238, /* 238 -> 238  ( î -> î ) */
    239, /* 239 -> 239  ( ï -> ï ) */
    240, /* 240 -> 240  ( ð -> ð ) */
    241, /* 241 -> 241  ( ñ -> ñ ) */
    242, /* 242 -> 242  ( ò -> ò ) */
    243, /* 243 -> 243  ( ó -> ó ) */
    244, /* 244 -> 244  ( ô -> ô ) */
    245, /* 245 -> 245  ( õ -> õ ) */
    246, /* 246 -> 246  ( ö -> ö ) */
    247, /* 247 -> 247  ( ÷ -> ÷ ) */
    248, /* 248 -> 248  ( ø -> ø ) */
    249, /* 249 -> 249  ( ù -> ù ) */
    250, /* 250 -> 250  ( ú -> ú ) */
    251, /* 251 -> 251  ( û -> û ) */
    252, /* 252 -> 252  ( ü -> ü ) */
    253, /* 253 -> 253  ( ý -> ý ) */
    254, /* 254 -> 254  ( þ -> þ ) */
    255  /* 255 -> 255  ( ÿ -> ÿ ) */
};

static const struct ExpandChar ExpansionTbl [ NUM_EXPAND_CHARS + 1 ] = {
{ 0, 0, 0 }
};

static const struct CompressPair CompressTbl [ NUM_COMPRESS_CHARS + 1 ] = {
{ { 99, 115}, { FIRST_PRIMARY+ 76, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 76, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }}, /* cs */
{ { 67, 115}, { FIRST_PRIMARY+ 76, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 76, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }}, /* Cs */
{ { 67,  83}, { FIRST_PRIMARY+ 76, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 76, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }}, /* CS */
{ {100, 122}, { FIRST_PRIMARY+ 80, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 80, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }}, /* dz */
{ { 68, 122}, { FIRST_PRIMARY+ 80, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 80, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }}, /* Dz */
{ { 68,  90}, { FIRST_PRIMARY+ 80, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 80, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }}, /* DZ */
{ {103, 121}, { FIRST_PRIMARY+ 88, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 88, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }}, /* gy */
{ { 71, 121}, { FIRST_PRIMARY+ 88, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 88, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }}, /* Gy */
{ { 71,  89}, { FIRST_PRIMARY+ 88, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 88, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }}, /* GY */
{ {108, 121}, { FIRST_PRIMARY+ 99, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 99, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }}, /* ly */
{ { 76, 121}, { FIRST_PRIMARY+ 99, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 99, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }}, /* Ly */
{ { 76,  89}, { FIRST_PRIMARY+ 99, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY+ 99, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }}, /* LY */
{ {110, 121}, { FIRST_PRIMARY + 104, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 104, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }}, /* ny */
{ { 78, 121}, { FIRST_PRIMARY + 104, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 104, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }}, /* Ny */
{ { 78,  89}, { FIRST_PRIMARY + 104, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 104, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }}, /* NY */
{ {115, 122}, { FIRST_PRIMARY + 120, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 119, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }}, /* sz */
{ { 83, 122}, { FIRST_PRIMARY + 120, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 119, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }}, /* Sz */
{ { 83,  90}, { FIRST_PRIMARY + 120, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 119, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }}, /* SZ */
{ {116, 121}, { FIRST_PRIMARY + 124, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 124, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }}, /* ty */
{ { 84, 121}, { FIRST_PRIMARY + 124, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 124, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }}, /* Ty */
{ { 84,  89}, { FIRST_PRIMARY + 124, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 124, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }}, /* TY */
{ {122, 115}, { FIRST_PRIMARY + 139, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 139, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }}, /* zs */
{ { 90, 115}, { FIRST_PRIMARY + 139, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 139, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }}, /* Zs */
{ { 90,  83}, { FIRST_PRIMARY + 139, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }, { FIRST_PRIMARY + 139, FIRST_SECONDARY+  2, NULL_TERTIARY,      0, 0 }}, /* ZS */
};

static const struct SortOrderTblEntry NoCaseOrderTbl [ NOCASESORT_LEN ] = {
{ FIRST_PRIMARY+  0, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*   0   */
{ FIRST_PRIMARY+  1, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*   1   */
{ FIRST_PRIMARY+  2, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*   2   */
{ FIRST_PRIMARY+  3, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*   3   */
{ FIRST_PRIMARY+  4, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*   4   */
{ FIRST_PRIMARY+  5, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*   5   */
{ FIRST_PRIMARY+  6, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*   6   */
{ FIRST_PRIMARY+  7, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*   7   */
{ FIRST_PRIMARY+  8, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*   8   */
{ FIRST_PRIMARY+  9, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*   9   */
{ FIRST_PRIMARY+ 10, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  10   */
{ FIRST_PRIMARY+ 11, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  11   */
{ FIRST_PRIMARY+ 12, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  12   */
{ FIRST_PRIMARY+ 13, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  13   */
{ FIRST_PRIMARY+ 14, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  14   */
{ FIRST_PRIMARY+ 15, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  15   */
{ FIRST_PRIMARY+ 16, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  16   */
{ FIRST_PRIMARY+ 17, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  17   */
{ FIRST_PRIMARY+ 18, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  18   */
{ FIRST_PRIMARY+ 19, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  19   */
{ FIRST_PRIMARY+ 20, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  20   */
{ FIRST_PRIMARY+ 21, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  21   */
{ FIRST_PRIMARY+ 22, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  22   */
{ FIRST_PRIMARY+ 23, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  23   */
{ FIRST_PRIMARY+ 24, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  24   */
{ FIRST_PRIMARY+ 25, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  25   */
{ FIRST_PRIMARY+ 26, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  26   */
{ FIRST_PRIMARY+ 27, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  27   */
{ FIRST_PRIMARY+ 28, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  28   */
{ FIRST_PRIMARY+ 29, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  29   */
{ FIRST_PRIMARY+ 30, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  30   */
{ FIRST_PRIMARY+ 31, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  31   */
{ FIRST_PRIMARY+ 32, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  32   */
{ FIRST_PRIMARY+ 33, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  33 ! */
{ FIRST_PRIMARY+ 34, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  34 " */
{ FIRST_PRIMARY+ 35, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  35 # */
{ FIRST_PRIMARY+ 36, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  36 $ */
{ FIRST_PRIMARY+ 37, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  37 % */
{ FIRST_PRIMARY+ 38, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  38 & */
{ FIRST_PRIMARY+ 39, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  39 ' */
{ FIRST_PRIMARY+ 40, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  40 ( */
{ FIRST_PRIMARY+ 41, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  41 ) */
{ FIRST_PRIMARY+ 42, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  42 * */
{ FIRST_PRIMARY+ 43, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  43 + */
{ FIRST_PRIMARY+ 44, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  44 , */
{ FIRST_PRIMARY+ 45, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  45 - */
{ FIRST_PRIMARY+ 46, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  46 . */
{ FIRST_PRIMARY+ 47, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  47 / */
{ FIRST_PRIMARY+ 48, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  48 0 */
{ FIRST_PRIMARY+ 49, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  49 1 */
{ FIRST_PRIMARY+ 50, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  50 2 */
{ FIRST_PRIMARY+ 51, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  51 3 */
{ FIRST_PRIMARY+ 52, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  52 4 */
{ FIRST_PRIMARY+ 53, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  53 5 */
{ FIRST_PRIMARY+ 54, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  54 6 */
{ FIRST_PRIMARY+ 55, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  55 7 */
{ FIRST_PRIMARY+ 56, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  56 8 */
{ FIRST_PRIMARY+ 57, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  57 9 */
{ FIRST_PRIMARY+ 58, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  58 : */
{ FIRST_PRIMARY+ 59, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  59 ; */
{ FIRST_PRIMARY+ 60, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  60 < */
{ FIRST_PRIMARY+ 61, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  61 = */
{ FIRST_PRIMARY+ 62, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  62 > */
{ FIRST_PRIMARY+ 63, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  63 ? */
{ FIRST_PRIMARY+ 64, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  64 @ */
{ FIRST_PRIMARY+ 65, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  65 A */
{ FIRST_PRIMARY+ 71, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  66 B */
{ FIRST_PRIMARY+ 72, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 1 }, /*  67 C */
{ FIRST_PRIMARY+ 77, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 1 }, /*  68 D */
{ FIRST_PRIMARY+ 81, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  69 E */
{ FIRST_PRIMARY+ 86, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  70 F */
{ FIRST_PRIMARY+ 87, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 1 }, /*  71 G */
{ FIRST_PRIMARY+ 89, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  72 H */
{ FIRST_PRIMARY+ 90, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  73 I */
{ FIRST_PRIMARY+ 93, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  74 J */
{ FIRST_PRIMARY+ 94, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  75 K */
{ FIRST_PRIMARY+ 95, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 1 }, /*  76 L */
{ FIRST_PRIMARY + 100, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  77 M */
{ FIRST_PRIMARY + 101, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 1 }, /*  78 N */
{ FIRST_PRIMARY + 105, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  79 O */
{ FIRST_PRIMARY + 110, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  80 P */
{ FIRST_PRIMARY + 111, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  81 Q */
{ FIRST_PRIMARY + 112, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  82 R */
{ FIRST_PRIMARY + 115, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 1 }, /*  83 S */
{ FIRST_PRIMARY + 121, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 1 }, /*  84 T */
{ FIRST_PRIMARY + 125, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  85 U */
{ FIRST_PRIMARY + 130, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  86 V */
{ FIRST_PRIMARY + 131, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  87 W */
{ FIRST_PRIMARY + 132, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  88 X */
{ FIRST_PRIMARY + 133, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /*  89 Y */
{ FIRST_PRIMARY + 135, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 1 }, /*  90 Z */
{ FIRST_PRIMARY + 140, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  91 [ */
{ FIRST_PRIMARY + 141, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  92 \ */
{ FIRST_PRIMARY + 142, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  93 ] */
{ FIRST_PRIMARY + 143, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  94 ^ */
{ FIRST_PRIMARY + 144, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  95 _ */
{ FIRST_PRIMARY + 145, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /*  96 ` */
{ FIRST_PRIMARY+ 65, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /*  97 a */
{ FIRST_PRIMARY+ 71, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /*  98 b */
{ FIRST_PRIMARY+ 72, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 1 }, /*  99 c */
{ FIRST_PRIMARY+ 77, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 1 }, /* 100 d */
{ FIRST_PRIMARY+ 81, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 101 e */
{ FIRST_PRIMARY+ 86, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 102 f */
{ FIRST_PRIMARY+ 87, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 1 }, /* 103 g */
{ FIRST_PRIMARY+ 89, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 104 h */
{ FIRST_PRIMARY+ 90, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 105 i */
{ FIRST_PRIMARY+ 93, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 106 j */
{ FIRST_PRIMARY+ 94, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 107 k */
{ FIRST_PRIMARY+ 95, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 1 }, /* 108 l */
{ FIRST_PRIMARY + 100, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 109 m */
{ FIRST_PRIMARY + 101, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 1 }, /* 110 n */
{ FIRST_PRIMARY + 105, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 111 o */
{ FIRST_PRIMARY + 110, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 112 p */
{ FIRST_PRIMARY + 111, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 113 q */
{ FIRST_PRIMARY + 112, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 114 r */
{ FIRST_PRIMARY + 115, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 1 }, /* 115 s */
{ FIRST_PRIMARY + 121, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 1 }, /* 116 t */
{ FIRST_PRIMARY + 125, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 117 u */
{ FIRST_PRIMARY + 130, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 118 v */
{ FIRST_PRIMARY + 131, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 119 w */
{ FIRST_PRIMARY + 132, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 120 x */
{ FIRST_PRIMARY + 133, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 121 y */
{ FIRST_PRIMARY + 135, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 1 }, /* 122 z */
{ FIRST_PRIMARY + 146, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 123 { */
{ FIRST_PRIMARY + 147, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 124 | */
{ FIRST_PRIMARY + 148, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 125 } */
{ FIRST_PRIMARY + 149, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 126 ~ */
{ FIRST_PRIMARY + 150, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 127   */
{ FIRST_PRIMARY + 151, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 128   */
{ FIRST_PRIMARY + 152, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 129   */
{ FIRST_PRIMARY + 153, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 130   */
{ FIRST_PRIMARY + 154, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 131   */
{ FIRST_PRIMARY + 155, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 132   */
{ FIRST_PRIMARY + 156, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 133   */
{ FIRST_PRIMARY + 157, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 134   */
{ FIRST_PRIMARY + 158, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 135   */
{ FIRST_PRIMARY + 159, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 136   */
{ FIRST_PRIMARY + 160, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 137   */
{ FIRST_PRIMARY + 161, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 138   */
{ FIRST_PRIMARY + 162, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 139   */
{ FIRST_PRIMARY + 163, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 140   */
{ FIRST_PRIMARY + 164, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 141   */
{ FIRST_PRIMARY + 165, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 142   */
{ FIRST_PRIMARY + 166, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 143   */
{ FIRST_PRIMARY + 167, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 144   */
{ FIRST_PRIMARY + 168, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 145   */
{ FIRST_PRIMARY + 169, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 146   */
{ FIRST_PRIMARY + 170, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 147   */
{ FIRST_PRIMARY + 171, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 148   */
{ FIRST_PRIMARY + 172, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 149   */
{ FIRST_PRIMARY + 173, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 150   */
{ FIRST_PRIMARY + 174, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 151   */
{ FIRST_PRIMARY + 175, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 152   */
{ FIRST_PRIMARY + 176, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 153   */
{ FIRST_PRIMARY + 177, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 155   */
{ FIRST_PRIMARY + 178, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 156   */
{ FIRST_PRIMARY + 179, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 154   */
{ FIRST_PRIMARY + 180, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 157   */
{ FIRST_PRIMARY + 181, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 158   */
{ FIRST_PRIMARY + 182, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 159   */
{ FIRST_PRIMARY + 183, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 160   */
{ FIRST_PRIMARY+ 70, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 161 ¡ */
{ FIRST_PRIMARY + 184, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 162 ¢ */
{ FIRST_PRIMARY+ 96, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 163 £ */
{ FIRST_PRIMARY + 185, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 164 ¤ */
{ FIRST_PRIMARY+ 98, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 165 ¥ */
{ FIRST_PRIMARY + 116, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 166 ¦ */
{ FIRST_PRIMARY + 186, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 167 § */
{ FIRST_PRIMARY + 187, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 168 ¨ */
{ FIRST_PRIMARY + 117, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 169 © */
{ FIRST_PRIMARY + 118, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 170 ª */
{ FIRST_PRIMARY + 122, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 171 « */
{ FIRST_PRIMARY + 138, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 172 ¬ */
{ FIRST_PRIMARY + 188, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 173  ­ */
{ FIRST_PRIMARY + 137, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 174 ® */
{ FIRST_PRIMARY + 136, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 175 ¯ */
{ FIRST_PRIMARY + 189, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 176 ° */
{ FIRST_PRIMARY+ 70, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 177 ± */
{ FIRST_PRIMARY + 190, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 178 ² */
{ FIRST_PRIMARY+ 96, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 179 ³ */
{ FIRST_PRIMARY + 191, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 180 ´ */
{ FIRST_PRIMARY+ 98, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 181 µ */
{ FIRST_PRIMARY + 116, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 182 ¶ */
{ FIRST_PRIMARY + 192, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 183 · */
{ FIRST_PRIMARY + 193, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 184 ¸ */
{ FIRST_PRIMARY + 117, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 185 ¹ */
{ FIRST_PRIMARY + 118, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 186 º */
{ FIRST_PRIMARY + 122, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 187 » */
{ FIRST_PRIMARY + 138, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 188 ¼ */
{ FIRST_PRIMARY + 194, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 189 ½ */
{ FIRST_PRIMARY + 137, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 190 ¾ */
{ FIRST_PRIMARY + 136, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 191 ¿ */
{ FIRST_PRIMARY + 114, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 192 À */
{ FIRST_PRIMARY+ 66, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 193 Á */
{ FIRST_PRIMARY+ 67, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 194 Â */
{ FIRST_PRIMARY+ 68, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 195 Ã */
{ FIRST_PRIMARY+ 69, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 196 Ä */
{ FIRST_PRIMARY+ 97, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 197 Å */
{ FIRST_PRIMARY+ 73, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 198 Æ */
{ FIRST_PRIMARY+ 74, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 199 Ç */
{ FIRST_PRIMARY+ 75, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 200 È */
{ FIRST_PRIMARY+ 82, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 201 É */
{ FIRST_PRIMARY+ 84, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 202 Ê */
{ FIRST_PRIMARY+ 85, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 203 Ë */
{ FIRST_PRIMARY+ 83, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 204 Ì */
{ FIRST_PRIMARY+ 91, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 205 Í */
{ FIRST_PRIMARY+ 92, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 206 Î */
{ FIRST_PRIMARY+ 79, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 207 Ï */
{ FIRST_PRIMARY+ 78, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 208 Ð */
{ FIRST_PRIMARY + 102, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 209 Ñ */
{ FIRST_PRIMARY + 103, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 210 Ò */
{ FIRST_PRIMARY + 106, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 211 Ó */
{ FIRST_PRIMARY + 109, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 212 Ô */
{ FIRST_PRIMARY + 108, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 213 Õ */
{ FIRST_PRIMARY + 107, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 214 Ö */
{ FIRST_PRIMARY + 195, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 215 × */
{ FIRST_PRIMARY + 113, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 216 Ø */
{ FIRST_PRIMARY + 127, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 217 Ù */
{ FIRST_PRIMARY + 126, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 218 Ú */
{ FIRST_PRIMARY + 129, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 219 Û */
{ FIRST_PRIMARY + 128, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 220 Ü */
{ FIRST_PRIMARY + 134, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 221 Ý */
{ FIRST_PRIMARY + 123, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 222 Þ */
{ FIRST_PRIMARY + 119, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 223 ß */
{ FIRST_PRIMARY + 114, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 224 à */
{ FIRST_PRIMARY+ 66, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 225 á */
{ FIRST_PRIMARY+ 67, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 226 â */
{ FIRST_PRIMARY+ 68, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 227 ã */
{ FIRST_PRIMARY+ 69, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 228 ä */
{ FIRST_PRIMARY+ 97, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 229 å */
{ FIRST_PRIMARY+ 73, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 230 æ */
{ FIRST_PRIMARY+ 74, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 231 ç */
{ FIRST_PRIMARY+ 75, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 232 è */
{ FIRST_PRIMARY+ 82, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 233 é */
{ FIRST_PRIMARY+ 84, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 234 ê */
{ FIRST_PRIMARY+ 85, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 235 ë */
{ FIRST_PRIMARY+ 83, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 236 ì */
{ FIRST_PRIMARY+ 91, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 237 í */
{ FIRST_PRIMARY+ 92, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 238 î */
{ FIRST_PRIMARY+ 79, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 239 ï */
{ FIRST_PRIMARY+ 78, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 240 ð */
{ FIRST_PRIMARY + 102, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 241 ñ */
{ FIRST_PRIMARY + 103, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 242 ò */
{ FIRST_PRIMARY + 106, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 243 ó */
{ FIRST_PRIMARY + 109, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 244 ô */
{ FIRST_PRIMARY + 108, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 245 õ */
{ FIRST_PRIMARY + 107, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 246 ö */
{ FIRST_PRIMARY + 196, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 247 ÷ */
{ FIRST_PRIMARY + 113, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 248 ø */
{ FIRST_PRIMARY + 127, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 249 ù */
{ FIRST_PRIMARY + 126, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 250 ú */
{ FIRST_PRIMARY + 129, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 251 û */
{ FIRST_PRIMARY + 128, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 252 ü */
{ FIRST_PRIMARY + 134, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 253 ý */
{ FIRST_PRIMARY + 123, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 254 þ */
{ FIRST_PRIMARY + 197, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }  /* 255   */
};


/* End of File : Language driver lat2hun */

