/*
This file serves as a template and basic guide to creating Max objects with the Min-Devkit. Anytime the 
documentation is referenced, see http://cycling74.github.io/min-devkit/guide/writingobjects. This template is
based off of the min.beat.random object found at 
https://github.com/Cycling74/min-devkit/blob/main/source/projects/min.beat.random/min.beat.random.cpp
*/
#include "c74_min.h"  // Must include to access min devkit
#include <sstream>  // String stream

namespace mindev = c74::min;  /*C74 just uses 'using namespace c74::min'. I like this alternattive because
                              it forces some necessary (in my opinion) verbosity. */


// using namespace std;  <-- DO NOT EVER PUT THIS IN YOUR CODE! Why? Look up namespace collisions.


/* To create a Min object you define a class that inherits from a specialization of the min::object class.
You then wrap this class with the MIN_EXTERNAL macro that exposes the class to Max. */
class tutorial : public mindev::object<tutorial> {
public:

    /* Constructors and Destructors are optional. Use them if there is any custom initialization and/or destruction to perform. 
    See the documentation for more information. */
    tutorial() :
        m_name()
    {
        cout << "Constructor called" << m_name << mindev::endl;
    }

    MIN_DESCRIPTION	{ "" };  // Description of the object
    MIN_TAGS		{ "" };  // Any tags to include
    MIN_AUTHOR		{ "" };  // Author of the objectcmake 
    MIN_RELATED		{ "" };  // Related Max Objects


    /* Inlets and outlets take three arguments inside the curly braces
    - An owner object (typically this)
    - A description
    - A type (as a string).
    See documentation for more info. */

    mindev::inlet<>  input /*inlet 1*/ {this, "(bang) say hi", "bang"};
    mindev::outlet<> greeting_out		    { this, "Message for a person", "bang" };
    mindev::outlet<> dayout{ this, "Get the day out", "today"};

    /* Messages are the most basic functional unit. They take four arguments
    - An owner object (typically this)
    - A type (as a string)
    - A description
    - A function (typically MIN_FUNCTION)
    See documentation for information on MIN_FUNCTION 
    
    Write your functionality within the MIN_FUNCTION curly braces. An important thing to remember about 
    messages is that only one message of each type should be included. You could include the following 
    messages:
    
    message<> msg1 {this, "int", "message1", some_function};
    message<> msg2 {this, "float", "message2", another_function};
    message<> msg3 {this, "bang", "message3", yet_another_function};
    
    The important notes here are that the first message triggers when an inlet receives an int, the second a
    float, and the third a bang. Use a switch statement to decide what to do based on what inlet the
    information came from as demonstrated below. Also note how to send data to the outlets. Note that the
    switch statements here are a bit overkill since the inlets take specific types. They would be more 
    necessary if, for example, there were two inlets that accepted integers. */

    mindev::message<> say_something{ this, "bang", "Say Hi!",
        MIN_FUNCTION {
            std::stringstream ss;
            ss << "Hello " << m_name << "! Your number is " << m_greeting;
            greeting_out.send(ss.str());
            cout << "Greeting delivered!" << mindev::endl;
            return {};
        }
    };

    /* Create an Argument.
    They need a templated type, an owner, name, description, and function(MIN_ARGUMENT_FUNCTION)
    */
    mindev::argument<mindev::string> minimum_arg { this, "person", "Person to Greet",
        MIN_ARGUMENT_FUNCTION {
            m_name = arg;  // arg is the argument value
        }
    };

    mindev::message<> today{ this, "today", "Get the day of the week!",
        MIN_FUNCTION{
            dayout.send("Today is Thursday!");
            return {};
        }
    };


    /* Create an attribute. They need a templated type, an owner, name, description, and function(MIN_FUNCTION)
    See the attributes section of the documentation for infomration on optional arguments such as mindev::title,
    mindev::description, and mindev::setter. */
    mindev::attribute<int> m_greeting { this, "Numb", 1};

private:
    mindev::string m_name;
};

MIN_EXTERNAL(tutorial);  // Wrap the class name in the MIN_EXTERNAL macro
