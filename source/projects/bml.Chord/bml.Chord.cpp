#define _CRT_SECURE_NO_WARNINGS

#include "c74_min.h"  // Must include to access min devkit
#include "lsl_cpp.h"
#include "bml-dsp/circular-buffer.h"
#include "bml-dsp/util/bml-math.h"
#include <thread>
#include <typeinfo>
#include <memory>
#include <sstream>
#include <assert.h>
#include <iostream>
#include <string>

namespace mindev = c74::min;

#include <mutex>
#include <functional>
#include <string>
#include <exception>

class BMLChord : public mindev::object<BMLChord>
{
public:
    BMLChord(const mindev::atoms& args = {}) :
        m_circBuf(512),
        m_running(false)
    {
        std::vector<double> t = BML::Math::arange(0.0, 1.0, 1.0/512.0);
        std::vector<double> y;

        for (size_t i = 0; i < t.size(); i++)
        {
            y.push_back(std::sin(200.0 * 2.0 * BML::Math::PI * t[i]));
        }

        m_circBuf.write(y);
    }

    mindev::inlet<> toggleOn { this, "Toggle On/Off", "int"};
    mindev::outlet<> dataOut { this, "Data Out", "list"};

    mindev::message<> onOff { this, "int", "Turn generator on and off",
        MIN_FUNCTION
        {
            cout << "onOff called" << mindev::endl;
            call_onOff(std::vector<int>(args.begin(), args.end()));
            return {};
        }
    };

    void call_onOff(const std::vector<int>& args)
    {
        int input_received = args[0];
        switch (input_received)
        {
            case 0:
                if (m_running)
                {
                    m_running = false;
                }
                break;
            
            case 1:
                if (!m_running)
                {
                    m_running = true;
                    metro.delay(0.0);
                }
        }
    }

    mindev::timer<> metro { this,
        MIN_FUNCTION {
            sendDataOut.set();
            cout << "metro called" << mindev::endl;

            if (m_running)
                metro.delay(100.0);

            return {};
        }
    };

    mindev::queue<mindev::placeholder::none> sendDataOut { this,
            MIN_FUNCTION
            {
                call_dataOut();
                return {};
            }
        };

    void call_dataOut()
    {
        cout << "data out called" << mindev::endl;
        std::vector<double> out = m_circBuf.read(50);
        dataOut.send(mindev::atoms(out.begin(), out.end()));
    }

private:
    BML::CircularBuffer m_circBuf;
    std::atomic_bool m_running;
};

MIN_EXTERNAL(BMLChord);  
