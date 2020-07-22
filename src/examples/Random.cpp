#include "Random.h"

Random::Random() : Random(0)
{
}

Random::Random(int Seed)
{
	int ii;
	int mj, mk;

	//Initialize our Seed array.
	//This algorithm comes from Numerical Recipes in C (2nd Ed.)
	int subtraction = (Seed == std::numeric_limits<int>::min()) ? std::numeric_limits<int>::max() : std::abs(Seed);
	mj = MSEED - subtraction;
	SeedArray[55] = mj;
	mk = 1;
	for (int i = 1; i < 55; i++)
	{ //Apparently the range [1..55] is special (Knuth) and so we're wasting the 0'th position.
		ii = (21 * i) % 55;
		SeedArray[ii] = mk;
		mk = mj - mk;
		if (mk < 0)
		{
			mk += MBIG;
		}
		mj = SeedArray[ii];
	}
	for (int k = 1; k < 5; k++)
	{
		for (int i = 1; i < 56; i++)
		{
			SeedArray[i] -= SeedArray[1 + (i + 30) % 55];
			if (SeedArray[i] < 0)
			{
				SeedArray[i] += MBIG;
			}
		}
	}
	inext = 0;
	inextp = 21;
	Seed = 1;
}

double Random::Sample()
{
	//Including this division at the end gives us significantly improved
	//random number distribution.
	return (InternalSample() * (1.0 / MBIG));
}

int Random::InternalSample()
{
	int retVal;
	int locINext = inext;
	int locINextp = inextp;

	if (++locINext >= 56)
	{
		locINext = 1;
	}
	if (++locINextp >= 56)
	{
		locINextp = 1;
	}

	retVal = SeedArray[locINext] - SeedArray[locINextp];

	if (retVal == MBIG)
	{
		retVal--;
	}
	if (retVal < 0)
	{
		retVal += MBIG;
	}

	SeedArray[locINext] = retVal;

	inext = locINext;
	inextp = locINextp;

	return retVal;
}

int Random::Next()
{
	return InternalSample();
}

double Random::GetSampleForLargeRange()
{
	// The distribution of double value returned by Sample
	// is not distributed well enough for a large range.
	// If we use Sample for a range [Int32.MinValue..Int32.MaxValue)
	// We will end up getting even numbers only.

	int result = InternalSample();
	// Note we can't use addition here. The distribution will be bad if we do that.
	bool negative = (InternalSample() % 2 == 0) ? true : false; // decide the sign based on second sample
	if (negative)
	{
		result = -result;
	}
	double d = result;
	d += (std::numeric_limits<int>::max() - 1); // get a number in range [0 .. 2 * Int32MaxValue - 1)
	d /= 2 * static_cast<unsigned int>(std::numeric_limits<int>::max()) - 1;
	return d;
}

int Random::Next(int minValue, int maxValue)
{
	long long range = static_cast<long long>(maxValue) - minValue;
	if (range <= static_cast<long long>(std::numeric_limits<int>::max()))
	{
		return (static_cast<int>(Sample() * range) + minValue);
	}
	else
	{
		return static_cast<int>(static_cast<long long>(GetSampleForLargeRange() * range) + minValue);
	}
}

int Random::Next(int maxValue)
{
	return static_cast<int>(Sample() * maxValue);
}

double Random::NextDouble()
{
	return Sample();
}

void Random::NextBytes(std::vector<unsigned char>& buffer)
{
	if (buffer.empty())
	{
		throw std::invalid_argument("buffer");
	}
	
	for (int i = 0; i < buffer.size(); i++)
	{
		buffer[i] = static_cast<unsigned char>(InternalSample() % (std::numeric_limits<unsigned char>::max() + 1));
	}
}