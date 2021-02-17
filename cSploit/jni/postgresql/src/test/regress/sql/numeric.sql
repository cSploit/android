--
-- NUMERIC
--

CREATE TABLE num_data (id int4, val numeric(210,10));
CREATE TABLE num_exp_add (id1 int4, id2 int4, expected numeric(210,10));
CREATE TABLE num_exp_sub (id1 int4, id2 int4, expected numeric(210,10));
CREATE TABLE num_exp_div (id1 int4, id2 int4, expected numeric(210,10));
CREATE TABLE num_exp_mul (id1 int4, id2 int4, expected numeric(210,10));
CREATE TABLE num_exp_sqrt (id int4, expected numeric(210,10));
CREATE TABLE num_exp_ln (id int4, expected numeric(210,10));
CREATE TABLE num_exp_log10 (id int4, expected numeric(210,10));
CREATE TABLE num_exp_power_10_ln (id int4, expected numeric(210,10));

CREATE TABLE num_result (id1 int4, id2 int4, result numeric(210,10));


-- ******************************
-- * The following EXPECTED results are computed by bc(1)
-- * with a scale of 200
-- ******************************

BEGIN TRANSACTION;
INSERT INTO num_exp_add VALUES (0,0,'0');
INSERT INTO num_exp_sub VALUES (0,0,'0');
INSERT INTO num_exp_mul VALUES (0,0,'0');
INSERT INTO num_exp_div VALUES (0,0,'NaN');
INSERT INTO num_exp_add VALUES (0,1,'0');
INSERT INTO num_exp_sub VALUES (0,1,'0');
INSERT INTO num_exp_mul VALUES (0,1,'0');
INSERT INTO num_exp_div VALUES (0,1,'NaN');
INSERT INTO num_exp_add VALUES (0,2,'-34338492.215397047');
INSERT INTO num_exp_sub VALUES (0,2,'34338492.215397047');
INSERT INTO num_exp_mul VALUES (0,2,'0');
INSERT INTO num_exp_div VALUES (0,2,'0');
INSERT INTO num_exp_add VALUES (0,3,'4.31');
INSERT INTO num_exp_sub VALUES (0,3,'-4.31');
INSERT INTO num_exp_mul VALUES (0,3,'0');
INSERT INTO num_exp_div VALUES (0,3,'0');
INSERT INTO num_exp_add VALUES (0,4,'7799461.4119');
INSERT INTO num_exp_sub VALUES (0,4,'-7799461.4119');
INSERT INTO num_exp_mul VALUES (0,4,'0');
INSERT INTO num_exp_div VALUES (0,4,'0');
INSERT INTO num_exp_add VALUES (0,5,'16397.038491');
INSERT INTO num_exp_sub VALUES (0,5,'-16397.038491');
INSERT INTO num_exp_mul VALUES (0,5,'0');
INSERT INTO num_exp_div VALUES (0,5,'0');
INSERT INTO num_exp_add VALUES (0,6,'93901.57763026');
INSERT INTO num_exp_sub VALUES (0,6,'-93901.57763026');
INSERT INTO num_exp_mul VALUES (0,6,'0');
INSERT INTO num_exp_div VALUES (0,6,'0');
INSERT INTO num_exp_add VALUES (0,7,'-83028485');
INSERT INTO num_exp_sub VALUES (0,7,'83028485');
INSERT INTO num_exp_mul VALUES (0,7,'0');
INSERT INTO num_exp_div VALUES (0,7,'0');
INSERT INTO num_exp_add VALUES (0,8,'74881');
INSERT INTO num_exp_sub VALUES (0,8,'-74881');
INSERT INTO num_exp_mul VALUES (0,8,'0');
INSERT INTO num_exp_div VALUES (0,8,'0');
INSERT INTO num_exp_add VALUES (0,9,'-24926804.045047420');
INSERT INTO num_exp_sub VALUES (0,9,'24926804.045047420');
INSERT INTO num_exp_mul VALUES (0,9,'0');
INSERT INTO num_exp_div VALUES (0,9,'0');
INSERT INTO num_exp_add VALUES (1,0,'0');
INSERT INTO num_exp_sub VALUES (1,0,'0');
INSERT INTO num_exp_mul VALUES (1,0,'0');
INSERT INTO num_exp_div VALUES (1,0,'NaN');
INSERT INTO num_exp_add VALUES (1,1,'0');
INSERT INTO num_exp_sub VALUES (1,1,'0');
INSERT INTO num_exp_mul VALUES (1,1,'0');
INSERT INTO num_exp_div VALUES (1,1,'NaN');
INSERT INTO num_exp_add VALUES (1,2,'-34338492.215397047');
INSERT INTO num_exp_sub VALUES (1,2,'34338492.215397047');
INSERT INTO num_exp_mul VALUES (1,2,'0');
INSERT INTO num_exp_div VALUES (1,2,'0');
INSERT INTO num_exp_add VALUES (1,3,'4.31');
INSERT INTO num_exp_sub VALUES (1,3,'-4.31');
INSERT INTO num_exp_mul VALUES (1,3,'0');
INSERT INTO num_exp_div VALUES (1,3,'0');
INSERT INTO num_exp_add VALUES (1,4,'7799461.4119');
INSERT INTO num_exp_sub VALUES (1,4,'-7799461.4119');
INSERT INTO num_exp_mul VALUES (1,4,'0');
INSERT INTO num_exp_div VALUES (1,4,'0');
INSERT INTO num_exp_add VALUES (1,5,'16397.038491');
INSERT INTO num_exp_sub VALUES (1,5,'-16397.038491');
INSERT INTO num_exp_mul VALUES (1,5,'0');
INSERT INTO num_exp_div VALUES (1,5,'0');
INSERT INTO num_exp_add VALUES (1,6,'93901.57763026');
INSERT INTO num_exp_sub VALUES (1,6,'-93901.57763026');
INSERT INTO num_exp_mul VALUES (1,6,'0');
INSERT INTO num_exp_div VALUES (1,6,'0');
INSERT INTO num_exp_add VALUES (1,7,'-83028485');
INSERT INTO num_exp_sub VALUES (1,7,'83028485');
INSERT INTO num_exp_mul VALUES (1,7,'0');
INSERT INTO num_exp_div VALUES (1,7,'0');
INSERT INTO num_exp_add VALUES (1,8,'74881');
INSERT INTO num_exp_sub VALUES (1,8,'-74881');
INSERT INTO num_exp_mul VALUES (1,8,'0');
INSERT INTO num_exp_div VALUES (1,8,'0');
INSERT INTO num_exp_add VALUES (1,9,'-24926804.045047420');
INSERT INTO num_exp_sub VALUES (1,9,'24926804.045047420');
INSERT INTO num_exp_mul VALUES (1,9,'0');
INSERT INTO num_exp_div VALUES (1,9,'0');
INSERT INTO num_exp_add VALUES (2,0,'-34338492.215397047');
INSERT INTO num_exp_sub VALUES (2,0,'-34338492.215397047');
INSERT INTO num_exp_mul VALUES (2,0,'0');
INSERT INTO num_exp_div VALUES (2,0,'NaN');
INSERT INTO num_exp_add VALUES (2,1,'-34338492.215397047');
INSERT INTO num_exp_sub VALUES (2,1,'-34338492.215397047');
INSERT INTO num_exp_mul VALUES (2,1,'0');
INSERT INTO num_exp_div VALUES (2,1,'NaN');
INSERT INTO num_exp_add VALUES (2,2,'-68676984.430794094');
INSERT INTO num_exp_sub VALUES (2,2,'0');
INSERT INTO num_exp_mul VALUES (2,2,'1179132047626883.596862135856320209');
INSERT INTO num_exp_div VALUES (2,2,'1.00000000000000000000');
INSERT INTO num_exp_add VALUES (2,3,'-34338487.905397047');
INSERT INTO num_exp_sub VALUES (2,3,'-34338496.525397047');
INSERT INTO num_exp_mul VALUES (2,3,'-147998901.44836127257');
INSERT INTO num_exp_div VALUES (2,3,'-7967167.56737750510440835266');
INSERT INTO num_exp_add VALUES (2,4,'-26539030.803497047');
INSERT INTO num_exp_sub VALUES (2,4,'-42137953.627297047');
INSERT INTO num_exp_mul VALUES (2,4,'-267821744976817.8111137106593');
INSERT INTO num_exp_div VALUES (2,4,'-4.40267480046830116685');
INSERT INTO num_exp_add VALUES (2,5,'-34322095.176906047');
INSERT INTO num_exp_sub VALUES (2,5,'-34354889.253888047');
INSERT INTO num_exp_mul VALUES (2,5,'-563049578578.769242506736077');
INSERT INTO num_exp_div VALUES (2,5,'-2094.18866914563535496429');
INSERT INTO num_exp_add VALUES (2,6,'-34244590.637766787');
INSERT INTO num_exp_sub VALUES (2,6,'-34432393.793027307');
INSERT INTO num_exp_mul VALUES (2,6,'-3224438592470.18449811926184222');
INSERT INTO num_exp_div VALUES (2,6,'-365.68599891479766440940');
INSERT INTO num_exp_add VALUES (2,7,'-117366977.215397047');
INSERT INTO num_exp_sub VALUES (2,7,'48689992.784602953');
INSERT INTO num_exp_mul VALUES (2,7,'2851072985828710.485883795');
INSERT INTO num_exp_div VALUES (2,7,'.41357483778485235518');
INSERT INTO num_exp_add VALUES (2,8,'-34263611.215397047');
INSERT INTO num_exp_sub VALUES (2,8,'-34413373.215397047');
INSERT INTO num_exp_mul VALUES (2,8,'-2571300635581.146276407');
INSERT INTO num_exp_div VALUES (2,8,'-458.57416721727870888476');
INSERT INTO num_exp_add VALUES (2,9,'-59265296.260444467');
INSERT INTO num_exp_sub VALUES (2,9,'-9411688.170349627');
INSERT INTO num_exp_mul VALUES (2,9,'855948866655588.453741509242968740');
INSERT INTO num_exp_div VALUES (2,9,'1.37757299946438931811');
INSERT INTO num_exp_add VALUES (3,0,'4.31');
INSERT INTO num_exp_sub VALUES (3,0,'4.31');
INSERT INTO num_exp_mul VALUES (3,0,'0');
INSERT INTO num_exp_div VALUES (3,0,'NaN');
INSERT INTO num_exp_add VALUES (3,1,'4.31');
INSERT INTO num_exp_sub VALUES (3,1,'4.31');
INSERT INTO num_exp_mul VALUES (3,1,'0');
INSERT INTO num_exp_div VALUES (3,1,'NaN');
INSERT INTO num_exp_add VALUES (3,2,'-34338487.905397047');
INSERT INTO num_exp_sub VALUES (3,2,'34338496.525397047');
INSERT INTO num_exp_mul VALUES (3,2,'-147998901.44836127257');
INSERT INTO num_exp_div VALUES (3,2,'-.00000012551512084352');
INSERT INTO num_exp_add VALUES (3,3,'8.62');
INSERT INTO num_exp_sub VALUES (3,3,'0');
INSERT INTO num_exp_mul VALUES (3,3,'18.5761');
INSERT INTO num_exp_div VALUES (3,3,'1.00000000000000000000');
INSERT INTO num_exp_add VALUES (3,4,'7799465.7219');
INSERT INTO num_exp_sub VALUES (3,4,'-7799457.1019');
INSERT INTO num_exp_mul VALUES (3,4,'33615678.685289');
INSERT INTO num_exp_div VALUES (3,4,'.00000055260225961552');
INSERT INTO num_exp_add VALUES (3,5,'16401.348491');
INSERT INTO num_exp_sub VALUES (3,5,'-16392.728491');
INSERT INTO num_exp_mul VALUES (3,5,'70671.23589621');
INSERT INTO num_exp_div VALUES (3,5,'.00026285234387695504');
INSERT INTO num_exp_add VALUES (3,6,'93905.88763026');
INSERT INTO num_exp_sub VALUES (3,6,'-93897.26763026');
INSERT INTO num_exp_mul VALUES (3,6,'404715.7995864206');
INSERT INTO num_exp_div VALUES (3,6,'.00004589912234457595');
INSERT INTO num_exp_add VALUES (3,7,'-83028480.69');
INSERT INTO num_exp_sub VALUES (3,7,'83028489.31');
INSERT INTO num_exp_mul VALUES (3,7,'-357852770.35');
INSERT INTO num_exp_div VALUES (3,7,'-.00000005190989574240');
INSERT INTO num_exp_add VALUES (3,8,'74885.31');
INSERT INTO num_exp_sub VALUES (3,8,'-74876.69');
INSERT INTO num_exp_mul VALUES (3,8,'322737.11');
INSERT INTO num_exp_div VALUES (3,8,'.00005755799201399553');
INSERT INTO num_exp_add VALUES (3,9,'-24926799.735047420');
INSERT INTO num_exp_sub VALUES (3,9,'24926808.355047420');
INSERT INTO num_exp_mul VALUES (3,9,'-107434525.43415438020');
INSERT INTO num_exp_div VALUES (3,9,'-.00000017290624149854');
INSERT INTO num_exp_add VALUES (4,0,'7799461.4119');
INSERT INTO num_exp_sub VALUES (4,0,'7799461.4119');
INSERT INTO num_exp_mul VALUES (4,0,'0');
INSERT INTO num_exp_div VALUES (4,0,'NaN');
INSERT INTO num_exp_add VALUES (4,1,'7799461.4119');
INSERT INTO num_exp_sub VALUES (4,1,'7799461.4119');
INSERT INTO num_exp_mul VALUES (4,1,'0');
INSERT INTO num_exp_div VALUES (4,1,'NaN');
INSERT INTO num_exp_add VALUES (4,2,'-26539030.803497047');
INSERT INTO num_exp_sub VALUES (4,2,'42137953.627297047');
INSERT INTO num_exp_mul VALUES (4,2,'-267821744976817.8111137106593');
INSERT INTO num_exp_div VALUES (4,2,'-.22713465002993920385');
INSERT INTO num_exp_add VALUES (4,3,'7799465.7219');
INSERT INTO num_exp_sub VALUES (4,3,'7799457.1019');
INSERT INTO num_exp_mul VALUES (4,3,'33615678.685289');
INSERT INTO num_exp_div VALUES (4,3,'1809619.81714617169373549883');
INSERT INTO num_exp_add VALUES (4,4,'15598922.8238');
INSERT INTO num_exp_sub VALUES (4,4,'0');
INSERT INTO num_exp_mul VALUES (4,4,'60831598315717.14146161');
INSERT INTO num_exp_div VALUES (4,4,'1.00000000000000000000');
INSERT INTO num_exp_add VALUES (4,5,'7815858.450391');
INSERT INTO num_exp_sub VALUES (4,5,'7783064.373409');
INSERT INTO num_exp_mul VALUES (4,5,'127888068979.9935054429');
INSERT INTO num_exp_div VALUES (4,5,'475.66281046305802686061');
INSERT INTO num_exp_add VALUES (4,6,'7893362.98953026');
INSERT INTO num_exp_sub VALUES (4,6,'7705559.83426974');
INSERT INTO num_exp_mul VALUES (4,6,'732381731243.745115764094');
INSERT INTO num_exp_div VALUES (4,6,'83.05996138436129499606');
INSERT INTO num_exp_add VALUES (4,7,'-75229023.5881');
INSERT INTO num_exp_sub VALUES (4,7,'90827946.4119');
INSERT INTO num_exp_mul VALUES (4,7,'-647577464846017.9715');
INSERT INTO num_exp_div VALUES (4,7,'-.09393717604145131637');
INSERT INTO num_exp_add VALUES (4,8,'7874342.4119');
INSERT INTO num_exp_sub VALUES (4,8,'7724580.4119');
INSERT INTO num_exp_mul VALUES (4,8,'584031469984.4839');
INSERT INTO num_exp_div VALUES (4,8,'104.15808298366741897143');
INSERT INTO num_exp_add VALUES (4,9,'-17127342.633147420');
INSERT INTO num_exp_sub VALUES (4,9,'32726265.456947420');
INSERT INTO num_exp_mul VALUES (4,9,'-194415646271340.1815956522980');
INSERT INTO num_exp_div VALUES (4,9,'-.31289456112403769409');
INSERT INTO num_exp_add VALUES (5,0,'16397.038491');
INSERT INTO num_exp_sub VALUES (5,0,'16397.038491');
INSERT INTO num_exp_mul VALUES (5,0,'0');
INSERT INTO num_exp_div VALUES (5,0,'NaN');
INSERT INTO num_exp_add VALUES (5,1,'16397.038491');
INSERT INTO num_exp_sub VALUES (5,1,'16397.038491');
INSERT INTO num_exp_mul VALUES (5,1,'0');
INSERT INTO num_exp_div VALUES (5,1,'NaN');
INSERT INTO num_exp_add VALUES (5,2,'-34322095.176906047');
INSERT INTO num_exp_sub VALUES (5,2,'34354889.253888047');
INSERT INTO num_exp_mul VALUES (5,2,'-563049578578.769242506736077');
INSERT INTO num_exp_div VALUES (5,2,'-.00047751189505192446');
INSERT INTO num_exp_add VALUES (5,3,'16401.348491');
INSERT INTO num_exp_sub VALUES (5,3,'16392.728491');
INSERT INTO num_exp_mul VALUES (5,3,'70671.23589621');
INSERT INTO num_exp_div VALUES (5,3,'3804.41728329466357308584');
INSERT INTO num_exp_add VALUES (5,4,'7815858.450391');
INSERT INTO num_exp_sub VALUES (5,4,'-7783064.373409');
INSERT INTO num_exp_mul VALUES (5,4,'127888068979.9935054429');
INSERT INTO num_exp_div VALUES (5,4,'.00210232958726897192');
INSERT INTO num_exp_add VALUES (5,5,'32794.076982');
INSERT INTO num_exp_sub VALUES (5,5,'0');
INSERT INTO num_exp_mul VALUES (5,5,'268862871.275335557081');
INSERT INTO num_exp_div VALUES (5,5,'1.00000000000000000000');
INSERT INTO num_exp_add VALUES (5,6,'110298.61612126');
INSERT INTO num_exp_sub VALUES (5,6,'-77504.53913926');
INSERT INTO num_exp_mul VALUES (5,6,'1539707782.76899778633766');
INSERT INTO num_exp_div VALUES (5,6,'.17461941433576102689');
INSERT INTO num_exp_add VALUES (5,7,'-83012087.961509');
INSERT INTO num_exp_sub VALUES (5,7,'83044882.038491');
INSERT INTO num_exp_mul VALUES (5,7,'-1361421264394.416135');
INSERT INTO num_exp_div VALUES (5,7,'-.00019748690453643710');
INSERT INTO num_exp_add VALUES (5,8,'91278.038491');
INSERT INTO num_exp_sub VALUES (5,8,'-58483.961509');
INSERT INTO num_exp_mul VALUES (5,8,'1227826639.244571');
INSERT INTO num_exp_div VALUES (5,8,'.21897461960978085228');
INSERT INTO num_exp_add VALUES (5,9,'-24910407.006556420');
INSERT INTO num_exp_sub VALUES (5,9,'24943201.083538420');
INSERT INTO num_exp_mul VALUES (5,9,'-408725765384.257043660243220');
INSERT INTO num_exp_div VALUES (5,9,'-.00065780749354660427');
INSERT INTO num_exp_add VALUES (6,0,'93901.57763026');
INSERT INTO num_exp_sub VALUES (6,0,'93901.57763026');
INSERT INTO num_exp_mul VALUES (6,0,'0');
INSERT INTO num_exp_div VALUES (6,0,'NaN');
INSERT INTO num_exp_add VALUES (6,1,'93901.57763026');
INSERT INTO num_exp_sub VALUES (6,1,'93901.57763026');
INSERT INTO num_exp_mul VALUES (6,1,'0');
INSERT INTO num_exp_div VALUES (6,1,'NaN');
INSERT INTO num_exp_add VALUES (6,2,'-34244590.637766787');
INSERT INTO num_exp_sub VALUES (6,2,'34432393.793027307');
INSERT INTO num_exp_mul VALUES (6,2,'-3224438592470.18449811926184222');
INSERT INTO num_exp_div VALUES (6,2,'-.00273458651128995823');
INSERT INTO num_exp_add VALUES (6,3,'93905.88763026');
INSERT INTO num_exp_sub VALUES (6,3,'93897.26763026');
INSERT INTO num_exp_mul VALUES (6,3,'404715.7995864206');
INSERT INTO num_exp_div VALUES (6,3,'21786.90896293735498839907');
INSERT INTO num_exp_add VALUES (6,4,'7893362.98953026');
INSERT INTO num_exp_sub VALUES (6,4,'-7705559.83426974');
INSERT INTO num_exp_mul VALUES (6,4,'732381731243.745115764094');
INSERT INTO num_exp_div VALUES (6,4,'.01203949512295682469');
INSERT INTO num_exp_add VALUES (6,5,'110298.61612126');
INSERT INTO num_exp_sub VALUES (6,5,'77504.53913926');
INSERT INTO num_exp_mul VALUES (6,5,'1539707782.76899778633766');
INSERT INTO num_exp_div VALUES (6,5,'5.72674008674192359679');
INSERT INTO num_exp_add VALUES (6,6,'187803.15526052');
INSERT INTO num_exp_sub VALUES (6,6,'0');
INSERT INTO num_exp_mul VALUES (6,6,'8817506281.4517452372676676');
INSERT INTO num_exp_div VALUES (6,6,'1.00000000000000000000');
INSERT INTO num_exp_add VALUES (6,7,'-82934583.42236974');
INSERT INTO num_exp_sub VALUES (6,7,'83122386.57763026');
INSERT INTO num_exp_mul VALUES (6,7,'-7796505729750.37795610');
INSERT INTO num_exp_div VALUES (6,7,'-.00113095617281538980');
INSERT INTO num_exp_add VALUES (6,8,'168782.57763026');
INSERT INTO num_exp_sub VALUES (6,8,'19020.57763026');
INSERT INTO num_exp_mul VALUES (6,8,'7031444034.53149906');
INSERT INTO num_exp_div VALUES (6,8,'1.25401073209839612184');
INSERT INTO num_exp_add VALUES (6,9,'-24832902.467417160');
INSERT INTO num_exp_sub VALUES (6,9,'25020705.622677680');
INSERT INTO num_exp_mul VALUES (6,9,'-2340666225110.29929521292692920');
INSERT INTO num_exp_div VALUES (6,9,'-.00376709254265256789');
INSERT INTO num_exp_add VALUES (7,0,'-83028485');
INSERT INTO num_exp_sub VALUES (7,0,'-83028485');
INSERT INTO num_exp_mul VALUES (7,0,'0');
INSERT INTO num_exp_div VALUES (7,0,'NaN');
INSERT INTO num_exp_add VALUES (7,1,'-83028485');
INSERT INTO num_exp_sub VALUES (7,1,'-83028485');
INSERT INTO num_exp_mul VALUES (7,1,'0');
INSERT INTO num_exp_div VALUES (7,1,'NaN');
INSERT INTO num_exp_add VALUES (7,2,'-117366977.215397047');
INSERT INTO num_exp_sub VALUES (7,2,'-48689992.784602953');
INSERT INTO num_exp_mul VALUES (7,2,'2851072985828710.485883795');
INSERT INTO num_exp_div VALUES (7,2,'2.41794207151503385700');
INSERT INTO num_exp_add VALUES (7,3,'-83028480.69');
INSERT INTO num_exp_sub VALUES (7,3,'-83028489.31');
INSERT INTO num_exp_mul VALUES (7,3,'-357852770.35');
INSERT INTO num_exp_div VALUES (7,3,'-19264149.65197215777262180974');
INSERT INTO num_exp_add VALUES (7,4,'-75229023.5881');
INSERT INTO num_exp_sub VALUES (7,4,'-90827946.4119');
INSERT INTO num_exp_mul VALUES (7,4,'-647577464846017.9715');
INSERT INTO num_exp_div VALUES (7,4,'-10.64541262725136247686');
INSERT INTO num_exp_add VALUES (7,5,'-83012087.961509');
INSERT INTO num_exp_sub VALUES (7,5,'-83044882.038491');
INSERT INTO num_exp_mul VALUES (7,5,'-1361421264394.416135');
INSERT INTO num_exp_div VALUES (7,5,'-5063.62688881730941836574');
INSERT INTO num_exp_add VALUES (7,6,'-82934583.42236974');
INSERT INTO num_exp_sub VALUES (7,6,'-83122386.57763026');
INSERT INTO num_exp_mul VALUES (7,6,'-7796505729750.37795610');
INSERT INTO num_exp_div VALUES (7,6,'-884.20756174009028770294');
INSERT INTO num_exp_add VALUES (7,7,'-166056970');
INSERT INTO num_exp_sub VALUES (7,7,'0');
INSERT INTO num_exp_mul VALUES (7,7,'6893729321395225');
INSERT INTO num_exp_div VALUES (7,7,'1.00000000000000000000');
INSERT INTO num_exp_add VALUES (7,8,'-82953604');
INSERT INTO num_exp_sub VALUES (7,8,'-83103366');
INSERT INTO num_exp_mul VALUES (7,8,'-6217255985285');
INSERT INTO num_exp_div VALUES (7,8,'-1108.80577182462841041118');
INSERT INTO num_exp_add VALUES (7,9,'-107955289.045047420');
INSERT INTO num_exp_sub VALUES (7,9,'-58101680.954952580');
INSERT INTO num_exp_mul VALUES (7,9,'2069634775752159.035758700');
INSERT INTO num_exp_div VALUES (7,9,'3.33089171198810413382');
INSERT INTO num_exp_add VALUES (8,0,'74881');
INSERT INTO num_exp_sub VALUES (8,0,'74881');
INSERT INTO num_exp_mul VALUES (8,0,'0');
INSERT INTO num_exp_div VALUES (8,0,'NaN');
INSERT INTO num_exp_add VALUES (8,1,'74881');
INSERT INTO num_exp_sub VALUES (8,1,'74881');
INSERT INTO num_exp_mul VALUES (8,1,'0');
INSERT INTO num_exp_div VALUES (8,1,'NaN');
INSERT INTO num_exp_add VALUES (8,2,'-34263611.215397047');
INSERT INTO num_exp_sub VALUES (8,2,'34413373.215397047');
INSERT INTO num_exp_mul VALUES (8,2,'-2571300635581.146276407');
INSERT INTO num_exp_div VALUES (8,2,'-.00218067233500788615');
INSERT INTO num_exp_add VALUES (8,3,'74885.31');
INSERT INTO num_exp_sub VALUES (8,3,'74876.69');
INSERT INTO num_exp_mul VALUES (8,3,'322737.11');
INSERT INTO num_exp_div VALUES (8,3,'17373.78190255220417633410');
INSERT INTO num_exp_add VALUES (8,4,'7874342.4119');
INSERT INTO num_exp_sub VALUES (8,4,'-7724580.4119');
INSERT INTO num_exp_mul VALUES (8,4,'584031469984.4839');
INSERT INTO num_exp_div VALUES (8,4,'.00960079113741758956');
INSERT INTO num_exp_add VALUES (8,5,'91278.038491');
INSERT INTO num_exp_sub VALUES (8,5,'58483.961509');
INSERT INTO num_exp_mul VALUES (8,5,'1227826639.244571');
INSERT INTO num_exp_div VALUES (8,5,'4.56673929509287019456');
INSERT INTO num_exp_add VALUES (8,6,'168782.57763026');
INSERT INTO num_exp_sub VALUES (8,6,'-19020.57763026');
INSERT INTO num_exp_mul VALUES (8,6,'7031444034.53149906');
INSERT INTO num_exp_div VALUES (8,6,'.79744134113322314424');
INSERT INTO num_exp_add VALUES (8,7,'-82953604');
INSERT INTO num_exp_sub VALUES (8,7,'83103366');
INSERT INTO num_exp_mul VALUES (8,7,'-6217255985285');
INSERT INTO num_exp_div VALUES (8,7,'-.00090187120721280172');
INSERT INTO num_exp_add VALUES (8,8,'149762');
INSERT INTO num_exp_sub VALUES (8,8,'0');
INSERT INTO num_exp_mul VALUES (8,8,'5607164161');
INSERT INTO num_exp_div VALUES (8,8,'1.00000000000000000000');
INSERT INTO num_exp_add VALUES (8,9,'-24851923.045047420');
INSERT INTO num_exp_sub VALUES (8,9,'25001685.045047420');
INSERT INTO num_exp_mul VALUES (8,9,'-1866544013697.195857020');
INSERT INTO num_exp_div VALUES (8,9,'-.00300403532938582735');
INSERT INTO num_exp_add VALUES (9,0,'-24926804.045047420');
INSERT INTO num_exp_sub VALUES (9,0,'-24926804.045047420');
INSERT INTO num_exp_mul VALUES (9,0,'0');
INSERT INTO num_exp_div VALUES (9,0,'NaN');
INSERT INTO num_exp_add VALUES (9,1,'-24926804.045047420');
INSERT INTO num_exp_sub VALUES (9,1,'-24926804.045047420');
INSERT INTO num_exp_mul VALUES (9,1,'0');
INSERT INTO num_exp_div VALUES (9,1,'NaN');
INSERT INTO num_exp_add VALUES (9,2,'-59265296.260444467');
INSERT INTO num_exp_sub VALUES (9,2,'9411688.170349627');
INSERT INTO num_exp_mul VALUES (9,2,'855948866655588.453741509242968740');
INSERT INTO num_exp_div VALUES (9,2,'.72591434384152961526');
INSERT INTO num_exp_add VALUES (9,3,'-24926799.735047420');
INSERT INTO num_exp_sub VALUES (9,3,'-24926808.355047420');
INSERT INTO num_exp_mul VALUES (9,3,'-107434525.43415438020');
INSERT INTO num_exp_div VALUES (9,3,'-5783481.21694835730858468677');
INSERT INTO num_exp_add VALUES (9,4,'-17127342.633147420');
INSERT INTO num_exp_sub VALUES (9,4,'-32726265.456947420');
INSERT INTO num_exp_mul VALUES (9,4,'-194415646271340.1815956522980');
INSERT INTO num_exp_div VALUES (9,4,'-3.19596478892958416484');
INSERT INTO num_exp_add VALUES (9,5,'-24910407.006556420');
INSERT INTO num_exp_sub VALUES (9,5,'-24943201.083538420');
INSERT INTO num_exp_mul VALUES (9,5,'-408725765384.257043660243220');
INSERT INTO num_exp_div VALUES (9,5,'-1520.20159364322004505807');
INSERT INTO num_exp_add VALUES (9,6,'-24832902.467417160');
INSERT INTO num_exp_sub VALUES (9,6,'-25020705.622677680');
INSERT INTO num_exp_mul VALUES (9,6,'-2340666225110.29929521292692920');
INSERT INTO num_exp_div VALUES (9,6,'-265.45671195426965751280');
INSERT INTO num_exp_add VALUES (9,7,'-107955289.045047420');
INSERT INTO num_exp_sub VALUES (9,7,'58101680.954952580');
INSERT INTO num_exp_mul VALUES (9,7,'2069634775752159.035758700');
INSERT INTO num_exp_div VALUES (9,7,'.30021990699995814689');
INSERT INTO num_exp_add VALUES (9,8,'-24851923.045047420');
INSERT INTO num_exp_sub VALUES (9,8,'-25001685.045047420');
INSERT INTO num_exp_mul VALUES (9,8,'-1866544013697.195857020');
INSERT INTO num_exp_div VALUES (9,8,'-332.88556569820675471748');
INSERT INTO num_exp_add VALUES (9,9,'-49853608.090094840');
INSERT INTO num_exp_sub VALUES (9,9,'0');
INSERT INTO num_exp_mul VALUES (9,9,'621345559900192.420120630048656400');
INSERT INTO num_exp_div VALUES (9,9,'1.00000000000000000000');
COMMIT TRANSACTION;
BEGIN TRANSACTION;
INSERT INTO num_exp_sqrt VALUES (0,'0');
INSERT INTO num_exp_sqrt VALUES (1,'0');
INSERT INTO num_exp_sqrt VALUES (2,'5859.90547836712524903505');
INSERT INTO num_exp_sqrt VALUES (3,'2.07605394920266944396');
INSERT INTO num_exp_sqrt VALUES (4,'2792.75158435189147418923');
INSERT INTO num_exp_sqrt VALUES (5,'128.05092147657509145473');
INSERT INTO num_exp_sqrt VALUES (6,'306.43364311096782703406');
INSERT INTO num_exp_sqrt VALUES (7,'9111.99676251039939975230');
INSERT INTO num_exp_sqrt VALUES (8,'273.64392922189960397542');
INSERT INTO num_exp_sqrt VALUES (9,'4992.67503899937593364766');
COMMIT TRANSACTION;
BEGIN TRANSACTION;
INSERT INTO num_exp_ln VALUES (0,'NaN');
INSERT INTO num_exp_ln VALUES (1,'NaN');
INSERT INTO num_exp_ln VALUES (2,'17.35177750493897715514');
INSERT INTO num_exp_ln VALUES (3,'1.46093790411565641971');
INSERT INTO num_exp_ln VALUES (4,'15.86956523951936572464');
INSERT INTO num_exp_ln VALUES (5,'9.70485601768871834038');
INSERT INTO num_exp_ln VALUES (6,'11.45000246622944403127');
INSERT INTO num_exp_ln VALUES (7,'18.23469429965478772991');
INSERT INTO num_exp_ln VALUES (8,'11.22365546576315513668');
INSERT INTO num_exp_ln VALUES (9,'17.03145425013166006962');
COMMIT TRANSACTION;
BEGIN TRANSACTION;
INSERT INTO num_exp_log10 VALUES (0,'NaN');
INSERT INTO num_exp_log10 VALUES (1,'NaN');
INSERT INTO num_exp_log10 VALUES (2,'7.53578122160797276459');
INSERT INTO num_exp_log10 VALUES (3,'.63447727016073160075');
INSERT INTO num_exp_log10 VALUES (4,'6.89206461372691743345');
INSERT INTO num_exp_log10 VALUES (5,'4.21476541614777768626');
INSERT INTO num_exp_log10 VALUES (6,'4.97267288886207207671');
INSERT INTO num_exp_log10 VALUES (7,'7.91922711353275546914');
INSERT INTO num_exp_log10 VALUES (8,'4.87437163556421004138');
INSERT INTO num_exp_log10 VALUES (9,'7.39666659961986567059');
COMMIT TRANSACTION;
BEGIN TRANSACTION;
INSERT INTO num_exp_power_10_ln VALUES (0,'NaN');
INSERT INTO num_exp_power_10_ln VALUES (1,'NaN');
INSERT INTO num_exp_power_10_ln VALUES (2,'224790267919917955.13261618583642653184');
INSERT INTO num_exp_power_10_ln VALUES (3,'28.90266599445155957393');
INSERT INTO num_exp_power_10_ln VALUES (4,'7405685069594999.07733999469386277636');
INSERT INTO num_exp_power_10_ln VALUES (5,'5068226527.32127265408584640098');
INSERT INTO num_exp_power_10_ln VALUES (6,'281839893606.99372343357047819067');
INSERT INTO num_exp_power_10_ln VALUES (7,'1716699575118597095.42330819910640247627');
INSERT INTO num_exp_power_10_ln VALUES (8,'167361463828.07491320069016125952');
INSERT INTO num_exp_power_10_ln VALUES (9,'107511333880052007.04141124673540337457');
COMMIT TRANSACTION;
BEGIN TRANSACTION;
INSERT INTO num_data VALUES (0, '0');
INSERT INTO num_data VALUES (1, '0');
INSERT INTO num_data VALUES (2, '-34338492.215397047');
INSERT INTO num_data VALUES (3, '4.31');
INSERT INTO num_data VALUES (4, '7799461.4119');
INSERT INTO num_data VALUES (5, '16397.038491');
INSERT INTO num_data VALUES (6, '93901.57763026');
INSERT INTO num_data VALUES (7, '-83028485');
INSERT INTO num_data VALUES (8, '74881');
INSERT INTO num_data VALUES (9, '-24926804.045047420');
COMMIT TRANSACTION;

-- ******************************
-- * Create indices for faster checks
-- ******************************

CREATE UNIQUE INDEX num_exp_add_idx ON num_exp_add (id1, id2);
CREATE UNIQUE INDEX num_exp_sub_idx ON num_exp_sub (id1, id2);
CREATE UNIQUE INDEX num_exp_div_idx ON num_exp_div (id1, id2);
CREATE UNIQUE INDEX num_exp_mul_idx ON num_exp_mul (id1, id2);
CREATE UNIQUE INDEX num_exp_sqrt_idx ON num_exp_sqrt (id);
CREATE UNIQUE INDEX num_exp_ln_idx ON num_exp_ln (id);
CREATE UNIQUE INDEX num_exp_log10_idx ON num_exp_log10 (id);
CREATE UNIQUE INDEX num_exp_power_10_ln_idx ON num_exp_power_10_ln (id);

VACUUM ANALYZE num_exp_add;
VACUUM ANALYZE num_exp_sub;
VACUUM ANALYZE num_exp_div;
VACUUM ANALYZE num_exp_mul;
VACUUM ANALYZE num_exp_sqrt;
VACUUM ANALYZE num_exp_ln;
VACUUM ANALYZE num_exp_log10;
VACUUM ANALYZE num_exp_power_10_ln;

-- ******************************
-- * Now check the behaviour of the NUMERIC type
-- ******************************

-- ******************************
-- * Addition check
-- ******************************
DELETE FROM num_result;
INSERT INTO num_result SELECT t1.id, t2.id, t1.val + t2.val
    FROM num_data t1, num_data t2;
SELECT t1.id1, t1.id2, t1.result, t2.expected
    FROM num_result t1, num_exp_add t2
    WHERE t1.id1 = t2.id1 AND t1.id2 = t2.id2
    AND t1.result != t2.expected;

DELETE FROM num_result;
INSERT INTO num_result SELECT t1.id, t2.id, round(t1.val + t2.val, 10)
    FROM num_data t1, num_data t2;
SELECT t1.id1, t1.id2, t1.result, round(t2.expected, 10) as expected
    FROM num_result t1, num_exp_add t2
    WHERE t1.id1 = t2.id1 AND t1.id2 = t2.id2
    AND t1.result != round(t2.expected, 10);

-- ******************************
-- * Subtraction check
-- ******************************
DELETE FROM num_result;
INSERT INTO num_result SELECT t1.id, t2.id, t1.val - t2.val
    FROM num_data t1, num_data t2;
SELECT t1.id1, t1.id2, t1.result, t2.expected
    FROM num_result t1, num_exp_sub t2
    WHERE t1.id1 = t2.id1 AND t1.id2 = t2.id2
    AND t1.result != t2.expected;

DELETE FROM num_result;
INSERT INTO num_result SELECT t1.id, t2.id, round(t1.val - t2.val, 40)
    FROM num_data t1, num_data t2;
SELECT t1.id1, t1.id2, t1.result, round(t2.expected, 40)
    FROM num_result t1, num_exp_sub t2
    WHERE t1.id1 = t2.id1 AND t1.id2 = t2.id2
    AND t1.result != round(t2.expected, 40);

-- ******************************
-- * Multiply check
-- ******************************
DELETE FROM num_result;
INSERT INTO num_result SELECT t1.id, t2.id, t1.val * t2.val
    FROM num_data t1, num_data t2;
SELECT t1.id1, t1.id2, t1.result, t2.expected
    FROM num_result t1, num_exp_mul t2
    WHERE t1.id1 = t2.id1 AND t1.id2 = t2.id2
    AND t1.result != t2.expected;

DELETE FROM num_result;
INSERT INTO num_result SELECT t1.id, t2.id, round(t1.val * t2.val, 30)
    FROM num_data t1, num_data t2;
SELECT t1.id1, t1.id2, t1.result, round(t2.expected, 30) as expected
    FROM num_result t1, num_exp_mul t2
    WHERE t1.id1 = t2.id1 AND t1.id2 = t2.id2
    AND t1.result != round(t2.expected, 30);

-- ******************************
-- * Division check
-- ******************************
DELETE FROM num_result;
INSERT INTO num_result SELECT t1.id, t2.id, t1.val / t2.val
    FROM num_data t1, num_data t2
    WHERE t2.val != '0.0';
SELECT t1.id1, t1.id2, t1.result, t2.expected
    FROM num_result t1, num_exp_div t2
    WHERE t1.id1 = t2.id1 AND t1.id2 = t2.id2
    AND t1.result != t2.expected;

DELETE FROM num_result;
INSERT INTO num_result SELECT t1.id, t2.id, round(t1.val / t2.val, 80)
    FROM num_data t1, num_data t2
    WHERE t2.val != '0.0';
SELECT t1.id1, t1.id2, t1.result, round(t2.expected, 80) as expected
    FROM num_result t1, num_exp_div t2
    WHERE t1.id1 = t2.id1 AND t1.id2 = t2.id2
    AND t1.result != round(t2.expected, 80);

-- ******************************
-- * Square root check
-- ******************************
DELETE FROM num_result;
INSERT INTO num_result SELECT id, 0, SQRT(ABS(val))
    FROM num_data;
SELECT t1.id1, t1.result, t2.expected
    FROM num_result t1, num_exp_sqrt t2
    WHERE t1.id1 = t2.id
    AND t1.result != t2.expected;

-- ******************************
-- * Natural logarithm check
-- ******************************
DELETE FROM num_result;
INSERT INTO num_result SELECT id, 0, LN(ABS(val))
    FROM num_data
    WHERE val != '0.0';
SELECT t1.id1, t1.result, t2.expected
    FROM num_result t1, num_exp_ln t2
    WHERE t1.id1 = t2.id
    AND t1.result != t2.expected;

-- ******************************
-- * Logarithm base 10 check
-- ******************************
DELETE FROM num_result;
INSERT INTO num_result SELECT id, 0, LOG(numeric '10', ABS(val))
    FROM num_data
    WHERE val != '0.0';
SELECT t1.id1, t1.result, t2.expected
    FROM num_result t1, num_exp_log10 t2
    WHERE t1.id1 = t2.id
    AND t1.result != t2.expected;

-- ******************************
-- * POWER(10, LN(value)) check
-- ******************************
DELETE FROM num_result;
INSERT INTO num_result SELECT id, 0, POWER(numeric '10', LN(ABS(round(val,200))))
    FROM num_data
    WHERE val != '0.0';
SELECT t1.id1, t1.result, t2.expected
    FROM num_result t1, num_exp_power_10_ln t2
    WHERE t1.id1 = t2.id
    AND t1.result != t2.expected;

-- ******************************
-- * miscellaneous checks for things that have been broken in the past...
-- ******************************
-- numeric AVG used to fail on some platforms
SELECT AVG(val) FROM num_data;
SELECT STDDEV(val) FROM num_data;
SELECT VARIANCE(val) FROM num_data;

-- Check for appropriate rounding and overflow
CREATE TABLE fract_only (id int, val numeric(4,4));
INSERT INTO fract_only VALUES (1, '0.0');
INSERT INTO fract_only VALUES (2, '0.1');
INSERT INTO fract_only VALUES (3, '1.0');	-- should fail
INSERT INTO fract_only VALUES (4, '-0.9999');
INSERT INTO fract_only VALUES (5, '0.99994');
INSERT INTO fract_only VALUES (6, '0.99995');  -- should fail
INSERT INTO fract_only VALUES (7, '0.00001');
INSERT INTO fract_only VALUES (8, '0.00017');
SELECT * FROM fract_only;
DROP TABLE fract_only;

-- Simple check that ceil(), floor(), and round() work correctly
CREATE TABLE ceil_floor_round (a numeric);
INSERT INTO ceil_floor_round VALUES ('-5.5');
INSERT INTO ceil_floor_round VALUES ('-5.499999');
INSERT INTO ceil_floor_round VALUES ('9.5');
INSERT INTO ceil_floor_round VALUES ('9.4999999');
INSERT INTO ceil_floor_round VALUES ('0.0');
INSERT INTO ceil_floor_round VALUES ('0.0000001');
INSERT INTO ceil_floor_round VALUES ('-0.000001');
SELECT a, ceil(a), ceiling(a), floor(a), round(a) FROM ceil_floor_round;
DROP TABLE ceil_floor_round;

-- Testing for width_bucket(). For convenience, we test both the
-- numeric and float8 versions of the function in this file.

-- errors
SELECT width_bucket(5.0, 3.0, 4.0, 0);
SELECT width_bucket(5.0, 3.0, 4.0, -5);
SELECT width_bucket(3.5, 3.0, 3.0, 888);
SELECT width_bucket(5.0::float8, 3.0::float8, 4.0::float8, 0);
SELECT width_bucket(5.0::float8, 3.0::float8, 4.0::float8, -5);
SELECT width_bucket(3.5::float8, 3.0::float8, 3.0::float8, 888);
SELECT width_bucket('NaN', 3.0, 4.0, 888);
SELECT width_bucket(0::float8, 'NaN', 4.0::float8, 888);

-- normal operation
CREATE TABLE width_bucket_test (operand_num numeric, operand_f8 float8);

COPY width_bucket_test (operand_num) FROM stdin;
-5.2
-0.0000000001
0.000000000001
1
1.99999999999999
2
2.00000000000001
3
4
4.5
5
5.5
6
7
8
9
9.99999999999999
10
10.0000000000001
\.

UPDATE width_bucket_test SET operand_f8 = operand_num::float8;

SELECT
    operand_num,
    width_bucket(operand_num, 0, 10, 5) AS wb_1,
    width_bucket(operand_f8, 0, 10, 5) AS wb_1f,
    width_bucket(operand_num, 10, 0, 5) AS wb_2,
    width_bucket(operand_f8, 10, 0, 5) AS wb_2f,
    width_bucket(operand_num, 2, 8, 4) AS wb_3,
    width_bucket(operand_f8, 2, 8, 4) AS wb_3f,
    width_bucket(operand_num, 5.0, 5.5, 20) AS wb_4,
    width_bucket(operand_f8, 5.0, 5.5, 20) AS wb_4f,
    width_bucket(operand_num, -25, 25, 10) AS wb_5,
    width_bucket(operand_f8, -25, 25, 10) AS wb_5f
    FROM width_bucket_test;

-- for float8 only, check positive and negative infinity: we require
-- finite bucket bounds, but allow an infinite operand
SELECT width_bucket(0.0::float8, 'Infinity'::float8, 5, 10); -- error
SELECT width_bucket(0.0::float8, 5, '-Infinity'::float8, 20); -- error
SELECT width_bucket('Infinity'::float8, 1, 10, 10),
       width_bucket('-Infinity'::float8, 1, 10, 10);

DROP TABLE width_bucket_test;

-- TO_CHAR()
--
SELECT '' AS to_char_1, to_char(val, '9G999G999G999G999G999')
	FROM num_data;

SELECT '' AS to_char_2, to_char(val, '9G999G999G999G999G999D999G999G999G999G999')
	FROM num_data;

SELECT '' AS to_char_3, to_char(val, '9999999999999999.999999999999999PR')
	FROM num_data;

SELECT '' AS to_char_4, to_char(val, '9999999999999999.999999999999999S')
	FROM num_data;

SELECT '' AS to_char_5,  to_char(val, 'MI9999999999999999.999999999999999')     FROM num_data;
SELECT '' AS to_char_6,  to_char(val, 'FMS9999999999999999.999999999999999')    FROM num_data;
SELECT '' AS to_char_7,  to_char(val, 'FM9999999999999999.999999999999999THPR') FROM num_data;
SELECT '' AS to_char_8,  to_char(val, 'SG9999999999999999.999999999999999th')   FROM num_data;
SELECT '' AS to_char_9,  to_char(val, '0999999999999999.999999999999999')       FROM num_data;
SELECT '' AS to_char_10, to_char(val, 'S0999999999999999.999999999999999')      FROM num_data;
SELECT '' AS to_char_11, to_char(val, 'FM0999999999999999.999999999999999')     FROM num_data;
SELECT '' AS to_char_12, to_char(val, 'FM9999999999999999.099999999999999') 	FROM num_data;
SELECT '' AS to_char_13, to_char(val, 'FM9999999999990999.990999999999999') 	FROM num_data;
SELECT '' AS to_char_14, to_char(val, 'FM0999999999999999.999909999999999') 	FROM num_data;
SELECT '' AS to_char_15, to_char(val, 'FM9999999990999999.099999999999999') 	FROM num_data;
SELECT '' AS to_char_16, to_char(val, 'L9999999999999999.099999999999999')	FROM num_data;
SELECT '' AS to_char_17, to_char(val, 'FM9999999999999999.99999999999999')	FROM num_data;
SELECT '' AS to_char_18, to_char(val, 'S 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 . 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9') FROM num_data;
SELECT '' AS to_char_19, to_char(val, 'FMS 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 . 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9') FROM num_data;
SELECT '' AS to_char_20, to_char(val, E'99999 "text" 9999 "9999" 999 "\\"text between quote marks\\"" 9999') FROM num_data;
SELECT '' AS to_char_21, to_char(val, '999999SG9999999999')			FROM num_data;
SELECT '' AS to_char_22, to_char(val, 'FM9999999999999999.999999999999999')	FROM num_data;
SELECT '' AS to_char_23, to_char(val, '9.999EEEE')				FROM num_data;

SELECT '' AS to_char_24, to_char('100'::numeric, 'FM999.9');
SELECT '' AS to_char_25, to_char('100'::numeric, 'FM999.');
SELECT '' AS to_char_26, to_char('100'::numeric, 'FM999');

-- TO_NUMBER()
--
SELECT '' AS to_number_1,  to_number('-34,338,492', '99G999G999');
SELECT '' AS to_number_2,  to_number('-34,338,492.654,878', '99G999G999D999G999');
SELECT '' AS to_number_3,  to_number('<564646.654564>', '999999.999999PR');
SELECT '' AS to_number_4,  to_number('0.00001-', '9.999999S');
SELECT '' AS to_number_5,  to_number('5.01-', 'FM9.999999S');
SELECT '' AS to_number_5,  to_number('5.01-', 'FM9.999999MI');
SELECT '' AS to_number_7,  to_number('5 4 4 4 4 8 . 7 8', '9 9 9 9 9 9 . 9 9');
SELECT '' AS to_number_8,  to_number('.01', 'FM9.99');
SELECT '' AS to_number_9,  to_number('.0', '99999999.99999999');
SELECT '' AS to_number_10, to_number('0', '99.99');
SELECT '' AS to_number_11, to_number('.-01', 'S99.99');
SELECT '' AS to_number_12, to_number('.01-', '99.99S');
SELECT '' AS to_number_13, to_number(' . 0 1-', ' 9 9 . 9 9 S');

--
-- Input syntax
--

CREATE TABLE num_input_test (n1 numeric);

-- good inputs
INSERT INTO num_input_test(n1) VALUES (' 123');
INSERT INTO num_input_test(n1) VALUES ('   3245874    ');
INSERT INTO num_input_test(n1) VALUES ('  -93853');
INSERT INTO num_input_test(n1) VALUES ('555.50');
INSERT INTO num_input_test(n1) VALUES ('-555.50');
INSERT INTO num_input_test(n1) VALUES ('NaN ');
INSERT INTO num_input_test(n1) VALUES ('        nan');

-- bad inputs
INSERT INTO num_input_test(n1) VALUES ('     ');
INSERT INTO num_input_test(n1) VALUES ('   1234   %');
INSERT INTO num_input_test(n1) VALUES ('xyz');
INSERT INTO num_input_test(n1) VALUES ('- 1234');
INSERT INTO num_input_test(n1) VALUES ('5 . 0');
INSERT INTO num_input_test(n1) VALUES ('5. 0   ');
INSERT INTO num_input_test(n1) VALUES ('');
INSERT INTO num_input_test(n1) VALUES (' N aN ');

SELECT * FROM num_input_test;

--
-- Test some corner cases for division
--

select 999999999999999999999::numeric/1000000000000000000000;
select div(999999999999999999999::numeric,1000000000000000000000);
select mod(999999999999999999999::numeric,1000000000000000000000);
select div(-9999999999999999999999::numeric,1000000000000000000000);
select mod(-9999999999999999999999::numeric,1000000000000000000000);
select div(-9999999999999999999999::numeric,1000000000000000000000)*1000000000000000000000 + mod(-9999999999999999999999::numeric,1000000000000000000000);
select mod (70.0,70) ;
select div (70.0,70) ;
select 70.0 / 70 ;
select 12345678901234567890 % 123;
select 12345678901234567890 / 123;
select div(12345678901234567890, 123);
select div(12345678901234567890, 123) * 123 + 12345678901234567890 % 123;
