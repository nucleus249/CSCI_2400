#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rdtsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);
    for (int i=0; i < size; i++) {
      for (int j=0; j < size; j++) {
	int value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
    return filter;
  } else {
    cerr << "Bad input in readFilter:" << filename << endl;
    exit(-1);
  }
}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output -> width = input -> width;
  output -> height = input -> height;
  int plane;
  int row;
  int col;

  //accumulators for use of registers and loop unrolling
  int coloracc;
  int coloracc2;
  int coloracc3;

  //variables to limit function calls
  int divisor = filter -> getDivisor();
  int filter00 = filter -> get(0,0);
  int filter10 = filter -> get(1,0);
  int filter20 = filter -> get(2,0);
  int filter01 = filter -> get(0,1);
  int filter11 = filter -> get(1,1);
  int filter21 = filter -> get(2,1);
  int filter02 = filter -> get(0,2);
  int filter12 = filter -> get(1,2);
  int filter22 = filter -> get(2,2);

//for loops reordered for memory efficiency
for(plane = 0; plane < 3; ++plane) {
       	for(row = 1; row < (input -> height) - 1 ; ++row) {
	       	for(col = 1; col < (input -> width) - 1; ++col ) {

	/*output -> color[plane][row][col] = 0;*/
	coloracc = 0;
	coloracc2 = 0;
	coloracc3 = 0;

	   /* output -> color[plane][row][col]
	      = output -> color[plane][row][col]
	      + (input -> color[plane][row + i - 1][col + j - 1] 
		 * filter -> get(i, j) );*/
       coloracc = coloracc + (input -> color[plane][row-1][col-1] * filter00);
       coloracc2 = coloracc2 + (input -> color[plane][row][col-1] * filter10);
       coloracc3 = coloracc3 + (input -> color[plane][row+1][col-1] * filter20);
       coloracc = coloracc + (input -> color[plane][row-1][col] *  filter01);
       coloracc2 = coloracc2 + (input -> color[plane][row][col] * filter11);
       coloracc3 = coloracc3 + (input -> color[plane][row+1][col] * filter21);
       coloracc = coloracc + (input -> color[plane][row-1][col+1] * filter02);
       coloracc2 = coloracc2 + (input -> color[plane][row][col+1] * filter12);
       coloracc3 = coloracc3 + (input -> color[plane][row+1][col+1] * filter22);
	
       coloracc = coloracc + coloracc2 + coloracc3;
       coloracc = coloracc / divisor;
       output -> color[plane][row][col] = coloracc;
       if ( /*output -> color[plane][row][col]*/ coloracc < 0 ) {
	  output -> color[plane][row][col] = 0;
       }
       
       if ( /*output -> color[plane][row][col]*/ coloracc > 255 ) { 
	  output -> color[plane][row][col] = 255;
       }
     }
   }
 }

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}
