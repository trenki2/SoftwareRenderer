#pragma once

#include <vector>
#include <cmath>
#include <limits>
#include <stdexcept>

//C# TO C++ CONVERTER NOTE: The following .NET attribute has no direct equivalent in C++:
//ORIGINAL LINE: [Serializable] public class Random
class Random
{
	  //
	  // Private Constants 
	  //
  private:
	  static constexpr int MBIG = std::numeric_limits<int>::max();
	  static constexpr int MSEED = 161803398;
	  static constexpr int MZ = 0;


	  //
	  // Member Variables
	  //
	  int inext = 0;
	  int inextp = 0;
	  std::vector<int> SeedArray = std::vector<int>(56);

	  //
	  // Public Constants
	  //

	  //
	  // Native Declarations
	  //

	  //
	  // Constructors
	  //

  public:
	  Random();

	  Random(int Seed);

	  //
	  // Package Private Methods
	  //

	  /*====================================Sample====================================
	  **Action: Return a new random number [0..1) and reSeed the Seed array.
	  **Returns: A double [0..1)
	  **Arguments: None
	  **Exceptions: None
	  ==============================================================================*/
  protected:
	  virtual double Sample();

  private:
	  int InternalSample();

	  //
	  // Public Instance Methods
	  // 


	  /*=====================================Next=====================================
	  **Returns: An int [0..Int32.MaxValue)
	  **Arguments: None
	  **Exceptions: None.
	  ==============================================================================*/
  public:
	  virtual int Next();

  private:
	  double GetSampleForLargeRange();


	  /*=====================================Next=====================================
	  **Returns: An int [minvalue..maxvalue)
	  **Arguments: minValue -- the least legal value for the Random number.
	  **           maxValue -- One greater than the greatest legal return value.
	  **Exceptions: None.
	  ==============================================================================*/
  public:
	  virtual int Next(int minValue, int maxValue);


	  /*=====================================Next=====================================
	  **Returns: An int [0..maxValue)
	  **Arguments: maxValue -- One more than the greatest legal return value.
	  **Exceptions: None.
	  ==============================================================================*/
	  virtual int Next(int maxValue);


	  /*=====================================Next=====================================
	  **Returns: A double [0..1)
	  **Arguments: None
	  **Exceptions: None
	  ==============================================================================*/
	  virtual double NextDouble();


	  /*==================================NextBytes===================================
	  **Action:  Fills the byte array with random bytes [0..0x7f].  The entire array is filled.
	  **Returns:Void
	  **Arugments:  buffer -- the array to be filled.
	  **Exceptions: None
	  ==============================================================================*/
	  virtual void NextBytes(std::vector<unsigned char> &buffer);
};
