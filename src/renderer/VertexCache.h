/*
MIT License

Copyright (c) 2017-2020 Markus Trenkwalder

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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