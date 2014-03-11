#include "CX_Algorithm.h"

using namespace CX::Algo;


LatinSquare::LatinSquare(void) :
	_columns(0)
{}

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

void LatinSquare::reorderRight(void) {
	for (unsigned int i = 0; i < square.size(); i++) {
		for (unsigned int j = square[i].size() - 1; j > 0; j--) {
			std::swap(square[i][j], square[i][j - 1]);
		}
	}
}

void LatinSquare::reorderLeft(void) {
	for (unsigned int i = 0; i < square.size(); i++) {
		for (unsigned int j = 0; j < square[i].size() - 1; j++) {
			std::swap(square[i][j], square[i][j + 1]);
		}
	}
}

void LatinSquare::reverseColumns(void) {
	std::vector< std::vector<unsigned int> > copy = square;
	for (unsigned int i = 0; i < square.size(); i++) {
		for (unsigned int j = 0, k = square[i].size() - 1; j < square[i].size(); j++, k--) {
			square[i][j] = copy[i][k];
		}
	}
}

void LatinSquare::swapColumns(unsigned int c1, unsigned int c2) {
	for (unsigned int i = 0; i < square.size(); i++) {
		std::swap(square[i][c1], square[i][c2]);
	}
}

bool LatinSquare::appendRight(const LatinSquare& ls) {
	if (this->square.size() != ls.square.size()) {
		return false;
	}

	_columns += ls.square.front().size();

	for (unsigned int i = 0; i < square.size(); i++) {
		for (unsigned int j = 0; j < ls.square[i].size(); j++) {
			square[i].push_back(ls.square[i][j]);
		}
	}

	return true;
}

bool LatinSquare::appendBelow(const LatinSquare& ls) {
	if (this->columns() != ls.columns()) {
		return false;
	}

	for (unsigned int i = 0; i < ls.rows(); i++) {
		square.push_back(ls.square[i]);
	}

	return true;
}

LatinSquare& LatinSquare::operator+=(unsigned int value) {
	for (unsigned int i = 0; i < rows(); i++) {
		for (unsigned int j = 0; j < columns(); j++) {
			square[i][j] += value;
		}
	}
	return *this;
}

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

unsigned int LatinSquare::columns(void) const {
	return _columns;
}

unsigned int LatinSquare::rows(void) const {
	return square.size();
}

std::vector<unsigned int> LatinSquare::getColumn(unsigned int col) const {
	std::vector<unsigned int> column(rows());
	for (unsigned int i = 0; i < rows(); i++) {
		column[i] = square[i].at(col);
	}
	return column;
};

std::vector<unsigned int> LatinSquare::getRow(unsigned int row) const {
	return square.at(row);
}

