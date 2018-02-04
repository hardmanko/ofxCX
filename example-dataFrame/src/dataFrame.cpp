#include "CX.h"

/* This example covers the CX_DataFrame and what you can do with it. 
It includes a snippet of R code for reading the output of this program into an R data frame.

The idea behind the CX_DataFrame is that it is a way to
1) Easily store data from an experiment using a clear, concise syntax, and
2) Easily output that data to a spreadsheet-style file that can be used by analysis software.

You should not use a CX_DataFrame as part of a series of calculations. Data is stored as a 
string internally so it would be really, really slow because every time data is stored or retrieved, 
it has to be converted to/from a string.

See the contents of the "myType.h" header file (included with the dataFrame example) for an example of how you can use your
own types (classes, structs) with CX_DataFrame in a way that will allow you to easily insert and extract data from
the data frame.
*/

void runExperiment(void) {

	//The CX_DataFrame that will be used for much of this example
	CX_DataFrame df;

	//Use the () notation to access an element at the given (column, row). 
	//Columns are named with strings and rows are numbered. 
	//You can just use = to set the values of cells. Lots of types are supported: int, double, string, etc. 
	df("double", 0) = 3.14; //The double 3.14 is put into the first row of the column named "double"
	df("double", 1) = 1.5; //Second row, same column...

	//You don't have to start with the first (0th) row because CX_DataFrames are dynamically resized.
	df("dwellings", 1) = "house";

	//You can easily store vectors of data.
	df("vect", 0) = CX::Util::sequence<int>(1, 3, 1); //Store the vector {1, 2, 3}
	df(1, "vect") = CX::Util::sequence<int>(9, 5, -2); //You can index by (row, column) if desired.

	//The contents of the data frame can be printed to a string, then used as you wish:
	string dataFrameString = df.print(" / "); //The forward-slash delimiter makes it easy to see which cells have not been initialized (a dwelling is missing).
	cout << "The initial data in the data frame: " << endl << dataFrameString << endl;

	Log.flush();

	//Once stuff has been put into the data frame, you can extract it out again
	double d = df("double", 0); //The type is implicitly converted during extraction as long as you are assigning the value to something.

	//In this case, a warning will be generated because the input data was a double, but the extracted value was an int.
	int whoops = df("double", 1);

	//You can extract vectors easily.
	vector<int> intVector = df("vect", 1);

	//The one thing that is tricky to extract are strings, which require a function call to be extracted
	string house = df("dwellings", 1).toString(); //which is ironic, given that the data is stored as a string internally...

	//To explicitly extract a particular type of data, use to<T>()
	double explicitDouble = df("double", 2).to<double>();

	//For vectors of data, use toVector<T>(). If the cell contains a scalar, toVector will return a 1-length vector.
	vector<int> explicitVector = df("vect", 0).toVector<int>();

	//You can see that what comes out looks like what went in (with the exception of the double that was converted to an int):
	cout << endl << "Some selected data: " << endl <<
		d << endl <<
		whoops << endl <<
		house << endl <<
		CX::Util::vectorToString(intVector, ";") << endl << endl;

	Log.flush();

	//Although you can use operator() to set data, it is also possible to fill out a row of data at at time
	//(e.g. over the course of a trial) and append that row to the data frame. Using rows is nice because you 
	//don't have to worry about getting the row index right.
	CX_DataFrameRow newRow;
	newRow["dwellings"] = "wigwam"; //With a CX_DataFrameRow, you access the columns with operator[].
	newRow["double"] = -200;
	//Notice that the "vect" column is missing in this row. This is fine, it will just have an empty cell.
	df.appendRow(newRow);

	cout << endl << "With a row appended: " << endl << df.print("\t") << endl; //Print again, this time with a tab delimiter

	Log.flush();

	//When you access a nonexistent row/column combination with operator(), it is created for you.
	//If instead you would like to get an exception telling you that you are out of bounds and 
	//no newly-created cell, use CX_DataFrame::at().
	try {
		double moo2 = df.at("moo", 2);
	} catch (std::out_of_range& e) {
		cout << "Exception caught: " << e.what() << endl;
		Log.flush();
	}


	//There are more complex ways to print that involve specifying which columns and/or rows are desired and more...
	CX_DataFrame::OutputOptions oOpt;

	oOpt.cellDelimiter = "\t"; //Delimiter between cells
	oOpt.printRowNumbers = false;
	oOpt.vectorElementDelimiter = ";"; //Delimiter between elements of a vector.
	oOpt.vectorEncloser = "\""; //This example uses double-quote around vector elements.

	//Only print these two columns
	oOpt.columnsToPrint.push_back("dwellings");
	oOpt.columnsToPrint.push_back("vect");

	//Only print rows 0 and 1.
	oOpt.rowsToPrint = { 0, 1 };

	cout << endl << "Only selected rows and columns: " << endl << df.print(oOpt);


	//If you want to iterate over the contents of the data frame, use getColumnNames() and getRowCount() to get the
	//the information needed. Rows in a CX_DataFrame are always numbered starting from 0.
	for (std::string& col : df.getColumnNames()) {
		for (unsigned int i = 0; i < df.getRowCount(); i++) {
			// Do something...

			//df(col, i).clear();
		}
	}


	vector<double> dVect = df.copyColumn<double>("double"); //You can copy the data in a column out of the data frame, as long as you
		//know what type the data is in. In this case, what was put in was all convertable to double, so we'll pick that type.

	//You can use operator[] to get either a row or column from the data frame. This row or column is not a copy
	//of the row or column from the data frame, but is linked to the data frame it came from. This means that
	//assigning to the row or column will affect the data in the original data frame.
	CX_DataFrameRow row1 = df[1]; //A numeric index gets you a row.
	CX_DataFrameColumn dwellings = df["dwellings"]; //A string index gets you a column.

	row1["double"] = 6.66;
	dwellings[0] = "castle";

	//Because a CX_DataFrameRow represents a row of a data frame, you can create new columns
	//in the data frame by creating a new column in the row.
	row1["new"] = "This is new";

	//Print the final version of the data frame
	cout << endl << "Final version: " << endl << df.print() << endl;


	/*
	//Data can easily be moved into an R data frame. First, output the data to a file, using some delimiter (tab, in this case):
	df.printToFile("[somewhere]/myDataFrame.txt", "\t");
	
	Then read the data into R with read.delim, specifying the same delimiter:

	df = read.delim("[somewhere]/myDataFrame.txt", sep="\t")

	You should have the same data in R that you had in CX. 
	Note, however, that R data frames can't store vectors in a single cell like CX_DataFrame can. 
	See CX_DataFrame::convertAllVectorColumnsToMultipleColumns() to flatten the vectors into multiple columns.
	You can read the data into Excel as well, by reading it in as a delimited file (or dragging and dropping).

	Let's print to data frame to a file so that you can experiment with it.
	*/
	df.printToFile("myDataFrame.txt", "\t");

	//You can also read data back into a CX_DataFrame from file.
	CX_DataFrame readCopy;
	readCopy.readFromFile("myDataFrame.txt", "\t"); //Note that you have to get the delimiter right.

	cout << "Data frame read in from file: " << endl << readCopy.print() << endl;


	//You can copy rows from a data frame into a new data frame. You can specify which rows
	//to copy and the order in which they are copied.
	// Copy rows 2, 1, 0, and 1 (copying row 1 twice).
	vector<CX_DataFrame::rowIndex_t> copyOrder = { 2, 1, 0, 1 };
	CX_DataFrame copyDf = df.copyRows(copyOrder);

	cout << endl << "Copy of the read in data frame: " << endl << copyDf.print() << endl;

	//You can also copy columns out into a new data frame. Unlike rows, you cannot copy the same column multiple times
	//because then more than one column would have the same name.
	vector<string> columns = { "dwellings", "double" };
	CX_DataFrame cols = df.copyColumns(columns);

	Log.flush(); //Check to see if any errors occured during the running of this example.


	//Draw some stuff to finish the example
	Disp.setWindowResolution(500, 100);
	Disp.beginDrawingToBackBuffer();
	ofBackground(0);
	ofSetColor(255);
	ofDrawBitmapString("Examine the console for various printouts that \ncorrespond to certain parts of the code.\n\n"
					   "Press any key to exit.", ofPoint(20, 20));
	Disp.endDrawingToBackBuffer();
	Disp.swapBuffers();

	Input.Keyboard.waitForKeypress(-1);
}