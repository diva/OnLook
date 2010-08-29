// Copyright (c)2009 Thomas Shikami
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include <string>
#include <vector>

class EAscii85 {
private:
	EAscii85() { }
public:
	static std::string encode(const std::vector<unsigned char> &in);
	static std::vector<unsigned char> decode(const std::string &in);
};

class EAESEncrypt {
public:
	EAESEncrypt(const unsigned char *key, const unsigned char *iv);
	EAESEncrypt(const std::vector<unsigned char> &key, const std::vector<unsigned char> &iv);
	~EAESEncrypt();

	std::vector<unsigned char> encrypt(const std::string &in);

private:
	EAESEncrypt(const EAESEncrypt&) {}

	class EncryptImpl;

	EncryptImpl *mEncryptImpl;
};

class EAESDecrypt {
public:
	EAESDecrypt(const unsigned char *key, const unsigned char *iv);
	EAESDecrypt(const std::vector<unsigned char> &key, const std::vector<unsigned char> &iv);
	~EAESDecrypt();

	std::string decrypt(const std::vector<unsigned char> &in);

private:
	EAESDecrypt(const EAESDecrypt&) {}
	class DecryptImpl;

	DecryptImpl *mDecryptImpl;
};

class EGenKey {
public:
	EGenKey(const std::string &password, const unsigned char *salt = 0);
	~EGenKey();

	const unsigned char *key() const { return mKey; }
	const unsigned char *iv() const { return mIv; }

private:
	unsigned char mKey[16];
	unsigned char mIv[16];
};

class EDH {
public:
	EDH(const std::vector<unsigned char> &secret);
	EDH();
	~EDH();

	void secretTo(std::vector<unsigned char> &secret);
	void publicTo(std::vector<unsigned char> &pub);
	std::vector<unsigned char> computeKey(const std::vector<unsigned char> &other_pub);

private:
	EDH(const EDH&) {}

	class DHImpl;

	DHImpl *mDHImpl;
};

class EDSA {
public:
	EDSA(const std::vector<unsigned char> &secret);
	EDSA();
	~EDSA();

	std::vector<unsigned char> getPublic();
	void setPublic(const std::vector<unsigned char> &pub);
	std::vector<unsigned char> generateSecret();
	static bool verify(const std::vector<unsigned char> &dgst, const std::vector<unsigned char> &sig, const std::vector <unsigned char> &pub);
	bool verify(const std::vector<unsigned char> &dgst, const std::vector<unsigned char> &sig);
	std::vector<unsigned char> sign(const std::vector<unsigned char> &dgst);

private:
	EDSA(const EDSA&) {}

	class DSAImpl;

	DSAImpl *mDSAImpl;
};
