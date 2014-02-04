#include "CX_EntryPoint.h"

/*
The idea behind the CX_DataFrame is that it is a way to
1) Easily store data from an experiment using a clear, concise syntax, and
2) Easily output that data to a spreadsheet-style file that can be used by analysis software.

This example covers the standard CX_DataFrame and what you can do with it. 
It includes a snippet of R code for reading the output of this program into an R data frame.

CX_DataFrame is NOT for Doing arithmetic. Data is stored as a string internally so you have the 
potential for precision issues, plus it would be really, really slow.

It also introduces CX_SafeDataFrame, which is potentially a better option than CX_DataFrame because it prevents 
some potential mistakes by removing some of the parts of the interface for which it is difficult to develop a good
mental model.
*/


//See the contents of this header file (included with the dataFrame example) for an example of how you can use your
//own types (classes, structs) with CX_DataFrame in a way that will allow you to easily insert and extract data from
//the data frame.
//#include "myType.h"

void setupExperiment (void) {
	CX_DataFrame df;

	df.readFromFile("data frame.txt");
	df.deleteColumn("rowNumber");
	df.deleteRow(0);
	cout << df.print() << endl;
	
	//vector<ofPoint> locations1 = df("locations", 1);

	Log.flush();

	//Use the () notation to access an element at the given (column, row). Columns are named with strings and rows
	//are numbered. Due to some operator overloading, you
	//can just use operator= to set the values. Most of the common types are supported: int, double, string, etc. 
	//Even vectors are supported as long as the vector contains only basic types (std::string is NOT a basic type,
	//as it is part of the stl, but should work as long as there are no semicolons in the strings).
	df("ints", 0) = 3; //The integer 3 is put into the first row of the column named "ints"
	df("ints", 1) = 42; //Second row, same column...
	df("dwellings", 1) = "house"; //You don't have to start with the first row, everything is dynamically resized.
	df("vect", 0) = CX::sequence(3, 1, -1); //You can easily store vectors of data.
	df(0, "doubles") = 123.456; //You can do (row, column), if desired.

	//You can also use operator[] to access cells in the data frame. However, using (row,column) is faster (computationally) than using operator[].
	df["doubles"][1] = 1.996;
	df[1]["vect"] = CX::sequence(9, 5, -2);

	//The contents of the data frame can be printed to a string, then used as you wish:
	string dataFrame = df.print(";"); //The semicolon delimiter makes it easy to see which cells have not been initialized (a dwelling is missing).
	cout << "The initial data in the data frame: " << endl << dataFrame << endl;

	//Once stuff has been put into the data frame, you can extract it fairly easily:
	double d = df("doubles", 0); //The type is implicitly converted during extraction.
	int i = df["ints"][0]; //operator[] can also be used to read out data.
	vector<int> intvector = df("vect", 0); //You can extract vectors easily. You must specify what type the data should be converted to (in this case, <int>).
	string house = df("dwellings", 1).toString(); //The common case that is tricky are strings, which require a special function call to be extracted.

	//You can see that what comes out looks like what went in:
	cout << endl << "Some selected data: " << endl << d << endl << i << endl << house << endl << CX::vectorToString(intvector) << endl << endl;

	//Although you can use operator() to set data, it is somewhat safer to fill out a row of data at at time
	//(e.g. over the course of a trial) and append that row to the data frame because you don't have to worry about getting the row index right.
	CX_DataFrameRow cellRow;
	cellRow["dwellings"] = "wigwam"; //CX_DataFrameRow is just a map<string, CX_DataFrameCell>, so access is done using operator[] and assignment.
	cellRow["ints"] = -7;
	cellRow["vect"] = CX::intVector(-1, 1);
	//Notice that the "doubles" column is missing. This is handled silently.
	df.appendRow(cellRow);

	cout << endl << "With a row appended: " << endl << df.print("\t") << endl; //Print again, this time with a tab delimiter

	//There are more complex ways to print that involve specifying which columns and rows are desired.	
	set<string> printCol;
	printCol.insert("dwellings");
	printCol.insert("vect");
	vector<unsigned int> printRow = CX::intVector<unsigned int>(0, 1);
	cout << endl << "Only selected rows and columns: " << df.print( printCol, printRow, ";" ); 

	//If you want to iterate over the contents of the data frame, use df.columnNames() and df.getRowCount() to get the
	//the information needed. Rows in a CX_DataFrame are always numbered from 0.

	vector<int> intVector = df.copyColumn<int>("ints"); //You can copy the data in a column out of the data frame, as long as you
		//know what type the data is in. In this case, what was put in was all convertable to int, so we'll pick that type.

	//You can use operator[] to get either a row or column from the data frame. This row or column is not a copy
	//of the row or column from the data frame, but is linked to the data frame it came from. This means that
	//assigning to the row or column will affect the data in the original data frame.
	CX_DataFrameRow row1 = df[1];
	CX_DataFrameColumn dwellings = df["dwellings"];

	row1["ints"] = 666; 
	dwellings[0] = "castle";
	df["doubles"][2] = 3.14;

	//The way that CX_DataFrameRows work allows you to create new columns in the data frame
	//just by accessing that column in a row that has been pulled out of the data frame.
	row1["new"] = "This is new";

	//Print the final version of the data frame
	cout << endl << "Final version: " << df.print() << endl;

	//This shows the equivalence of various orders of the use of operator[].
	string s1 = row1["dwellings"].toString();
	string s2 = dwellings[1].toString();
	string s3 = df["dwellings"][1].toString();
	cout << "s1, s2, and s3: " << s1 << ", " << s2 << ", and " << s3 << endl;

	df.printToFile("myDataFrame.txt");

	/*
	//Data can easily be moved into an R data frame. First, output the data to a file.
	df.printToFile("[somewhere]/myDataFrame.txt"); //Check the return value for success.
	
	#Then read the data into R with:
	df = read.delim("[somewhere]/myDataFrame.txt")
	#That's it!

	#Here is an R helper function for getting data out of the ghetto vectors used by CX_DataFrame
	#x is a character or a factor element that can be converted with as.character().
	#This function only operates on the first element of x if it has length > 1.
	numericVector = function(x, delimiter=";") {
	  as.numeric(strsplit(as.character(x), split=delimiter, fixed=TRUE)[[1]])
	}

	numericVector(df$vect[1]) #Example use
	*/

	//You can copy rows from a data frame into a new data frame. You can specify which rows
	//to copy. You can even copy the same row multiple times.
	vector<CX_DataFrame::rowIndex_t> copyOrder = CX::intVector<CX_DataFrame::rowIndex_t>(2, 0);
	copyOrder.push_back(0); //Copy the first row twice.
	CX_DataFrame copyDf = df.copyRows(copyOrder);

	cout << endl << copyDf.print() << endl;

	//You can also copy columns out into a new data frame. Unlike rows, you cannot copy the same column multiple times.
	vector<string> columns;
	columns.push_back("dwellings");
	columns.push_back("ints");
	CX_DataFrame cols = df.copyColumns(columns);



	/*
	There is another kind of data frame for those who believe themselves to be error prone (i.e. self-aware
	people). The normal CX_DataFrame allows	you to make a number of errors having to do with indexing 
	(mispelled column name, invalid row index, thinking that a CX_DataFrameRow is a copy and not linked 
	to the data, etc.). CX_SafeDataFrame is much safer to use, but has somewhat restricted functionality.
	*/
	CX_SafeDataFrame sdf;

	CX_DataFrameRow row; //This is not linked to sdf. In order for data added to this row to get into sdf, you have to use appendRow().
	row["str"] = "nylon";
	row["int"] = 4;
	sdf.appendRow(row); //One good way to add data to a CX_SafeDataFrame is CX_SafeDataFrame::appendRow().

	//If you want to make more space in the data frame, you have to do it manually.
	sdf.setRowCount(2);
	sdf("int", 1) = 7; //Now you can store information in the new row.
	sdf("str", 1) = "steel";

	//You can also add new columns, but again, you must do it manually.
	sdf.addColumn("new");
	sdf("new", 0) = "newdata1";
	sdf("new", 1) = "newdata2";

	//The print function is still available. You can also print to files in the same way.
	cout << endl << "CX_SafeDataFrame contents: " << endl << sdf.print() << endl; 

	//You can read out data just like with a standard CX_DataFrame.
	string nylon = sdf("str", 0).toString(); 
	int seven = sdf("int", 1);

	//If you try to access any data that is out of bounds, you will get errors logged to Log.
	//operator() will return an empty string if out of bounds. It will not automatically resize a CX_SafeDataFrame.
	string outOfBounds = sdf("int", 2).toString();
	sdf("undefined", 1) = "error";
	Log.flush(); //If you check the logs, there should be an error about the out of bounds accesses.

	//You can also use CX_SafeDataFrame::at(), which throws an exception on out of bounds access. at() does not log errors, you have to catch the exception
	try {
		sdf.at("undefined", 4) = 5;
	} catch (std::exception& e) {
		cout << endl << e.what() << endl;
	}

	vector<int> intColumn = sdf.copyColumn<int>("int"); //You can still copy out vectors of converted data.

	//sdf["str"] //Both operator[] overloads are gone.

	sdf.getRowCount(); //These are both still avalable.
	sdf.columnNames();
}

void updateExperiment (void) {
	//Do nothing
}