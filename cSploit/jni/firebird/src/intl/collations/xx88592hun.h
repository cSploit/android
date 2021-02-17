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
    159, /* 159 -> 159  ( � -> � ) */
    160, /* 160 -> 160  ( � -> � ) */
    161, /* 161 -> 161  ( � -> � ) */
    162, /* 162 -> 162  ( � -> � ) */
    163, /* 163 -> 163  ( � -> � ) */
    164, /* 164 -> 164  ( � -> � ) */
    165, /* 165 -> 165  ( � -> � ) */
    166, /* 166 -> 166  ( � -> � ) */
    167, /* 167 -> 167  ( � -> � ) */
    168, /* 168 -> 168  ( � -> � ) */
    169, /* 169 -> 169  ( � -> � ) */
    170, /* 170 -> 170  ( � -> � ) */
    171, /* 171 -> 171  ( � -> � ) */
    172, /* 172 -> 172  ( � -> � ) */
    173, /* 173 -> 173  ( � -> � ) */
    174, /* 174 -> 174  ( � -> � ) */
    175, /* 175 -> 175  ( � -> � ) */
    176, /* 176 -> 176  ( � -> � ) */
    161, /* 177 -> 161  ( � -> � ) */
    178, /* 178 -> 178  ( � -> � ) */
    163, /* 179 -> 163  ( � -> � ) */
    180, /* 180 -> 180  ( � -> � ) */
    165, /* 181 -> 165  ( � -> � ) */
    166, /* 182 -> 166  ( � -> � ) */
    183, /* 183 -> 183  ( � -> � ) */
    184, /* 184 -> 184  ( � -> � ) */
    169, /* 185 -> 169  ( � -> � ) */
    170, /* 186 -> 170  ( � -> � ) */
    171, /* 187 -> 171  ( � -> � ) */
    172, /* 188 -> 172  ( � -> � ) */
    189, /* 189 -> 189  ( � -> � ) */
    174, /* 190 -> 174  ( � -> � ) */
    175, /* 191 -> 175  ( � -> � ) */
    192, /* 192 -> 192  ( � -> � ) */
    193, /* 193 -> 193  ( � -> � ) */
    194, /* 194 -> 194  ( � -> � ) */
    195, /* 195 -> 195  ( � -> � ) */
    196, /* 196 -> 196  ( � -> � ) */
    197, /* 197 -> 197  ( � -> � ) */
    198, /* 198 -> 198  ( � -> � ) */
    199, /* 199 -> 199  ( � -> � ) */
    200, /* 200 -> 200  ( � -> � ) */
    201, /* 201 -> 201  ( � -> � ) */
    202, /* 202 -> 202  ( � -> � ) */
    203, /* 203 -> 203  ( � -> � ) */
    204, /* 204 -> 204  ( � -> � ) */
    205, /* 205 -> 205  ( � -> � ) */
    206, /* 206 -> 206  ( � -> � ) */
    207, /* 207 -> 207  ( � -> � ) */
    208, /* 208 -> 208  ( � -> � ) */
    209, /* 209 -> 209  ( � -> � ) */
    210, /* 210 -> 210  ( � -> � ) */
    211, /* 211 -> 211  ( � -> � ) */
    212, /* 212 -> 212  ( � -> � ) */
    213, /* 213 -> 213  ( � -> � ) */
    214, /* 214 -> 214  ( � -> � ) */
    215, /* 215 -> 215  ( � -> � ) */
    216, /* 216 -> 216  ( � -> � ) */
    217, /* 217 -> 217  ( � -> � ) */
    218, /* 218 -> 218  ( � -> � ) */
    219, /* 219 -> 219  ( � -> � ) */
    220, /* 220 -> 220  ( � -> � ) */
    221, /* 221 -> 221  ( � -> � ) */
    222, /* 222 -> 222  ( � -> � ) */
    223, /* 223 -> 223  ( � -> � ) */
    192, /* 224 -> 192  ( � -> � ) */
    193, /* 225 -> 193  ( � -> � ) */
    194, /* 226 -> 194  ( � -> � ) */
    195, /* 227 -> 195  ( � -> � ) */
    196, /* 228 -> 196  ( � -> � ) */
    197, /* 229 -> 197  ( � -> � ) */
    198, /* 230 -> 198  ( � -> � ) */
    199, /* 231 -> 199  ( � -> � ) */
    200, /* 232 -> 200  ( � -> � ) */
    201, /* 233 -> 201  ( � -> � ) */
    202, /* 234 -> 202  ( � -> � ) */
    203, /* 235 -> 203  ( � -> � ) */
    204, /* 236 -> 204  ( � -> � ) */
    205, /* 237 -> 205  ( � -> � ) */
    206, /* 238 -> 206  ( � -> � ) */
    207, /* 239 -> 207  ( � -> � ) */
    208, /* 240 -> 208  ( � -> � ) */
    209, /* 241 -> 209  ( � -> � ) */
    210, /* 242 -> 210  ( � -> � ) */
    211, /* 243 -> 211  ( � -> � ) */
    212, /* 244 -> 212  ( � -> � ) */
    213, /* 245 -> 213  ( � -> � ) */
    214, /* 246 -> 214  ( � -> � ) */
    247, /* 247 -> 247  ( � -> � ) */
    216, /* 248 -> 216  ( � -> � ) */
    217, /* 249 -> 217  ( � -> � ) */
    218, /* 250 -> 218  ( � -> � ) */
    219, /* 251 -> 219  ( � -> � ) */
    220, /* 252 -> 220  ( � -> � ) */
    221, /* 253 -> 221  ( � -> � ) */
    222, /* 254 -> 222  ( � -> � ) */
    255  /* 255 -> 255  ( � -> � ) */
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
    159, /* 159 -> 159  ( � -> � ) */
    160, /* 160 -> 160  ( � -> � ) */
    177, /* 161 -> 177  ( � -> � ) */
    162, /* 162 -> 162  ( � -> � ) */
    179, /* 163 -> 179  ( � -> � ) */
    164, /* 164 -> 164  ( � -> � ) */
    181, /* 165 -> 181  ( � -> � ) */
    182, /* 166 -> 182  ( � -> � ) */
    167, /* 167 -> 167  ( � -> � ) */
    168, /* 168 -> 168  ( � -> � ) */
    185, /* 169 -> 185  ( � -> � ) */
    186, /* 170 -> 186  ( � -> � ) */
    187, /* 171 -> 187  ( � -> � ) */
    188, /* 172 -> 188  ( � -> � ) */
    173, /* 173 -> 173  ( � -> � ) */
    190, /* 174 -> 190  ( � -> � ) */
    191, /* 175 -> 191  ( � -> � ) */
    176, /* 176 -> 176  ( � -> � ) */
    177, /* 177 -> 177  ( � -> � ) */
    178, /* 178 -> 178  ( � -> � ) */
    179, /* 179 -> 179  ( � -> � ) */
    180, /* 180 -> 180  ( � -> � ) */
    181, /* 181 -> 181  ( � -> � ) */
    182, /* 182 -> 182  ( � -> � ) */
    183, /* 183 -> 183  ( � -> � ) */
    184, /* 184 -> 184  ( � -> � ) */
    185, /* 185 -> 185  ( � -> � ) */
    186, /* 186 -> 186  ( � -> � ) */
    187, /* 187 -> 187  ( � -> � ) */
    188, /* 188 -> 188  ( � -> � ) */
    189, /* 189 -> 189  ( � -> � ) */
    190, /* 190 -> 190  ( � -> � ) */
    191, /* 191 -> 191  ( � -> � ) */
    224, /* 192 -> 224  ( � -> � ) */
    225, /* 193 -> 225  ( � -> � ) */
    226, /* 194 -> 226  ( � -> � ) */
    227, /* 195 -> 227  ( � -> � ) */
    228, /* 196 -> 228  ( � -> � ) */
    229, /* 197 -> 229  ( � -> � ) */
    230, /* 198 -> 230  ( � -> � ) */
    231, /* 199 -> 231  ( � -> � ) */
    232, /* 200 -> 232  ( � -> � ) */
    233, /* 201 -> 233  ( � -> � ) */
    234, /* 202 -> 234  ( � -> � ) */
    235, /* 203 -> 235  ( � -> � ) */
    236, /* 204 -> 236  ( � -> � ) */
    237, /* 205 -> 237  ( � -> � ) */
    238, /* 206 -> 238  ( � -> � ) */
    239, /* 207 -> 239  ( � -> � ) */
    240, /* 208 -> 240  ( � -> � ) */
    241, /* 209 -> 241  ( � -> � ) */
    242, /* 210 -> 242  ( � -> � ) */
    243, /* 211 -> 243  ( � -> � ) */
    244, /* 212 -> 244  ( � -> � ) */
    245, /* 213 -> 245  ( � -> � ) */
    246, /* 214 -> 246  ( � -> � ) */
    215, /* 215 -> 215  ( � -> � ) */
    248, /* 216 -> 248  ( � -> � ) */
    249, /* 217 -> 249  ( � -> � ) */
    250, /* 218 -> 250  ( � -> � ) */
    251, /* 219 -> 251  ( � -> � ) */
    252, /* 220 -> 252  ( � -> � ) */
    253, /* 221 -> 253  ( � -> � ) */
    254, /* 222 -> 254  ( � -> � ) */
    223, /* 223 -> 223  ( � -> � ) */
    224, /* 224 -> 224  ( � -> � ) */
    225, /* 225 -> 225  ( � -> � ) */
    226, /* 226 -> 226  ( � -> � ) */
    227, /* 227 -> 227  ( � -> � ) */
    228, /* 228 -> 228  ( � -> � ) */
    229, /* 229 -> 229  ( � -> � ) */
    230, /* 230 -> 230  ( � -> � ) */
    231, /* 231 -> 231  ( � -> � ) */
    232, /* 232 -> 232  ( � -> � ) */
    233, /* 233 -> 233  ( � -> � ) */
    234, /* 234 -> 234  ( � -> � ) */
    235, /* 235 -> 235  ( � -> � ) */
    236, /* 236 -> 236  ( � -> � ) */
    237, /* 237 -> 237  ( � -> � ) */
    238, /* 238 -> 238  ( � -> � ) */
    239, /* 239 -> 239  ( � -> � ) */
    240, /* 240 -> 240  ( � -> � ) */
    241, /* 241 -> 241  ( � -> � ) */
    242, /* 242 -> 242  ( � -> � ) */
    243, /* 243 -> 243  ( � -> � ) */
    244, /* 244 -> 244  ( � -> � ) */
    245, /* 245 -> 245  ( � -> � ) */
    246, /* 246 -> 246  ( � -> � ) */
    247, /* 247 -> 247  ( � -> � ) */
    248, /* 248 -> 248  ( � -> � ) */
    249, /* 249 -> 249  ( � -> � ) */
    250, /* 250 -> 250  ( � -> � ) */
    251, /* 251 -> 251  ( � -> � ) */
    252, /* 252 -> 252  ( � -> � ) */
    253, /* 253 -> 253  ( � -> � ) */
    254, /* 254 -> 254  ( � -> � ) */
    255  /* 255 -> 255  ( � -> � ) */
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
{ FIRST_PRIMARY+ 70, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 161 � */
{ FIRST_PRIMARY + 184, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 162 � */
{ FIRST_PRIMARY+ 96, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 163 � */
{ FIRST_PRIMARY + 185, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 164 � */
{ FIRST_PRIMARY+ 98, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 165 � */
{ FIRST_PRIMARY + 116, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 166 � */
{ FIRST_PRIMARY + 186, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 167 � */
{ FIRST_PRIMARY + 187, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 168 � */
{ FIRST_PRIMARY + 117, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 169 � */
{ FIRST_PRIMARY + 118, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 170 � */
{ FIRST_PRIMARY + 122, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 171 � */
{ FIRST_PRIMARY + 138, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 172 � */
{ FIRST_PRIMARY + 188, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 173  � */
{ FIRST_PRIMARY + 137, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 174 � */
{ FIRST_PRIMARY + 136, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 175 � */
{ FIRST_PRIMARY + 189, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 176 � */
{ FIRST_PRIMARY+ 70, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 177 � */
{ FIRST_PRIMARY + 190, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 178 � */
{ FIRST_PRIMARY+ 96, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 179 � */
{ FIRST_PRIMARY + 191, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 180 � */
{ FIRST_PRIMARY+ 98, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 181 � */
{ FIRST_PRIMARY + 116, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 182 � */
{ FIRST_PRIMARY + 192, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 183 � */
{ FIRST_PRIMARY + 193, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 184 � */
{ FIRST_PRIMARY + 117, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 185 � */
{ FIRST_PRIMARY + 118, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 186 � */
{ FIRST_PRIMARY + 122, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 187 � */
{ FIRST_PRIMARY + 138, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 188 � */
{ FIRST_PRIMARY + 194, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 189 � */
{ FIRST_PRIMARY + 137, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 190 � */
{ FIRST_PRIMARY + 136, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 191 � */
{ FIRST_PRIMARY + 114, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 192 � */
{ FIRST_PRIMARY+ 66, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 193 � */
{ FIRST_PRIMARY+ 67, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 194 � */
{ FIRST_PRIMARY+ 68, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 195 � */
{ FIRST_PRIMARY+ 69, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 196 � */
{ FIRST_PRIMARY+ 97, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 197 � */
{ FIRST_PRIMARY+ 73, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 198 � */
{ FIRST_PRIMARY+ 74, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 199 � */
{ FIRST_PRIMARY+ 75, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 200 � */
{ FIRST_PRIMARY+ 82, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 201 � */
{ FIRST_PRIMARY+ 84, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 202 � */
{ FIRST_PRIMARY+ 85, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 203 � */
{ FIRST_PRIMARY+ 83, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 204 � */
{ FIRST_PRIMARY+ 91, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 205 � */
{ FIRST_PRIMARY+ 92, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 206 � */
{ FIRST_PRIMARY+ 79, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 207 � */
{ FIRST_PRIMARY+ 78, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 208 � */
{ FIRST_PRIMARY + 102, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 209 � */
{ FIRST_PRIMARY + 103, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 210 � */
{ FIRST_PRIMARY + 106, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 211 � */
{ FIRST_PRIMARY + 109, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 212 � */
{ FIRST_PRIMARY + 108, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 213 � */
{ FIRST_PRIMARY + 107, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 214 � */
{ FIRST_PRIMARY + 195, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 215 � */
{ FIRST_PRIMARY + 113, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 216 � */
{ FIRST_PRIMARY + 127, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 217 � */
{ FIRST_PRIMARY + 126, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 218 � */
{ FIRST_PRIMARY + 129, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 219 � */
{ FIRST_PRIMARY + 128, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 220 � */
{ FIRST_PRIMARY + 134, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 221 � */
{ FIRST_PRIMARY + 123, FIRST_SECONDARY+  1, NULL_TERTIARY,      0, 0 }, /* 222 � */
{ FIRST_PRIMARY + 119, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 223 � */
{ FIRST_PRIMARY + 114, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 224 � */
{ FIRST_PRIMARY+ 66, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 225 � */
{ FIRST_PRIMARY+ 67, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 226 � */
{ FIRST_PRIMARY+ 68, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 227 � */
{ FIRST_PRIMARY+ 69, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 228 � */
{ FIRST_PRIMARY+ 97, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 229 � */
{ FIRST_PRIMARY+ 73, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 230 � */
{ FIRST_PRIMARY+ 74, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 231 � */
{ FIRST_PRIMARY+ 75, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 232 � */
{ FIRST_PRIMARY+ 82, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 233 � */
{ FIRST_PRIMARY+ 84, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 234 � */
{ FIRST_PRIMARY+ 85, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 235 � */
{ FIRST_PRIMARY+ 83, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 236 � */
{ FIRST_PRIMARY+ 91, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 237 � */
{ FIRST_PRIMARY+ 92, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 238 � */
{ FIRST_PRIMARY+ 79, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 239 � */
{ FIRST_PRIMARY+ 78, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 240 � */
{ FIRST_PRIMARY + 102, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 241 � */
{ FIRST_PRIMARY + 103, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 242 � */
{ FIRST_PRIMARY + 106, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 243 � */
{ FIRST_PRIMARY + 109, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 244 � */
{ FIRST_PRIMARY + 108, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 245 � */
{ FIRST_PRIMARY + 107, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 246 � */
{ FIRST_PRIMARY + 196, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }, /* 247 � */
{ FIRST_PRIMARY + 113, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 248 � */
{ FIRST_PRIMARY + 127, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 249 � */
{ FIRST_PRIMARY + 126, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 250 � */
{ FIRST_PRIMARY + 129, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 251 � */
{ FIRST_PRIMARY + 128, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 252 � */
{ FIRST_PRIMARY + 134, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 253 � */
{ FIRST_PRIMARY + 123, FIRST_SECONDARY+  0, NULL_TERTIARY,      0, 0 }, /* 254 � */
{ FIRST_PRIMARY + 197, NULL_SECONDARY,      NULL_TERTIARY,      0, 0 }  /* 255   */
};


/* End of File : Language driver lat2hun */

