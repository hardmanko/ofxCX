#include "CX_EntryPoint.h"

/*
The idea behind the CX_DataFrame is that it is an easy way to
1) Store data from an experiment, and
2) Easily output that data to a spreadsheet-style file that can be used by analysis software.

This example covers the standard CX_DataFrame and what you can do with it. 
It includes a snippet of R code for reading the output of this program into an R data frame.

It also introduces CX_SafeDataFrame, which is generally a better 
option than CX_DataFrame because it prevents a lot of potential mistakes.
*/

void setupExperiment (void) {
	CX_DataFrame df;

	//Use the () notation to access an element at the given (column, row). Columns are named with strings and rows
	//are numbered. Due to some operator overloading, you
	//can just use operator= to set the values. Most of the common types are supported: int, double, string, etc. 
	//Even vectors are supported as long as the vector contains only basic types (std::string is NOT a basic type,
	//as it is part of the stl, but should work as long as there are no commas in the strings).
	df("dwellings", 1) = "house";
	df("ints", 0) = 3;
	df("ints", 1) = 42;
	df("vect", 0) = CX::intVector(3,1);
	df(0, "doubles") = 123.456; //You can do row first, if desired.
	//Everything is dynamically resized when using operator().

	//The contents of the data frame can be printed to a string, then used as you wish:
	string dataFrame = df.print(";"); //The semicolon delimiter makes it easy to see which cells have not been initialized.
	cout << dataFrame << endl;

	//Once stuff has been put into the data frame, you can extract it fairly easily:
	double d = df("doubles", 0); //The type is implicitly converted during extraction.
	int beast = df("ints", 0);
	vector<int> intvector = df("vect", 0); //You can extract vectors without very much effort.
	string house = df("dwellings", 1).toString(); //The common case that is tricky are strings, which require a special function call to be extracted.

	//You can see that what comes out looks like what went in:
	cout << endl << d << endl << beast << endl << house << endl << ofToString(intvector) << endl << endl;

	//Although you can use operator() to set data, it is somewhat safer to fill out a row of data at at time
	//(e.g. over the course of a trial) and append that row to the data frame.
	CX_DataFrameRow cellRow;
	cellRow["dwellings"] = "wigwam"; //CX_DataFrameRow is just a map<string, CX_DataFrameCell>, so access is done using operator[] and assignment.
	cellRow["ints"] = -7;
	cellRow["vect"] = CX::intVector(1, -1);
	//Notice that the "doubles" column is missing. This is handled silently.
	df.appendRow(cellRow);

	cout << df.print("\t") << endl; //Print again, this time with a tab delimiter

	//There are more complex ways to print that involve specifying which columns and rows are desired.	
	set<string> printCol;
	printCol.insert("dwellings");
	printCol.insert("vect");
	vector<unsigned int> printRow = CX::uintVector(0, 1);
	cout << df.print( printCol, printRow, ";" ); 

	//If you want to iterate over the contents of the data frame, use df.columnNames() and df.rowCount() to get the
	//the information needed. Rows in a CX_DataFrame always numbered from 0.

	vector<int> intVector = df.copyColumn<int>("ints"); //You can copy the data in a column out of the data frame, as long as you
		//know what type the data is in. In this case, what was put in was all convertable to int, so we'll pick that type.

	//operator[] is overloaded for the data frame. Using a string argument will return the column with that name.
	//Using an unsigned int argument will return that row. 
	CX_DataFrameRow row1 = df[1];
	string s1 = row1["dwellings"].toString();

	CX_DataFrameColumn dwellings = df["dwellings"];
	string s2 = dwellings[1].toString();

	string s3 = df["dwellings"][1].toString(); //operator[] can be chained because vectors and maps also use it.

	cout << "s1, s2, and s3: " << s1 << ", " << s2 << ", and " << s3 << endl;

	//When what is returned by operator[] is assigned to, the data frame itself gets updated. E.g.:
	//However, if data is added to the data frame in a way the causes the data frame to be resized after 
	//getting a row or column out of the data frame, this may fail spectacularly. E.g.:
	//df.appendRow(row1); //Don't do this sort of thing here.
	row1["ints"] = 666; 
	dwellings[0] = "castle";
	df["doubles"][2] = 3.14;

	//Fill in the last few empty cells before printing it
	df("doubles", 1) = 1.996;
	df("vect", 1) = CX::sequence(9, 5, -2);

	cout << endl << df.print() << endl;

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



	/*
	There is another kind of data frame for those who are not convinced that they are God's gift
	to programming and recognize that they can make errors. The normal CX_DataFrame allows
	you to screw up in a lot of ways. CX_SafeDataFrame is much safer to use, but has somewhat 
	restricted functionality.
	*/
	CX_SafeDataFrame sdf;

	CX_DataFrameRow row;
	row["str"] = "help";
	row["int"] = 4;
	sdf.appendRow(row); //The only way to add data to a CX_SafeDataFrame is CX_SafeDataFrame::appendRow().

	//sdf("int", 1) = 7; //Unlike CX_DataFrame, you cannot store data like this. This is commented out because it is a compile-time error.

	row.clear(); //Optional clearing to make sure that if one of the previously used columns was not assigned to, it would not stay around.

	row["str"] = "me";
	row["int"] = 7;
	sdf.appendRow(row);

	cout <<  endl << endl << sdf.print() << endl; //The print function is still available. You can also print to file in the same way.

	string help = sdf("str", 0).toString(); //You can read out data normally.
	int seven = sdf("int", 1);

	string outOfBounds = sdf("str", 2).toString(); //operator() will return an empty string if out of bounds. It will not resize the data frame.
	Log.flush(); //If you check the logs, there should be a warning about it as well.

	vector<int> intColumn = sdf.copyColumn<int>("int"); //You can still copy out vectors of converted data.

	//sdf["str"] //Both operator[] overloads are gone.

	sdf.rowCount(); //These are both still avalable.
	sdf.columnNames();
}

void updateExperiment (void) {
	//Do nothing
}