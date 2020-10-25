#ifndef LZ4MT_XXH32_H
#define LZ4MT_XXH32_H

#include "xxhash.h"

namespace Lz4Mt {

class Xxh32 {
public:
	Xxh32(uint32_t seed);
	Xxh32(const void* input, int len, uint32_t seed);
	~Xxh32();
	bool update(const void* input, int len);
	uint32_t digest();

private:
	mutable std::mutex mut;
    std::unique_ptr<XXH32_state_t> st;
};

} // namespace Lz4Mt

#endif
