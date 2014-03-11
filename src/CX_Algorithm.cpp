#include "CX_Algorithm.h"

using namespace CX::Algo;


LatinSquare::LatinSquare(void) :
	_columns(0)
{}

LatinSquare::LatinSquare(unsigned int dimensions) {
	generate(dimensions);
}

/*! Generate a latin square with the given dimensions. This deletes any previous contents
of the latin square. */
void LatinSquare::generate(unsigned int dimensions) {
	square.resize(dimensions);
	_columns = dimensions;
	for (unsigned int i = 0; i < dimensions; i++) {
		square[i].resize(dimensions);
		for (unsigned int j = 0; j < dimensions; j++) {
			square[i][j] = (i + j) % dimensions;
		}
	}
}

/*! This function shifts the columns to the right and the last column is moved
to be the first column. */
void LatinSquare::reorderRight(void) {
	for (unsigned int i = 0; i < square.size(); i++) {
		for (unsigned int j = square[i].size() - 1; j > 0; j--) {
			std::swap(square[i][j], square[i][j - 1]);
		}
	}
}

/*! This function shifts the columns to the left and the first column is moved
to be the last column. */
void LatinSquare::reorderLeft(void) {
	for (unsigned int i = 0; i < square.size(); i++) {
		for (unsigned int j = 0; j < square[i].size() - 1; j++) {
			std::swap(square[i][j], square[i][j + 1]);
		}
	}
}

void LatinSquare::reorderUp(void) {
	for (unsigned int i = 0; i < square.size() - 1; i++) {
		std::swap(square[i], square[i + 1]);
	}
}

void LatinSquare::reorderDown(void) {
	for (unsigned int i = square.size() - 1; i > 0; i--) {
		std::swap(square[i], square[i - 1]);
	}
}

/*! Reverses the order of the columns in the latin square. */
void LatinSquare::reverseColumns(void) {
	std::vector< std::vector<unsigned int> > copy = square;
	for (unsigned int i = 0; i < square.size(); i++) {
		for (unsigned int j = 0, k = square[i].size() - 1; j < square[i].size(); j++, k--) {
			square[i][j] = copy[i][k];
		}
	}
}

/*! Reverses the order of the rows in the latin square. */
void LatinSquare::reverseRows(void) {
	std::vector< std::vector<unsigned int> > copy = square;
	for (unsigned int i = 0, j = rows() - 1; i < rows(); i++, j--) {
		square[i] = copy[j];
	}
}

/*! Swap the given columns. If either column is out of range, this function has no effect. */
void LatinSquare::swapColumns(unsigned int c1, unsigned int c2) {
	if (c1 >= columns() || c2 >= columns()) {
		return;
	}

	for (unsigned int i = 0; i < rows(); i++) {
		std::swap(square[i][c1], square[i][c2]);
	}
}

/*! Swap the given rows. If either row is out of range, this function has no effect. */
void LatinSquare::swapRows(unsigned int r1, unsigned int r2) {
	if (r1 >= rows() || r2 >= rows()) {
		return;
	}

	std::swap(square[r1], square[r2]);
}

/*! Appends another LatinSquare (ls) to the right of this one. If the number of 
rows of both latin squares is not equal, this has no effect and returns false. */
bool LatinSquare::appendRight(const LatinSquare& ls) {
	if (this->rows() != ls.rows()) {
		return false;
	}

	_columns += ls.square.front().size();

	for (unsigned int i = 0; i < rows(); i++) {
		for (unsigned int j = 0; j < ls.columns(); j++) {
			square[i].push_back(ls.square[i][j]);
		}
	}

	return true;
}

/*! Appends another LatinSquare (ls) below of this one. If the number of 
columns of both latin squares is not equal, this has no effect and returns false. */
bool LatinSquare::appendBelow(const LatinSquare& ls) {
	if (this->columns() != ls.columns()) {
		return false;
	}

	for (unsigned int i = 0; i < ls.rows(); i++) {
		square.push_back(ls.square[i]);
	}

	return true;
}

/*! Adds the given value to all of the values in the latin square. */
LatinSquare& LatinSquare::operator+=(unsigned int value) {
	for (unsigned int i = 0; i < rows(); i++) {
		for (unsigned int j = 0; j < columns(); j++) {
			square[i][j] += value;
		}
	}
	return *this;
}

/*! Prints the contents of the latin square to a string with the given delimiter between
elements of the latin square. */
std::string LatinSquare::print(std::string delim) {
	stringstream s;
	for (unsigned int i = 0; i < rows(); i++) {
		for (unsigned int j = 0; j < columns(); j++) {
			s << square[i][j];
			if (j != square[i].size() - 1) {
				s << delim;
			}
		}
		s << endl;
	}
	return s.str();
}

/*! Checks to make sure that the latin square held by this instance is a valid latin square. */
bool LatinSquare::validate(void) const {
	if (columns() != rows()) {
		return false;
	}

	vector<unsigned int> firstRow = square.front();
	std::sort(firstRow.begin(), firstRow.end());

	vector<unsigned int>::iterator logicalEnd = std::unique(firstRow.begin(), firstRow.end());
	if (logicalEnd != firstRow.end()) { //No duplicates allowed!
		return false;
	}

	for (unsigned int i = 1; i < rows(); i++) {
		vector<unsigned int> thisRow = square[i];
		std::sort(thisRow.begin(), thisRow.end());
		for (unsigned int j = 0; j < columns(); j++) {
			if (thisRow[j] != firstRow[j]) {
				return false;
			}
		}
	}

	for (unsigned int j = 0; j < columns(); j++) {
		vector<unsigned int> thisColumn = getColumn(j);
		std::sort(thisColumn.begin(), thisColumn.end());
		for (unsigned int i = 0; i < rows(); i++) {
			if (thisColumn[i] != firstRow[i]) {
				return false;
			}
		}
	}

	return true;
}

/*! Returns the number of columns. */
unsigned int LatinSquare::columns(void) const {
	return _columns;
}

/*! Returns the number of rows. */
unsigned int LatinSquare::rows(void) const {
	return square.size();
}

/*! Returns a copy of the given column. Throws std::out_of_range if the column is out of range. */
std::vector<unsigned int> LatinSquare::getColumn(unsigned int col) const {
	if (col >= columns()) {
		throw std::out_of_range("Latin square column index out of range.");
	}


	std::vector<unsigned int> column(rows());
	for (unsigned int i = 0; i < rows(); i++) {
		column[i] = square[i][col];
	}
	return column;
};

/*! Returns a copy of the given row. Throws std::out_of_range if the row is out of range. */
std::vector<unsigned int> LatinSquare::getRow(unsigned int row) const {
	if (row >= rows()) {
		throw std::out_of_range("Latin square row index out of range.");
	}

	return square[row];
}

