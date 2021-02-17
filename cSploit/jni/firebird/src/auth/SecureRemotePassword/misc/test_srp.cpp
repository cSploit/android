#include "../auth/SecureRemotePassword/srp.h"

using namespace Auth;

int main(int argc, char** argv)
{
	Firebird::string salt;
#if SRP_DEBUG > 1
	BigInteger s("02E268803000000079A478A700000002D1A6979000000026E1601C000000054F");
#else
	BigInteger s;
	s.random(128);
#endif
	s.getText(salt);

	RemotePassword* server = new RemotePassword();
	RemotePassword* client = new RemotePassword();

	const char* account = "SYSDBA";
	const char* password = "masterkey";

	Firebird::UCharBuffer verifier;
	dumpIt("salt", salt);
#if SRP_DEBUG > 0
	fprintf(stderr, "%s %s\n", account, password);
#endif
	server->computeVerifier(account, salt, password).getBytes(verifier);
	dumpIt("verifier", verifier);

	Firebird::string clientPubKey, serverPubKey;
	client->genClientKey(clientPubKey);
	fprintf(stderr, "C Pub %d\n", clientPubKey.length());
	server->genServerKey(serverPubKey, verifier);
	fprintf(stderr, "S Pub %d\n", serverPubKey.length());

	Firebird::UCharBuffer key1, key2;
	client->clientSessionKey(key1, account, salt.c_str(), argc > 1 ? argv[1] : password, serverPubKey.c_str());
	server->serverSessionKey(key2, clientPubKey.c_str(), verifier);

	BigInteger cProof = client->clientProof(account, salt.c_str(), key1);
	BigInteger sProof = server->clientProof(account, salt.c_str(), key2);

	printf("%s\n", cProof == sProof ? "OK" : "differ");
}
