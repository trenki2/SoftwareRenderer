#pragma once

namespace swr {

class VertexCache {
private:
	static const int VertexCacheSize = 16;

	int inputIndex[VertexCacheSize];
	int outputIndex[VertexCacheSize];

public:
	VertexCache()
	{
		clear();
	}

	void clear()
	{
		for (size_t i = 0; i < VertexCacheSize; i++)
			inputIndex[i] = -1;
	}

	void set(int inIndex, int outIndex)
	{
		int cacheIndex = inIndex % VertexCacheSize;
		inputIndex[cacheIndex] = inIndex;
		outputIndex[cacheIndex] = outIndex;
	}

	int lookup(int inIndex) const
	{
		int cacheIndex = inIndex % VertexCacheSize;
		if (inputIndex[cacheIndex] == inIndex)
			return outputIndex[cacheIndex];
		else
			return -1;
	}
};

} // end namespace swr