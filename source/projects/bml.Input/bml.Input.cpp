#define _CRT_SECURE_NO_WARNINGS

#include "c74_min.h"  // Must include to access min devkit
#include "lsl_cpp.h"
#include "bml-dsp/circular-buffer.h"
#include <thread>
#include <typeinfo>
#include <memory>
#include <sstream>
#include <assert.h>
#include <iostream>

namespace mindev = c74::min;

#include <mutex>
#include <functional>
#include <string>
#include <exception>

const int DATA_INLET = 0;
const int INFO_INLET = 1;

class BMLInput : public mindev::object<BMLInput>
{

public:

    BMLInput(const mindev::atoms& args = {}) :
        m_running(false),
        m_buffers(),
        m_mut(),
        m_mess(),
        m_outlets(),
        m_dumpOutIndex(0),
        m_numChannels(8),
        m_streamPropValue(),
        m_streamProperty(),
        m_sr(0),
        m_nLslChannels(0)
    {
        if (args.size() > 0)
            m_numChannels = args[0];

        for (int i = 0; i <= m_numChannels; i++)
        {
            std::stringstream ss;
            if (i == m_numChannels)
            { 
                ss << "Info Out";
                m_dumpOutIndex = i;
            }
            else
            {
                ss << "LSL Out " << i + 1;
            }

            auto outlet = std::make_unique<mindev::outlet<>>(this, ss.str(), "list");
            m_outlets.push_back(std::move(outlet));
        }

        for (int i = 0; i < m_numChannels; i++)
        {
            auto buffer = std::make_unique<BML::CircularBuffer>(2000);
            m_buffers.push_back(std::move(buffer));
        }
    }

    ~BMLInput()
    {
        if (m_streamProperty != nullptr)
            delete m_streamProperty;

        if (m_streamPropValue != nullptr)
            delete m_streamPropValue;
    }

    MIN_DESCRIPTION{ "" };  // Description of the object
    MIN_TAGS{ "" };  // Any tags to include
    MIN_AUTHOR{ "Daniel Ethridge" };  // Author of the objectcmake
    MIN_RELATED{ "" };  // Related Max Objects

    mindev::inlet<> toggle{ this, "Toggle on/off", "int" };
    mindev::inlet<> info{ this, "Info"};

    mindev::argument<int> channelsArg{ this, "channels", "Number of channels in the LSL stream.",
        MIN_ARGUMENT_FUNCTION
        {
            m_numChannels = arg;
        }
    };

    mindev::attribute<mindev::symbol> streamProperty{ this, "Property name", "",
        mindev::setter { MIN_FUNCTION {
            if (args.size() > 0)
                m_streamProperty = new mindev::symbol(args[0]);

            return args;
        }}
    };

    mindev::attribute<mindev::symbol> streamPropValue { this, "Stream Name", "",
        mindev::setter { MIN_FUNCTION {
            if (args.size() > 0)
                m_streamPropValue = new mindev::symbol(args[0]);
            
            return args;
        }}
    };

    mindev::message<> getData{ this, "bang", "LSL Data Out",
        MIN_FUNCTION
        {
            call_getData(inlet);
            return {};
        }
    };

    mindev::message<> onOff{ this, "int", "Turn LSL receiving on or off",
        MIN_FUNCTION
        {
            call_onOff(inlet, args);
            return {};
        }
    };

    mindev::message<> nChannels { this, "nchannels", "Get the number of LSL channels",
        MIN_FUNCTION 
        {
            call_nChannels(inlet);
            return {};
        }
    };

    mindev::message<> samplerateOut { this, "samplerate", "Get the sample rate",
        MIN_FUNCTION 
        { 
            call_samplerateOut(inlet); 
            return {};
        }
    };

    mindev::queue<mindev::placeholder::none> noStreamFound { this,
        MIN_FUNCTION
        {
            std::string message("Warning: No LSL Stream found!");
            cerr << message << mindev::endl;
            m_outlets[m_dumpOutIndex]->send(message);
            return {};
        }
    };



    void call_getData(int inlet)
    {
        if (inlet != DATA_INLET) return;

        for (int i = 0; i < m_numChannels; i++)
        {
            std::vector<double> values = m_buffers[i]->readNew();
            if (values.size() != 0)
            {
                mindev::atoms outValues(values.begin(), values.end());
                m_outlets[i]->send(outValues);
            }
        }
    }

    void call_onOff(int inlet, const mindev::atoms& args)
    {
        if (inlet != DATA_INLET) return;

        int input_received = args[0];  // Read inlet
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
                std::thread t(
                    &BMLInput::initializeAndReadLsl, 
                    this, 
                    std::ref(m_lslInlet), 
                    std::ref(m_running), 
                    std::ref(m_buffers),
                    std::ref(m_outlets)
                );
                t.detach();
            }
            break;

        default:
            break;
        }
    }

    void call_nChannels(int inlet)
    {
        if (inlet != INFO_INLET)
            return;
            
        m_outlets[m_dumpOutIndex]->send(m_nLslChannels);
    }

    void call_samplerateOut(int inlet)
    {
        if (inlet != INFO_INLET)
            return;

        m_outlets[m_dumpOutIndex]->send(m_sr);
    }

    void initializeAndReadLsl(
        std::unique_ptr<lsl::stream_inlet>& lsl_inlet, 
        std::atomic_bool& running, 
        std::vector<std::unique_ptr<BML::CircularBuffer>>& bufs,
        std::vector<std::unique_ptr<mindev::outlet<>>>& outlets)
    {
        // Don't initialize more than one
        if (running)
        {
            return;
        }

        std::vector<lsl::stream_info> results;
        if ((m_streamProperty == nullptr) || (m_streamPropValue == nullptr))
            results = lsl::resolve_stream("name", "grace", 1, 2.0);
        else
        {
            std::string streamProperty = *m_streamProperty;
            std::string streamPropertyValue = *m_streamPropValue;
            results = lsl::resolve_stream("name", "grace", 1, 2.0);
        }

        if (results.size() == 0)
        {
            noStreamFound.set();
            std::cout << "Warning!: No LSL Stream Found\n";
            return;
        }

        // Get lsl stream
        m_lslInlet = std::make_unique<lsl::stream_inlet>(results.at(0));
        m_sr = m_lslInlet->info().nominal_srate();
        m_nLslChannels = m_lslInlet->info().channel_count();

        std::vector<float> samples;
        double timestamp;

        running = true;
        while (running)
        {
            timestamp = m_lslInlet->pull_sample(samples);
            for (int i = 0; i < m_numChannels; i++)
            {
                if (samples.size() > i)
                    bufs[i]->write(samples[i]);
            }
        }

        m_sr = 0.0;
        m_nLslChannels = 0.0;
        m_lslInlet.reset();
    }

    std::string getStreamPropertyValue()
    {
        if (m_streamPropValue != nullptr)
        {
            return *m_streamPropValue;
        }

        return "";
    }

    std::string getStreamProperty()
    {
        if (m_streamProperty != nullptr)
        {
            return *m_streamProperty;
        }

        return "";
    }

    int getNumChannels() { return m_numChannels; }
    bool lslRunning() { return m_running.load(); }
    mindev::atom getSr() { return m_sr; }
    mindev::atom getNumLslChannels() { return m_nLslChannels; }
    

private:
    std::atomic_bool m_running;
    std::vector<std::unique_ptr<BML::CircularBuffer>> m_buffers;
    std::mutex m_mut;
    std::string m_mess;
    std::unique_ptr<lsl::stream_inlet> m_lslInlet;

    std::vector<std::unique_ptr<mindev::outlet<>>> m_outlets;
    int m_dumpOutIndex;
    int m_numChannels;
    mindev::symbol* m_streamProperty;
    mindev::symbol* m_streamPropValue;

    mindev::atom m_sr;
    mindev::atom m_nLslChannels;
};

MIN_EXTERNAL(BMLInput);  
