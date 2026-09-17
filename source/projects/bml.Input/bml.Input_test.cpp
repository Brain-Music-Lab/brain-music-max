 /// @file
 ///	@ingroup 	minexamples
 ///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
 ///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#define _CRT_SECURE_NO_WARNINGS
 
#include "c74_min_unittest.h"     // required unit test header
#include "bml.Input.cpp"    // need the source of our object so that we can access it
#include <thread>
#include <chrono>

// Unit tests are written using the Catch framework as described at
// https://github.com/philsquared/Catch/blob/master/docs/tutorial.md

namespace max = c74::max;


SCENARIO("Arguments and attributes")
{
    ext_main(nullptr);

    GIVEN("Default construction of the object")
    {
        mindev::test_wrapper<BMLInput> instance;
        BMLInput& bmlInput = instance;

        REQUIRE(bmlInput.getStreamPropertyValue() == "");
        REQUIRE(bmlInput.getStreamProperty() == "");
        REQUIRE(bmlInput.getNumChannels() == 8);
    }
    GIVEN("Construction with different parameters")
    {
        BMLInput bmlInput({10});
        bmlInput.streamPropValue = "test_value";
        bmlInput.streamProperty = "test_property";

        REQUIRE(bmlInput.getStreamPropertyValue() == "test_value");
        REQUIRE(bmlInput.getStreamProperty() == "test_property");
        REQUIRE(bmlInput.getNumChannels() == 10);
    }
}

SCENARIO("Info inlet interactions")
{
    mindev::test_wrapper<BMLInput> instance;
    BMLInput& bmlInput = instance;

    WHEN("The 'samplerate' message is passed")
    {
        bmlInput.call_samplerateOut(INFO_INLET);
        auto& output = *c74::max::object_getoutput(bmlInput, bmlInput.getNumChannels());

        REQUIRE(output.size() == 1);
        REQUIRE(output[0].size() == 1);
        REQUIRE(output[0][0] == bmlInput.getSr());

        if (!bmlInput.lslRunning())
        {
            REQUIRE(output[0][0] == 0.0);
            REQUIRE(bmlInput.getSr() == 0.0);
        }

        for (int i = 0; i < bmlInput.getNumChannels(); i++)
        {
            output = *c74::max::object_getoutput(bmlInput, i);
            REQUIRE(output.size() == 0);
        }
    }

    AND_WHEN("The 'channelOut' message is passed")
    {
        bmlInput.call_nChannels(INFO_INLET);
        auto& output = *c74::max::object_getoutput(bmlInput, bmlInput.getNumChannels());

        REQUIRE(output.size() == 1);
        REQUIRE(output[0].size() == 1);
        REQUIRE(output[0][0] == bmlInput.getNumLslChannels());

        if (!bmlInput.lslRunning())
        {
            REQUIRE(output[0][0] == 0.0);
            REQUIRE(bmlInput.getNumLslChannels() == 0.0);
        }

        for (int i = 0; i < bmlInput.getNumChannels(); i++)
        {
            output = *c74::max::object_getoutput(bmlInput, i);
            REQUIRE(output.size() == 0);
        }
    }

    AND_WHEN("The 'getData' message is passed")
    {
        bmlInput.call_getData(INFO_INLET);
        auto& output = *c74::max::object_getoutput(bmlInput, bmlInput.getNumChannels());
        REQUIRE(output.size() == 0);

        for (int i = 0; i < bmlInput.getNumChannels(); i++)
        {
            output = *c74::max::object_getoutput(bmlInput, i);
            REQUIRE(output.size() == 0);
        }
    }

    AND_WHEN("The 'onOff' message is passed")
    {
        bmlInput.call_onOff(INFO_INLET, mindev::atoms({0}));
        auto& output = *c74::max::object_getoutput(bmlInput, bmlInput.getNumChannels());
        REQUIRE(output.size() == 0);

        for (int i = 0; i < bmlInput.getNumChannels(); i++)
        {
            output = *c74::max::object_getoutput(bmlInput, i);
            REQUIRE(output.size() == 0);
        }    
    }
}

SCENARIO("Data inlet interactions")
{
    mindev::test_wrapper<BMLInput> instance;
    BMLInput& bmlInput = instance;

    bmlInput.streamProperty = "type";
    bmlInput.streamPropValue = "EEG";

    WHEN("The 'samplerate' message is passed")
    {
        bmlInput.call_samplerateOut(DATA_INLET);

        for (int i = 0; i <= bmlInput.getNumChannels(); i++)
        {
            auto& output = *c74::max::object_getoutput(bmlInput, i);
            REQUIRE(output.size() == 0);
        }
    }

    AND_WHEN("The 'channelOut' message is passed")
    {
        bmlInput.call_nChannels(DATA_INLET);

        for (int i = 0; i <= bmlInput.getNumChannels(); i++)
        {
            auto& output = *c74::max::object_getoutput(bmlInput, i);
            REQUIRE(output.size() == 0);
        }
    }

    // TODO: THIS TEST DOESN'T WORK
    AND_WHEN("The 'onOff' message is passed")
    {
        using namespace std::chrono_literals;

        bmlInput.call_onOff(DATA_INLET, mindev::atoms({1}));
        std::this_thread::sleep_for(1000ms);
        REQUIRE(bmlInput.lslRunning() == true);

        bmlInput.call_onOff(DATA_INLET, mindev::atoms({0}));
        std::this_thread::sleep_for(1000ms);
        REQUIRE(bmlInput.lslRunning() == false);
    }

    AND_WHEN("The 'getData' message is passed")
    {
        using namespace std::chrono_literals;
        bmlInput.call_onOff(DATA_INLET, mindev::atoms({0}));
        std::this_thread::sleep_for(1000ms);
        bmlInput.call_getData(DATA_INLET);

        for (int i = 0; i < bmlInput.getNumChannels(); i++)
        {
            auto& output = *c74::max::object_getoutput(bmlInput, i);
            REQUIRE(output.size() == 0);
        }

        bmlInput.call_onOff(DATA_INLET, mindev::atoms({0}));
    }
}

