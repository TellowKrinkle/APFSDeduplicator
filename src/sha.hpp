#pragma once

#include <array>
#include <vector>
#include <CommonCrypto/CommonDigest.h>

namespace apfsdedup {

struct SHA256Hash {
	std::array<uint8_t, CC_SHA256_DIGEST_LENGTH> data;

	inline SHA256Hash() = default;

	explicit inline SHA256Hash(const std::vector<uint8_t>& data);

	static SHA256Hash fromFile(const char *path);

	bool operator==(const SHA256Hash& other) const {
		return memcmp(this, &other, CC_SHA256_DIGEST_LENGTH) == 0;
	}

	friend std::ostream& operator<<(std::ostream& os, SHA256Hash const& hash);
};

class SHA256Hasher {
	CC_SHA256_CTX ctx;

public:
	inline SHA256Hasher() {
		CC_SHA256_Init(&ctx);
	}

	inline void add(const void *data, CC_LONG len) {
		CC_SHA256_Update(&ctx, data, len);
	}

	inline void add(const std::vector<uint8_t>& data) {
		add(data.data(), data.size());
	}

	[[ nodiscard ]]
	inline SHA256Hash finalize() {
		SHA256Hash out;
		finalizeInto(&out);
		return out;
	}

	inline void finalizeInto(SHA256Hash *out) {
		CC_SHA256_Final(out->data.data(), &ctx);
	}
};

inline SHA256Hash::SHA256Hash(const std::vector<uint8_t>& data) {
	SHA256Hasher hasher;
	hasher.add(data);
	hasher.finalizeInto(this);
}

} // namespace apfsdedup

namespace std {
	template<>
	struct hash<apfsdedup::SHA256Hash> {
		[[ nodiscard ]]
		size_t operator()(const apfsdedup::SHA256Hash& value) const {
			size_t out = 0;
			auto *data = reinterpret_cast<const size_t*>(value.data.data());
			for (int i = 0; i < CC_SHA256_DIGEST_LENGTH * sizeof(uint8_t) / sizeof(size_t); i++) {
				out ^= data[i];
			}
			return out;
		}
	};
}
