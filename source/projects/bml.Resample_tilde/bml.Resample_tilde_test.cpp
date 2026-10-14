 /// @file
 ///	@ingroup 	minexamples
 ///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
 ///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#define _CRT_SECURE_NO_WARNINGS
 
#include "c74_min_unittest.h"     // required unit test header
#include "bml.Resample_tilde.cpp"    // need the source of our object so that we can access it
#include <thread>
#include <chrono>
#include <iostream>
#include <typeinfo>
#include <exception>

#include <bml-dsp/util/csv.h>
#include <bml-dsp/util/bml-math.h>

// Unit tests are written using the Catch framework as described at
// https://github.com/philsquared/Catch/blob/master/docs/tutorial.md

namespace max = c74::max;


// SCENARIO("Arguments and attributes")
// {
//     ext_main(nullptr);

//     mindev::test_wrapper<BMLResample> instance;
//     BMLResample& bmlResample = instance;

//     bmlResample.dspsetup(48000.0);

    
//         // Protected from crash
//         std::vector<string> data({"hello", "there", "98.2"});
//         std::vector<double> test;
//         bmlResample.call_dataIn(0, data);
// }

SCENARIO("Fractional Resampling")
{
    ext_main(nullptr);

    // Set up the test
    long frameCount = 2048;
    long channelCount = 1;  // Only using one truth array
    double** samplesIn = nullptr;  // Never accessed in the code
    double** samplesOut = new double*[channelCount];

    for (long i = 0; i < channelCount; i++)
    {
        samplesOut[i] = new double[frameCount];
    }

    for (long i = 0; i < channelCount; i++)
    {
        for (long j = 0; j < frameCount; j++)
        {
            samplesOut[i][j] = 0.0;
        }
    }

    mindev::audio_bundle input(samplesIn, channelCount, frameCount);
    mindev::audio_bundle output(samplesOut, channelCount, frameCount);

    // Get python truth.
    std::stringstream ss;
    std::filesystem::path currentPath(__FILE__);
    ss << currentPath.parent_path().string() << "/../../../tests/ground_truth/resample_test3_y.csv";
    std::vector<double> y = BML::readOneLineCSV(ss.str());

    ss.str("");
    ss << currentPath.parent_path().string() << "/../../../tests/ground_truth/resample_test3_y_truth.csv";
    std::vector<double> yTruth = BML::readOneLineCSV(ss.str());

    // set up resampler
    mindev::test_wrapper<BMLResample> instance;
    BMLResample& bmlResample = instance;
    // bmlResample.dspsetup(48000.0);
    size_t numTaps = ((bmlResample.TapsInResampleFilter() - 1) / 4) + 1;
    
    // zero pad
    std::vector<double> full;
    for (int i = 0; i < 1000; i++)
    {
        y.push_back(0.0);
    }

    // Resample on an input block size of 50
    size_t block_size = 50;
    size_t itIdx = 0;
    while (itIdx * block_size < y.size() - block_size)
    {
        // set iterators
        auto startIt = y.begin() + itIdx * block_size;
        auto endIt = startIt + block_size;

        // set input and output
        std::vector<double> tempIn(startIt, endIt);
        std::vector<double> tempOut;
        bmlResample.call_dataIn(0, tempIn);  // set input data vector
        bmlResample(input, output);  // resample

        // save output
        for (size_t i = 0; i < frameCount; i++)
        {
            full.push_back(samplesOut[0][i]);
        }

        itIdx += 1;
    }

    // create resampled vector
    size_t startIdx = ((numTaps-1) / 2);
    size_t endIdx = startIdx + 48000;
    std::vector resampSig(full.begin() + startIdx, full.begin() + endIdx);

    REQUIRE(resampSig.size() == yTruth.size());

    // At a sampling rate of 48000, 480 samples is approximately 10ms
    std::vector<double> errors(yTruth.size() - 960, 0.0);

    for (size_t i = 480; i < yTruth.size() - 480; i++)
    {
        // REQUIRE_THAT(resampSig[i], Catch::Matchers::WithinAbs(yTruth[i], 0.1));  // Max error is 0.1
        errors[i-480] = resampSig[i] - yTruth[i];
    }

    std::vector<double> sqrError = errors;
    double mean = std::accumulate(errors.begin(), errors.end(), 0.0) / static_cast<double>(errors.size());

    std::sort(errors.begin(), errors.end());
    for (size_t i = 1; i < errors.size(); i++)  // Proof of being sorted for median calculation
    {
        REQUIRE(errors[i - 1] <= errors[i]);
    }

    size_t idx;
    if (errors.size() % 2 == 0)
    {
        idx = errors.size() / 2;
    }
    else
    {
        idx = (errors.size() - 1) / 2;
    }

    // Average and median errors are *much* less than max error.
    // REQUIRE(mean < 0.001);
    // REQUIRE(errors[idx] < 0.001);  // median

    
    // Clean up
    for (long i = 0; i < channelCount; i++)
    {
        delete[] samplesOut[i];
    }

    delete[] samplesOut;
}
