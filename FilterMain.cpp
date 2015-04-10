#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rtdsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{
    
    if ( argc < 2)
    {
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
        for (int i=0; i < size; ++i) {
            for (int j=0; j < size; ++j) {
                int value;
                input >> value;
                filter -> set(i,j,value);
            }
        }
        return filter;
    }
    

}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{
    
    long long cycStart, cycStop;
    
    cycStart = rdtscll();
    
    output -> width = input -> width;
    output -> height = input -> height;
    
    int w1 = (input -> width) -1;
    int h1 = (input -> height) -1; //instead of doing calculations in the loop, go ahead and calculate the width and height before the loop and just use the variables
    int plane, row, col, sum;
    int div = filter -> getDivisor(); //same thing
    
    int fArray[8]; //Localizing filter -> get(x,x)
    fArray[0] = filter -> get(0,0);
    fArray[1] = filter -> get(0,1);
    fArray[2] = filter -> get(0,2);
    fArray[3] = filter -> get(1,0);
    fArray[4] = filter -> get(1,1);
    fArray[5] = filter -> get(1,2);
    fArray[6] = filter -> get(2,0);
    fArray[7] = filter -> get(2,1);
    fArray[8] = filter -> get(2,2);
    
    for(plane = 0; plane < 3; ++plane) { //switching the order of the loops helps us with locality
        for(row = 1; row < h1 ; ++row) {
            for(col = 1; col < w1; ++col) {
                
                int sum = 0; //took out original code; redeclared it as sum outside the loop to stop in loop calculations
                

                
                sum += (input -> color[plane][row     - 1][col     - 1] * fArray[0]); //filter -> get(x,x)
                sum += (input -> color[plane][row     - 1][col        ] * fArray[1]);
                sum += (input -> color[plane][row     - 1][col     - 1] * fArray[2]);
                sum += (input -> color[plane][row        ][col     - 1] * fArray[3]);
                sum += (input -> color[plane][row        ][col        ] * fArray[4]);
                sum += (input -> color[plane][row        ][col     + 1] * fArray[5]);
                sum += (input -> color[plane][row     + 1][col     - 1] * fArray[6]);
                sum += (input -> color[plane][row     + 1][col        ] * fArray[7]);
                sum += (input -> color[plane][row     + 1][col     + 1] * fArray[8]);
                
                
                output -> color[plane][row][col] = sum;
                
                sum = sum / div;
                
                if ( sum  < 0 ) {
                    sum = 0;
                }
                
                if ( sum  > 255 ) { 
                    sum = 255;
                }
                output -> color[plane][row][col] = sum;
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
