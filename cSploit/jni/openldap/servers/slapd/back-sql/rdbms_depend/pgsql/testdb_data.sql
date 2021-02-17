insert into institutes (id,name) values (1,'Example');

insert into persons (id,name,surname,password) values (1,'Mitya','Kovalev','mit');
insert into persons (id,name,surname) values (2,'Torvlobnor','Puzdoy');
insert into persons (id,name,surname) values (3,'Akakiy','Zinberstein');

insert into phones (id,phone,pers_id) values (1,'332-2334',1);
insert into phones (id,phone,pers_id) values (2,'222-3234',1);
insert into phones (id,phone,pers_id) values (3,'545-4563',2);

insert into documents (id,abstract,title) values (1,'abstract1','book1');
insert into documents (id,abstract,title) values (2,'abstract2','book2');

insert into authors_docs (pers_id,doc_id) values (1,1);
insert into authors_docs (pers_id,doc_id) values (1,2);
insert into authors_docs (pers_id,doc_id) values (2,1);

insert into referrals (id,name,url) values (1,'Referral','ldap://localhost:9012/');

insert into certs (id,cert,pers_id) values (1,decode('MIIDazCCAtSgAwIBAgIBAjANBgkqhkiG9w0BAQQFADB3MQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTEfMB0GA1UEChMWT3BlbkxEQVAgRXhhbXBsZSwgTHRkLjETMBEGA1UEAxMKRXhhbXBsZSBDQTEdMBsGCSqGSIb3DQEJARYOY2FAZXhhbXBsZS5jb20wHhcNMDMxMDE3MTYzMzE5WhcNMDQxMDE2MTYzMzE5WjB+MQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTEfMB0GA1UEChMWT3BlbkxEQVAgRXhhbXBsZSwgTHRkLjEYMBYGA1UEAxMPVXJzdWxhIEhhbXBzdGVyMR8wHQYJKoZIhvcNAQkBFhB1aGFtQGV4YW1wbGUuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDuxgp5ELV9LmhxWMpV7qc4028QQT3+zzFDXhruuXE7ji2n3S3ea8bOwDtJh+qnsDe561DhHHHlgIjMKCiDEizYMpxvJPYEXmvp0huRkMgpKZgmel95BSkt6TYmJ0erS3aoimOHLEFimmnTLolNRMiWqNBvqwobx940PGwUWEePKQIDAQABo4H/MIH8MAkGA1UdEwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5TU0wgR2VuZXJhdGVkIENlcnRpZmljYXRlMB0GA1UdDgQWBBSjI94TbBmuDEeUUOiC37EK0Uf0XjCBoQYDVR0jBIGZMIGWgBRLbyEaNiTSkPlDsFNHLX3hwOaYI6F7pHkwdzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlmb3JuaWExHzAdBgNVBAoTFk9wZW5MREFQIEV4YW1wbGUsIEx0ZC4xEzARBgNVBAMTCkV4YW1wbGUgQ0ExHTAbBgkqhkiG9w0BCQEWDmNhQGV4YW1wbGUuY29tggEAMA0GCSqGSIb3DQEBBAUAA4GBAIgUcARb3OlWYNbmr1nmqESuxLn16uqI1Ot6WkcICvpkdQ+Bo+R9AP05xpoXocZtKdNvBu3FNxB/jFkiOcLU2lX7Px1Ijnsjh60qVRy9HOsHCungIKlGcnXLKHmKu0y//5jds/HnaJsGcHI5JRG7CBJbW+wrwge3trJ1xHJI8prN','base64'),3);

