 /// @file
 ///	@ingroup 	minexamples
 ///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
 ///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#define _CRT_SECURE_NO_WARNINGS
 
#include "c74_min_unittest.h"     // required unit test header
#include "bml.Input.cpp"    // need the source of our object so that we can access it

// Unit tests are written using the Catch framework as described at
// https://github.com/philsquared/Catch/blob/master/docs/tutorial.md

namespace max = c74::max;

SCENARIO("Testing how to test")
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
        // BMLInput bmlInput({10});
        // bmlInput.streamProperty = "test_property";
        // bmlInput.streamPropValue = "test_value";

        // REQUIRE(bmlInput.getStreamPropertyValue() == "test_property");
        // REQUIRE(bmlInput.getStreamProperty() == "test_value");
        // REQUIRE(bmlInput.getNumChannels() == 10);
    }

}

// SCENARIO("Testing how to test") 
// {
//     ext_main(nullptr);    // every unit test must call ext_main() once to configure the class

//     GIVEN("A default instance of the object.")
//     {
//         // mindev::test_wrapper<BMLInput> instance;
//         // BMLInput& bmlInput = instance;

//         // REQUIRE(true==true);

//         // REQUIRE(bmlInput.getStreamName() == "");
//         // REQUIRE(bmlInput.getStreamProperty() == "");
//         // REQUIRE(bmlInput.getNumChannels() == 6);
//     }
//     // AND_GIVEN("An instance with non-default attributes and arguments")
//     // {
//         // BMLInput* bmlInput = new BMLInput();

//         // REQUIRE(bmlInput->getStreamName() == "");
//         // REQUIRE(bmlInput->getStreamProperty() == "");
//         // REQUIRE(bmlInput->getNumChannels() == 8);
//     // }
//     // AND_WHEN("a bang is received")
//     // {
//     //     bmlInput.get_data();
//     //     THEN("We check the outlet to see")
//     //     {
//     //         auto& output = *max::object_getoutput(bmlInput, 0);
//     //         REQUIRE(output.size() == 0);
//     //     }

//     // }
// }
