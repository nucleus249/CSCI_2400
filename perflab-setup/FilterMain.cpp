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

  //variables to limit function calls
  int const divisor = filter -> getDivisor();

  int const rlim = input->height - 1;
  int const clim = input->width - 1;

  int const filter00 = filter -> get(0,0);
  int const filter10 = filter -> get(1,0);
  int const filter20 = filter -> get(2,0);
  int const filter01 = filter -> get(0,1);
  int const filter11 = filter -> get(1,1);
  int const filter21 = filter -> get(2,1);
  int const filter02 = filter -> get(0,2);
  int const filter12 = filter -> get(1,2);
  int const filter22 = filter -> get(2,2);

//for loops reordered for memory efficiency
for(int plane = 0; plane < 3; ++plane) {
	auto const p = input->color[plane];
       	for(int row = 1; row < rlim ; ++row) {
		auto const rp1 = p[row + 1];
		auto const rp0 = p[row];
		auto const rm1 = p[row - 1];
	       	for(int col = 1; col < clim; ++col ) {
			int const cp1 = col + 1;
			int const cm1 = col - 1;

       int &out = output->color[plane][row][col];

       out =
       ( rm1[cm1]*filter00
       + rm1[col]*filter01
       + rm1[cp1]*filter02
       + rp0[cm1]*filter10
       + rp0[col]*filter11
       + rp0[cp1]*filter12
       + rp1[cm1]*filter20
       + rp1[col]*filter21
       + rp1[cp1]*filter22)
       /divisor;
	
       if (out > 255) {
	  out = 255;
       } else if (out < 0) { 
	  out = 0;
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
