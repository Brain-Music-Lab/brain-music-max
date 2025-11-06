#include "c74_min.h"  // Must include to access min devkit
#include "lsl_cpp.h"
#include "bml-dsp/circular-buffer.h"
#include <thread>
#include <memory>
#include <sstream>
#include <assert.h>

namespace mindev = c74::min;  

#include <mutex>
#include <functional>
#include <string>


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
        //m_logsIndex(1),
        m_numChannels(8)
    {
        if (args.size() > 0)
            m_numChannels = args[0];

        for (int i = 0; i < m_numChannels + 1; i++)
        {
            std::stringstream ss;
            if (i == m_numChannels)
            { 
                ss << "Info Out";
                m_dumpOutIndex = i;
            }
            /*else if (i == m_numChannels + 1)
            {
                ss << "Logs";               
                m_logsIndex = i;
            }*/
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

    MIN_DESCRIPTION{ "" };  // Description of the object
    MIN_TAGS{ "" };  // Any tags to include
    MIN_AUTHOR{ "" };  // Author of the objectcmake
    MIN_RELATED{ "" };  // Related Max Objects

    mindev::inlet<> toggle{ this, "Toggle on/off", "int" };
    mindev::inlet<> info{ this, "Info"};

    //mindev::attribute<mindev::symbol> m_name{ this, "type", "EEG" };

    mindev::argument<int> channelsArg{ this, "channels", "Number of channels in the LSL stream.",
        MIN_ARGUMENT_FUNCTION
        {
            m_numChannels = arg;
        }
    };

    mindev::message<> get_data{ this, "bang", "LSL Data Out",
        MIN_FUNCTION
        {
            if (inlet != DATA_INLET) return {};

                for (int i = 0; i < m_numChannels; i++)
                {
                    std::vector<double> values = m_buffers[i]->readNew();
                    if (values.size() != 0)
                    {
                        mindev::atoms outValues(values.begin(), values.end());
                        m_outlets[i]->send(outValues);
                    }
                }

            return {};
        }
    };

    mindev::message<> on_off{ this, "int", "Turn LSL receiving on or off",
        MIN_FUNCTION
        {
            if (inlet != DATA_INLET) return {};

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

            return {};
        }
    };

    mindev::message<> channelOut { this, "nchannels", "Get the number of channels",
        MIN_FUNCTION
        {
            if (inlet != INFO_INLET)
            return {};

            if (!m_lslInlet)
            {
                mindev::atom messageOut("LSL is not running.");
                m_outlets[m_dumpOutIndex]->send(messageOut);
                return {};
            }

            mindev::atom nChannels(m_lslInlet->info().channel_count());
            m_outlets[m_dumpOutIndex]->send(nChannels);

            return {};
        }
    };

    mindev::message<> samplerateOut { this, "samplerate", "Get the sample rate",
        MIN_FUNCTION
        {
            if (inlet != INFO_INLET)
            return {};

            if (!m_lslInlet)
            {
                mindev::atom messageOut("LSL is not running.");
                m_outlets[m_dumpOutIndex]->send(messageOut);
                return {};
            }

            mindev::atom fs(m_lslInlet->info().nominal_srate());
            m_outlets[m_dumpOutIndex]->send(fs);

            return {};
        }
    };

    void initializeAndReadLsl(
        std::unique_ptr<lsl::stream_inlet>& lsl_inlet, 
        std::atomic_bool& running, 
        std::vector<std::unique_ptr<BML::CircularBuffer>>& bufs,
        std::vector<std::unique_ptr<mindev::outlet<>>>& outlets)
    {
        
        // Don't initialize more than one
        if (running)
        {
            //outlets[m_logsIndex]->send("LSL stream already running.");
            return;
        }

        running = true;

        std::vector<lsl::stream_info> results = lsl::resolve_stream("type", "EEG");

        m_lslInlet = std::make_unique<lsl::stream_inlet>(results.at(0));

        std::vector<float> samples;
        double timestamp;

        while (running)
        {
            timestamp = m_lslInlet->pull_sample(samples);
            for (int i = 0; i < m_numChannels; i++)
            {
                if (samples.size() > i)
                    bufs[i]->write(samples[i]);
            }
        }


        m_lslInlet.reset();
        //outlets[m_logsIndex]->send("LSL connection closed.");
    }

private:
    std::atomic_bool m_running;
    std::vector<std::unique_ptr<BML::CircularBuffer>> m_buffers;
    std::mutex m_mut;
    std::string m_mess;
    std::unique_ptr<lsl::stream_inlet> m_lslInlet;

    std::vector<std::unique_ptr<mindev::outlet<>>> m_outlets;
    int m_dumpOutIndex;
    //int m_logsIndex;
    int m_numChannels;
};

MIN_EXTERNAL(BMLInput);  
