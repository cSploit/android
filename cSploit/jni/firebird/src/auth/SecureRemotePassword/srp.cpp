#include "firebird.h"
#include "gen/iberror.h"

#include "../auth/SecureRemotePassword/srp.h"
#include "../common/classes/alloc.h"
#include "../common/classes/ImplementHelper.h"
#include "../common/classes/init.h"
#include "../common/classes/fb_string.h"
#include "../common/sha.h"

using namespace Firebird;

namespace {
const char* primeStr =	"E67D2E994B2F900C3F41F08F5BB2627ED0D49EE1FE767A52EFCD565C"
						"D6E768812C3E1E9CE8F0A8BEA6CB13CD29DDEBF7A96D4A93B55D488D"
						"F099A15C89DCB0640738EB2CBDD9A8F7BAB561AB1B0DC1C6CDABF303"
						"264A08D1BCA932D1F1EE428B619D970F342ABA9A65793B8B2F041AE5"
						"364350C16F735F56ECBCA87BD57B29E7";
const char* genStr = "02";
} // anonumous namespace

namespace Auth {

class RemoteGroup
{
public:
	BigInteger	prime, generator, k;

	explicit RemoteGroup(Firebird::MemoryPool&)
		: prime(primeStr), generator(genStr), k()
	{
		Auth::Sha1 hash;

		hash.processInt(prime);
		if (prime.length() > generator.length())
		{
			unsigned int pad = prime.length() - generator.length();
			char pb[1024];
			memset(pb, 0, pad);

			hash.process(pad, pb);
		}
		hash.processInt(generator);

		hash.getInt(k);
	}

private:
	static InitInstance<RemoteGroup> group;

public:
	static RemoteGroup* getGroup()
	{
		return &group();
	}
};

InitInstance<RemoteGroup> RemoteGroup::group;

const char* RemotePassword::plugName = "Srp";

RemotePassword::RemotePassword()
	: group(RemoteGroup::getGroup())
{
#if SRP_DEBUG > 1
	privateKey = BigInteger("60975527035CF2AD1989806F0407210BC81EDC04E2762A56AFD529DDDA2D4393");
#else
	privateKey.random(RemotePassword::SRP_KEY_SIZE);
#endif
	privateKey %= group->prime;
}

BigInteger RemotePassword::getUserHash(const char* account, const char* salt, const char* password)
{
	hash.reset();
	hash.process(account);
	hash.process(":");
	hash.process(password);
	UCharBuffer hash1;
	hash.getHash(hash1);

	hash.reset();
	hash.process(salt);
	hash.process(hash1);
	BigInteger rc;
	hash.getInt(rc);

	return rc;
}

BigInteger RemotePassword::computeVerifier(const string& account, const string& salt, const string& password)
{
	BigInteger x(getUserHash(account.c_str(), salt.c_str(), password.c_str()));
	return group->generator.modPow(x, group->prime);
}

void RemotePassword::genClientKey(string& pubkey)
{
	dumpIt("privateKey(C)", privateKey);
	clientPublicKey = group->generator.modPow(privateKey, group->prime);
	clientPublicKey.getText(pubkey);
	dumpIt("clientPublicKey", clientPublicKey);
}

void RemotePassword::genServerKey(string& pubkey, const Firebird::UCharBuffer& verifier)
{
	dumpIt("privateKey(S)", privateKey);
	BigInteger gb(group->generator.modPow(privateKey, group->prime));	// g^b
	dumpIt("gb", gb);
	BigInteger v(verifier);												// v
	BigInteger kv = (group->k * v) % group->prime;
	dumpIt("kv", kv);
	serverPublicKey = (kv + gb) % group->prime;
	serverPublicKey.getText(pubkey);
	dumpIt("serverPublicKey", serverPublicKey);
}

void RemotePassword::computeScramble()
{
	hash.reset();
	dumpIt("computeScramble: clientPublicKey", clientPublicKey);
	hash.processStrippedInt(clientPublicKey);
	dumpIt("computeScramble: serverPublicKey", serverPublicKey);
	hash.processStrippedInt(serverPublicKey);
	hash.getInt(scramble);
}

void RemotePassword::clientSessionKey(UCharBuffer& sessionKey, const char* account,
									  const char* salt, const char* password,
									  const char* serverPubKey)
{
	serverPublicKey = BigInteger(serverPubKey);
	computeScramble();
	dumpIt("scramble", scramble);
	BigInteger x = getUserHash(account, salt, password);		// x
	BigInteger gx = group->generator.modPow(x, group->prime);	// g^x
	BigInteger kgx = (group->k * gx) % group->prime;			// kg^x
	dumpIt("kgx", kgx);
	BigInteger diff = (serverPublicKey - kgx) % group->prime;	// B - kg^x
	BigInteger ux = (scramble * x) % group->prime;				// ux
	BigInteger aux = (privateKey + ux) % group->prime;			// A + ux
	dumpIt("clientPrivateKey", privateKey);
	dumpIt("aux", aux);
	BigInteger sessionSecret = diff.modPow(aux, group->prime);	// (B - kg^x) ^ (a + ux)
	dumpIt("sessionSecret", sessionSecret);

	hash.reset();
	hash.processStrippedInt(sessionSecret);
	hash.getHash(sessionKey);
}

void RemotePassword::serverSessionKey(UCharBuffer& sessionKey, const char* clientPubKey,
									  const UCharBuffer& verifier)
{
	clientPublicKey = BigInteger(clientPubKey);
	computeScramble();
	dumpIt("scramble", scramble);
	BigInteger v = BigInteger(verifier);
	BigInteger vu = v.modPow(scramble, group->prime);					// v^u
	BigInteger Avu = (clientPublicKey * vu) % group->prime;				// Av^u
	dumpIt("Avu", Avu);
	BigInteger sessionSecret = Avu.modPow(privateKey, group->prime);	// (Av^u) ^ b
	dumpIt("serverPrivateKey", privateKey);
	dumpIt("sessionSecret", sessionSecret);

	hash.reset();
	hash.processStrippedInt(sessionSecret);
	hash.getHash(sessionKey);
}

// H(H(prime) ^ H(g), H(I), s, A, B, K)
BigInteger RemotePassword::clientProof(const char* account, const char* salt, const UCharBuffer& sessionKey)
{
	hash.reset();
	hash.processInt(group->prime);
	BigInteger n1;
	hash.getInt(n1);

	hash.reset();
	hash.processInt(group->generator);
	BigInteger n2;
	hash.getInt(n2);
	n1 = n1.modPow(n2, group->prime);

	hash.reset();
	hash.process(account);
	hash.getInt(n2);

	hash.reset();
	hash.processInt(n1);				// H(prime) ^ H(g)
	hash.processInt(n2);				// H(I)
	hash.process(salt);					// s
	hash.processInt(clientPublicKey);	// A
	hash.processInt(serverPublicKey);	// B
	hash.process(sessionKey);			// K

	BigInteger rc;
	hash.getInt(rc);
	return rc;
}

#if SRP_DEBUG > 0
void dumpIt(const char* name, const Firebird::UCharBuffer& data)
{
	fprintf(stderr, "%s\n", name);
	for (size_t x=0; x<data.getCount(); ++x)
		fprintf(stderr, "%02x ", data[x]);
	fprintf(stderr, "\n");
}

void dumpIt(const char* name, const Firebird::string& str)
{
	fprintf(stderr, "%s: '%s'\n", name, str.c_str());
}

void dumpBin(const char* name, const Firebird::string& str)
{
	fprintf(stderr, "%s (%ld)\n", name, str.length());
	for (size_t x = 0; x < str.length(); ++x)
		fprintf(stderr, "%02x ", str[x]);
	fprintf(stderr, "\n");
}

void dumpIt(const char* name, const BigInteger& bi)
{
	string x;
	bi.getText(x);
	dumpIt(name, x);
}
#endif

void checkStatusVectorForMissingTable(const ISC_STATUS* v)
{
	while (v[0] == isc_arg_gds)
	{
		if (v[1] == isc_dsql_relation_err)
		{
			Arg::Gds(isc_missing_data_structures).raise();
		}

		do
		{
			v += 2;
		} while (v[0] != isc_arg_warning && v[0] != isc_arg_gds && v[0] != isc_arg_end);
	}
}

} // namespace Auth
