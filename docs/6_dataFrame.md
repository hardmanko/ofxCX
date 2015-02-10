Storing and Outputting Data {#dataFrameTutorial}
===========================

Overview
--------

The purpose of having participants do tasks is to get data from them. CX provides the CX_DataFrame as a mechanism for storing data in a structured way and outputting that data for further processing.  

The structure of a data frame is essentially the same as a spreadsheet. There are some number of rows, each with a numeric index, and some number of columns, each of which has a unique name. At each combination of row and column, one cell of data is stored.

The type of data stored within each cell is independent of the type of data in other cells of the data frame. However, generally the type of data within a column will be the same for the whole column. Many different types of data can be stored in a CX_DataFrame, the specifics of which will be described later.

In addition to being able to store arbitrary types of data in each cell, a CX_DataFrame can store an arbitrary number of items in each cell. As such, a CX_DataFrame is rectangular in the row and column dimensions, but jagged in the third dimension of cell vectors. This is useful in cases where cells in a column of data are each a logical unit but can vary in length from row to row (perhaps from trial to trial). Without the ability to have a vector of data within a cell, several columns would have to be used instead. To maintain backwards compatibility with spreadsheet software that cannot store vectors of data within a single cell, CX_DataFrame::convertAllVectorColumnsToMultipleColumns() does what its name says.

All of the data that is given to a CX_DataFrame is converted to and stored as a string. If the data is extracted out of the CX_DataFrame, it is converted from a string back to the requested type. This conversion process is slow, so CX_DataFrames are not suited for computational tasks in which data is repeatedly retrieved and stored in the data frame. In addition, it is possible for floating point data to lose precision when converted to a string, although the loss of precision should be insignificant and, by default, there should be no loss of precision with floating point types.

Once an experiment is complete, the contents of a CX_DataFrame can be printed to a file in a delimited format (tab-delimited by default). Delimited data can be read by pretty much any software that could be used to process it (e.g. R using read.delim, Excel, etc.).


Usage
-----

Using a CX_DataFrame is fairly straightforward. To start, we need to create a CX_DataFrame and put stuff into it.

~~~{.cpp}
CX_DataFrame df;
df("double", 0) = 3.14;
df("double", 1) = 1.5;
~~~

On the first line, we created a CX_DataFrame named `df`. On the next line, we start putting in data. Cells of the data frame can be accessed by using operator(), with the user providing a column name (a string) and a row index (an unsigned integer). We access the cell in the column named "double" at row 0 and assign the value 3.14 to it. Then, we do the same for the second row of the "double" column and assign the value 0.5 to it.

Notice that we did not need to tell the data frame that we wanted to make a new column with 2 rows, it did it for us. CX_DataFrames automatically resize themselves to fit new data when you use operator(). 


We can make other columns with other kinds of data in them. For example, we could make a column with the names of some types of dwellings in it.
~~~{.cpp}
df("dwellings", 1) = "house";
~~~
Instead of the `double`s that we used earlier, these values are strings, which works just fine. Note that we skipped over row 0 for the "dwellings" column, which means that there will be an empty cell in row 0. In addition to being able to store a variety of data types, data frames can also store vectors of data.

~~~{.cpp}
df("vect", 0) = CX::Util::sequence(1, 3, 1); 
df(1, "vect") = CX::Util::sequence(9, 5, -2);
~~~

In the first case, we assigned a vector containing {1, 2, 3} to a cell. In the second case, we did a similar thing, except note that the row index and column name were given in reverse order. Now that we have some data in the data frame, let's look at the data. We can print the contents of a data frame to a string and then print that string to the console.

~~~{.cpp}
string dataFrameString = df.print("/");
cout << "The initial data in the data frame: " << endl << dataFrameString << endl;
~~~
First, the contents of `df` are printed with a forward-slash used as the delimiter between cells (the delimiter between the elements of the vectors are left at the default).

You can also extract data from a CX_DataFrame. This is done by reversing which side of the assignment operator the data frame and data appear on.

~~~{.cpp}
double d = df("double", 0);
int whoops = df("double", 1);
~~~

Here we read out some values. In the first case, it is a `double` that is being assigned to, so the contents of the cell in the data frame are implicitly converted to `double`. The second line has a programming error on it: We originally put in the value `1.5`, but now we are saying that we want an `int` out, which is not the same type as the value that we put in, which was `double`. In this case, a best-effort attempt is made to give you data of the requested type, but a warning will be logged. The warning will tell you that there is a mismatch between the type of data that was inserted and the type that was extracted. If you notice this in the logs, take care! You could have a serious problem on your hands. Moving on to extracting more types of data:

~~~{.cpp}
vector<int> intVector = df("vect", 1);
~~~
You can just put a vector of some type of the left hand side of the assignment operator and the data will be converted to that type. If you only put in a single value, but ask for a vector, you will get a vector of length 1. To extract strings, you need to explicitly request a string:
~~~{.cpp}
string house = df("dwellings", 1).toString();
~~~
There is no good reason why you should need to call a function to get a string out, but there is an issue extracting strings that I have not been able to work around. It is possible to explicitly specify which type of data you want to extract by using \ref CX::CX_DataFrameCell::to<T>() "to<T>()", replacing `T` with the type of your choice. In addition, if you want a vector of data from a cell, you can use \ref CX::CX_DataFrameCell::toVector<T>() "toVector<T>()". Using `to()` or `toVector()` might be neccessary if you are not storing the extracted value in a variable, but instead using it in an expression. If you are storing an extracted value in a variable, it's relatively easy for the compiler to figure out what type of data you want without you needing to specify it explicitly (but it is not always possible). It is harder for the compiler to figure it out when a value is used in an expression with other values of various types.

The questioning mind wonders what would happen if we were to read from a cell that has not yet been created, like this:
~~~{.cpp}
int i = df("ints", 2);
~~~
First off, a new cell would be created at ("ints", 2) with nothing in it (i.e. a zero-length vector). Then, an attempt will be made to convert that empty cell to an `int`, which is impossible to do in any meaningful way, so an error will be logged and a freshly constructed `int` with some value (probably 0) will be returned. In general, if a type `T` is requested, but there is nothing in the cell, an instance of `T` will be constructed using the default constructor and that value will be returned. If instead of a logged error and an invalid value, you would like to get an exception when attempting an out-of-bounds access, you can use CX::CX_DataFrame::at() instead of operator(). at() does not dynamically resize the data frame, instead throwing a std::out_of_range exception if you are accessing something that does not exist.

Once you have a CX_DataFrame that is full of data, you can easily output it to a file for later processing. You do this with \ref CX::CX_DataFrame::printToFile(), which has several versions. The most simple version only needs a file name and uses default values for the various outputting options. With the following call

~~~{.cpp}
df.printToFile("myData.txt");
~~~

the data frame contents will be printed to `PROJECT_DIR/bin/data/myData.txt`. The file will be a tab-delimited text file, which is a file type that can be read by pretty much any program that does data analysis (and if not, you can open it with Excel and change its format). Some of the settings that you can change are the delimiter between cells (default tab: "\t"), whether to output row numbers (default false), what to enclode vector cells with (default double quote: "\""), and what to delimit elements of a vector with (default semicolon: ";"). In addition, you can choose to only print out specific rows and columns of the data frame. To use these more advanced options, there are a number of versions of CX_DataFrame::printToFile() that can be used, the most thorough one taking a CX::CX_DataFrame::OutputOptions struct.

A common issue is that altough it is fine to use vector-containing cells within CX, other software does not support vectors of data within single cells. To deal with this, call

~~~{.cpp}
df.convertAllVectorColumnsToMultipleColumns(1, true); //Arguments are: start index, delete originals
~~~

which does what it claims to do. With this function call, we are saying to convert the vectors to multiple columns, naming the new vectors with indices starting from 1 and deleting the old vector column once the conversion is complete. In our example, there is a column named "vect" that has 3 elements per cell. After this function call, there will be 3 columns, named vect1, vect2, and vect3, with each column containing one of the elements of the original cells of vect. If the number of elements per cell is not the same in every row of a column, some rows will have empty cells that follow the last data cell. For the sake of explicitness, the empty cells will be filled with the string "NA".



What types can be stored in a data frame?
-----------------------------------------

All of the types that are built into C++ (e.g. int, float) can be stored. Most of the types that are in the standard library (e.g. std::string) can be stored. Many of the types built into openFrameworks (e.g. ofColor, ofPoint) can be stored. In general, any type `T` can be stored into and retrieved from a CX_DataFrame as long as `T` has overloaded versions of the folowing functions, which are the stream insertion and extraction operators:

\code{.cpp}
std::ostream& operator<<(std::ostream&, const T&)
std::istream& operator>>(std::istream&, const T&)
\endcode

It is possible to only define one of these functions and have partial functionality, but honestly, why would you ever do that? Inside of the `src` folder for the dataFrame example, see `myType.h` for an example of how to define these functions for a class. See also the following website: http://geeksquiz.com/overloading-stream-insertion-operators-c/ Alternatively, search the internet for "c++ overload stream insertion and extraction operators".



