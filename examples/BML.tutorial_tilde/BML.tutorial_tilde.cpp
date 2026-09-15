/*
This file serves as a template and basic guide to creating Max objects with the Min-Devkit. Anytime the 
documentation is referenced, see http://cycling74.github.io/min-devkit/guide/writingobjects. This template is
based off of the min.beat.random object found at 
https://github.com/Cycling74/min-devkit/blob/main/source/projects/min.beat.random/min.beat.random.cpp
*/
#include "c74_min.h"
#include "math.h"
#include <algorithm>

namespace mindev = c74::min;  /*C74 just uses 'using namespace c74::min'.I like this alternattive because
                              it forces some necessary (in my opinion) verbosity. */


// using namespace std;  <-- DO NOT EVER PUT THIS IN YOUR CODE! Why? Look up namespace collisions.


/* To create a Min object you define a class that inherits from a specialization of the min::object class.
You then wrap this class with the MIN_EXTERNAL macro that exposes the class to Max. */
class example : public mindev::object<example>, public mindev::vector_operator<> 
{
public:
    /* Constructors and Destructors are optional. Use them if there is any custom initialization and/or destruction to perform. 
    See the documentation for more information. */

    MIN_DESCRIPTION	{ "" };  // Description of the object
    MIN_TAGS		{ "" };  // Any tags to include
    MIN_AUTHOR		{ "" };  // Author of the objectcmake 
    MIN_RELATED		{ "" };  // Related Max Objects

    example() :
        m_frequency(220.0),
        m_samplerate(0.0),
        m_time(0.0),
        m_on(false)
    {}


    /* Inlets and outlets take three arguments inside the curly braces
    - An owner object (typically this)
    - A description
    - A type (as a string).
    See documentation for more info. */

    mindev::inlet<>  set_freq {this, "(Toggle/float) Toggle on/off and set frequency of oscillator", "number"};
    mindev::outlet<> osc_out  { this, "Oscillator out", "signal" };

    mindev::message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION{
            m_samplerate = static_cast<double>(args[0]);
            return {};
        } 
    };

    mindev::message<> toggle{ this, "int", "Toggle on and off",
        MIN_FUNCTION {
            if (args[0] == 0)
            {
                m_on = false;
                cout << "Turned off" << mindev::endl;
            }
            else if (args[0] == 1)
            {
                m_on = true;
                cout << "Turned on" << mindev::endl;
            }
            return {};
        }
    };

    mindev::message<> freq{ this, "float", "Set frequency",
        MIN_FUNCTION{
            m_frequency = args[0];
        return {};
        }
    };

    void operator()(mindev::audio_bundle input, mindev::audio_bundle output)
    {
        size_t num_channels = static_cast<size_t>(output.channel_count());
        size_t frame_count = static_cast<size_t>(output.frame_count());

        for (size_t ch = 0; ch < static_cast<size_t>(num_channels); ch++)
        {
            std::for_each(
                output.samples(ch),
                output.samples(ch) + frame_count,
                [this](double& val)
                {
                    m_time += 1.0 / m_samplerate;
                    val = std::sin(2.0 * 3.141592 * m_frequency * m_time);
                });
        }
    };

private:
    double m_samplerate;
    double m_time;
    double m_frequency;
    bool m_on;
};


MIN_EXTERNAL(example);  // Wrap the class name in the MIN_EXTERNAL macro
