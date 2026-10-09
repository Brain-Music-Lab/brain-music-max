 /// @file
 ///	@ingroup 	minexamples
 ///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
 ///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#define _CRT_SECURE_NO_WARNINGS
 
#include "c74_min_unittest.h"     // required unit test header
#include "bml.Resample_tilde.cpp"    // need the source of our object so that we can access it
#include "bml-dsp/"
#include <thread>
#include <chrono>
#include <iostream>
#include <typeinfo>
#include <exception>

// Unit tests are written using the Catch framework as described at
// https://github.com/philsquared/Catch/blob/master/docs/tutorial.md

namespace max = c74::max;


SCENARIO("Arguments and attributes")
{
    ext_main(nullptr);

    mindev::test_wrapper<BMLResample> instance;
    BMLResample& bmlResample = instance;

    bmlResample.dspsetup(48000.0);

    // {
    //     // Protected from crash
    //     mindev::atoms data({"hello", "there", 98.2});
    //     std::vector<double> test;
    //     bmlResample.call_dataIn(0, data);
    // }

    {
        // mindev::atoms data({21.2, 81.8, 98.2});
        std::vector<double> test;
        // bmlResample.call_dataIn(0, data);

        // Do memory allocation
        long frameCount = 10;
        long channelCount = 1;
        double** samplesIn = new double*[channelCount];
        double** samplesOut = new double*[channelCount];

        for (long i = 0; i < channelCount; i++)
        {
            samplesIn[i] = new double[frameCount];
            samplesOut[i] = new double[frameCount];
        }

        for (long i = 0; i < channelCount; i++)
        {
            for (long j = 0; j < frameCount; j++)
            {
                samplesIn[i][j] = 0.0;
                samplesOut[i][j] = i * frameCount + j;
            }
        }

        // Create bundles
        mindev::audio_bundle input(samplesIn, channelCount, frameCount);
        mindev::audio_bundle output(samplesOut, channelCount, frameCount);

        bmlResample(input, output);

        // memory deallocation
        for (long i = 0; i < channelCount; i++)
        {
            delete[] samplesIn[i];
            delete[] samplesOut[i];
        }

        delete[] samplesIn;
        delete[] samplesOut;
    }
}