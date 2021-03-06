/**
 * @file main.cpp
 * @brief Contais the main code for automated 2d mosaic construction
 * @version 0.2
 * @date 10/02/2018
 * @author Victor Garcia
 * @title Main code
 */

#include "include/options.h"
#include "include/mosaic.hpp"

/// User namespaces
using namespace std;

/*
 * @function main
 * @brief Main function
 */
int main( int argc, char** argv ) {

    double t;
    cv::Mat img;
    string input_directory, output_directory;
    vector<string> file_names;

    try{
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help) {
        std::cout << parser;
        return 1;
    }
    catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Use -h, --help command to see usage" << std::endl;
        return 1;
    }
    catch (args::ValidationError e) {
        std::cerr << "Bad input commands" << std::endl;
        std::cerr << "Use -h, --help command to see usage" << std::endl;
        return 1;
    }

    input_directory = args::get(input_dir);
    output_directory = args::get(output_dir);

    //-- VERBOSE SECTION --//
    cout << endl << "2D mosaic generation"<<endl;
    cout << "Author: Victor Garcia"<<endl<<endl;
    cout << "  Built with OpenCV\t" <<cyan<< CV_VERSION << reset << endl;
    cout << "  Input directory:\t"<<cyan<< input_directory<<reset << endl;
    cout << "  Output directory:\t"<<cyan<< output_directory<<reset << endl;
    //-- Feature Extractor
    detector_surf ?
    cout<<"  Feature extractor:\t"<<cyan<< "SURF"<<reset<<endl:
    detector_sift ?
    cout<<"  Feature extractor:\t"<<cyan<< "SIFT"<<reset<<endl:
    detector_akaze ?
    cout<<"  Feature extractor:\t"<<cyan<< "A-KAZE"<<reset<<endl:
    cout<<"  Feature extractor:\t"<<cyan<< "KAZE\t(Default)"<<reset<<endl;
    //-- Feature Matcher
    matcher_brutef ?
    cout<<"  Feature Matcher:\t"<<cyan<<"BRUTE FORCE"<< reset << endl:
    cout<<"  Feature Matcher:\t"<<cyan<<"FLANN\t(Default)"<< reset << endl;
    //-- # bands for multiband blender
    blender_bands ?
    cout<<"  Nº bands (blender):\t"<<cyan<< args::get(blender_bands)<<reset<<endl :
    cout<<"  Nº bands (blender):\t"<<cyan<<0<< reset << endl;
    //-- Mosaic mode
    cout << "  Mosaic Mode:\t\t" << cyan;
    euclidean_mode ? cout<<cyan<<"Euclidean" : cout <<cyan<<"Perspective";
    cout << reset << endl;
    //-- Seam finder
    cout << "  Seam finder:\t\t" << cyan;
    graph_cut ? cout <<cyan<< "Graph cut" : cout <<cyan<< "Simple";
    cout << reset << endl;
    //-- Optional commands
    cout << boolalpha;
    // cout << "  Use grid detection:\t"<<cyan<< use_grid <<reset << endl;
    // cout << "  Eclidean correction:\t"<<cyan<< euclidean_correction <<reset << endl;
    cout << "  Apply SCB:\t\t"<<cyan<< final_scb <<reset << endl<< endl;

    m2d::Mosaic mosaic(true);
    mosaic.stitcher = new m2d::Stitcher(
        true, // grid detection                                                   
        detector_surf  ? m2d::USE_SURF  :
        detector_sift  ? m2d::USE_SIFT  :
        detector_akaze ? m2d::USE_AKAZE : m2d::USE_KAZE,
        matcher_flann  ? m2d::USE_FLANN : m2d::USE_BRUTE_FORCE
    );
    mosaic.blender = new m2d::Blender(blender_bands ? args::get(blender_bands) : 0,
                                      graph_cut,
                                      final_scb);

    t = (double) getTickCount();

    file_names = read_filenames(input_directory);
    for (int i=0; i<file_names.size(); i++) {
        img = imread(file_names[i], IMREAD_COLOR);
        if (!img.data) {
            cout<< red <<" --(!) Error reading image "<< reset << endl;
            continue;
        }
        mosaic.feed(img);
    }
    mosaic.compute(euclidean_mode);
    mosaic.merge(true);
    mosaic.save(output_directory);
    
    t = ((double)getTickCount() - t) / getTickFrequency();
    cout <<endl<<endl<<"  Execution time:\t" << green << t << reset <<" s" <<endl;

    if (output) {
        mosaic.show();
    }

    return 0;
}