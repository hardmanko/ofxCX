/*
In order to use your own type with a CX_DataFrame you will have to define stream
insertion and stream extraction operators for your type. 

The stream insertion and extraction operators are operator<< and operator>>, respectively.

See below for an example of how to define these functions.

In this example, your type has a public data member, an int, and
a private data member, a float.

The end result will be that you can do this:

CX_DataFrame df;
myType mt(15, 1.357);
df("myType", 0) = mt;
myType dataCopy = df("myType", 0);

*/

#include <iostream>
#include <iomanip>

class myType {
public:
	myType(void) {}

	myType(int i_, float f) {
		i = i_;
		_f = f;
	}

	int i;

private:
	
	float _f;

	// By declaring the stream operators as friend, they will have access to private members of the class.
	// Otherwise _f would not be accessible because the stream operators are not member functions
	// of the class.
	friend std::ostream& operator<< (std::ostream& os, const myType& myt);
	friend std::istream& operator>> (std::istream& is, myType& myt);
};

// The stream insertion and extraction function bodies should usually be put into a .cpp 
// file, with only the declarations in the header. I used the inline keyword to get around this.
// You won't need (or want) the inline keyword in a .cpp file.

// Stream insertion operator
inline std::ostream& operator<<(std::ostream& os, const myType& myt) {

	// Insert the values of myt into the ostream with a comma-space delimiter. 
	// Comma-space is standard for oF stuff, but you can use anything you want
	// to, as long as you do the same thing on extraction.
	os << myt.i << ", " << myt._f; 

	return os; // Return the ostream so that other chained stream operations can use it.
}

// Stream extraction operator
inline std::istream& operator>>(std::istream& is, myType& myt) {
	// Extract data in the same order that it went in. 

	is >> myt.i; // Extract an int from the stream into myt.i.

	is.ignore(2); // Ignore the next two characters after the integer (the comma and space: ", ")

	is >> myt._f; // Get a float out of the istream.

	return is; // Return the istream so that other chained stream operations can use it.
}
