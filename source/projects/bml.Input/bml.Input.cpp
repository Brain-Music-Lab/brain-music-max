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

class BMLInput : public mindev::object<BMLInput>
{

public:

    BMLInput(const mindev::atoms& args = {}) :
        m_running(false),
        m_buffers(),
        m_mut(),
        m_mess(),
        m_outlets(),
        m_infoOutIndex(0),
        m_numChannels(8),
        m_streamProperty("type"),
        m_streamPropValue("EEG"),
        m_sr(0),
        m_nLslChannels(0),
        m_metroSpeed(100.0)
    {
        if (args.size() > 0)
            m_numChannels = args[0];
        if (args.size() > 1)
            m_metroSpeed = args[1];

        for (int i = 0; i < m_numChannels; i++)
        {
            std::stringstream ss;
            ss << "LSL Out " << i + 1;
            auto outlet = std::make_unique<mindev::outlet<>>(this, ss.str(), "list");
            m_outlets.push_back(std::move(outlet));

            auto buffer = std::make_unique<BML::CircularBuffer>(2000);
            m_buffers.push_back(std::move(buffer));
        }

        auto outlet = std::make_unique<mindev::outlet<>>(this, "Timestamps out", "list");
        m_outlets.push_back(std::move(outlet));
        m_timestampIndex = m_outlets.size() - 1;

        outlet = std::make_unique<mindev::outlet<>>(this, "Info out", "list");
        m_outlets.push_back(std::move(outlet));
        m_infoOutIndex = m_outlets.size() - 1;

        m_timestamps = std::make_unique<BML::CircularBuffer>(2000);
        m_equalTimestamps = std::make_unique<BML::CircularBuffer>(2000);

        
    }

    ~BMLInput()
    {

    }

    MIN_DESCRIPTION{ "" };  // Description of the object
    MIN_TAGS{ "" };  // Any tags to include
    MIN_AUTHOR{ "Daniel Ethridge" };  // Author of the objectcmake
    MIN_RELATED{ "" };  // Related Max Objects

    mindev::inlet<> toggle{ this, "Input", "int" };

    // Arguments
    mindev::argument<int> channelsArg{ this, "channels", "Number of channels in the LSL stream.",
        MIN_ARGUMENT_FUNCTION
        {
            m_numChannels = arg;
        }
    };

    mindev::argument<double> metroSpeedArg {this, "metroSpeed", "How often to output most recent LSL data",
        MIN_ARGUMENT_FUNCTION
        {
            m_metroSpeed = arg;
        }
    };

    // Attributes
    mindev::attribute<mindev::symbol> streamProperty{ this, "property", "type",
        mindev::description { "The Property to look for (e.g. type, name). Default is 'type'" },
        mindev::setter { MIN_FUNCTION {
            if (args.size() > 0) 
            {
                m_streamProperty = mindev::symbol(args[0]);
            }
            return args;
        }}
    };

    mindev::attribute<mindev::symbol> streamPropValue { this, "value", "EEG",
        mindev::description { "The value of the property (e.g. EEG, ECG, or the name of a named stream). Default is 'EEG'" },
        mindev::setter { MIN_FUNCTION {
            if (args.size() > 0)
            {
                m_streamPropValue = mindev::symbol(args[0]);
            }
            
            return args;
        }}
    };

    // mindev::message<> getData{ this, "bang", "LSL Data Out",
    //     MIN_FUNCTION
    //     {
    //         call_getData(inlet);
    //         return {};
    //     }
    // };

    // Queues
    mindev::queue<mindev::placeholder::none> logWarningMessage { this,
        MIN_FUNCTION
        {
            // std::string message("Warning: No LSL Stream found!");
            cerr << m_warningMessage << mindev::endl;
            m_outlets[m_infoOutIndex]->send(m_warningMessage);
            return {};
        }
    };

    mindev::queue<mindev::placeholder::none> logInfoMessage { this,
        MIN_FUNCTION
        {
            // std::string message("Warning: No LSL Stream found!");
            cout << m_infoMessage << mindev::endl;
            m_outlets[m_infoOutIndex]->send(m_infoMessage);
            return {};
        }
    };

    // Get data pipeline. startMetro --> metro --> getDataOnMain --> call_getData
    mindev::timer<> metro { this, 
        MIN_FUNCTION {
            getDataOnMain.set();

            if (m_running)
                metro.delay(m_metroSpeed);

            return {};
        }
    };

    mindev::queue<mindev::placeholder::none> getDataOnMain { this,
        MIN_FUNCTION
        {
            call_getData(0);
            return {};
        }
    };

    void call_getData(int inlet)
    {
        std::vector<double> equalTimeValues = m_equalTimestamps->readNew();
        std::vector<double> timeValues = m_timestamps->read(equalTimeValues.size());
        m_timestamps->setWritePos(m_equalTimestamps->getWritePos());
        m_timestamps->setReadPos(m_equalTimestamps->getReadPos());
        std::vector<double> interpValues;
        for (int i = 0; i < m_numChannels; i++)
        {
            std::vector<double> values = m_buffers[i]->read(equalTimeValues.size());
            m_buffers[i]->setWritePos(m_equalTimestamps->getWritePos());
            m_buffers[i]->setReadPos(m_equalTimestamps->getReadPos());
            if (values.size() != 0)
            {
                BML::Math::lerp(
                    equalTimeValues,  // x
                    timeValues,       // xp
                    values,           // yp
                    interpValues      // out
                );
                mindev::atoms outValues(interpValues.begin(), interpValues.end());
                m_outlets[i]->send(outValues);
            }
            mindev::atoms outTime(equalTimeValues.begin(), equalTimeValues.end());
            m_outlets[m_timestampIndex]->send(outTime);
        }
    }

    // Number of LSL channels message
    mindev::message<> nChannels { this, "nchannels", "Get the number of LSL channels",
        MIN_FUNCTION 
        {
            call_nChannels(inlet);
            return {};
        }
    };

    void call_nChannels(int inlet)
    {            
        m_outlets[m_infoOutIndex]->send(m_nLslChannels);
    }

    // sammple rate message
    mindev::message<> samplerateOut { this, "samplerate", "Get the sample rate",
        MIN_FUNCTION 
        { 
            call_samplerateOut(inlet); 
            return {};
        }
    };

    void call_samplerateOut(int inlet)
    {
        std::cout << "size of outlets: " << m_outlets.size() << "\n";
        std::cout << "m_infoOutIndex: " << m_infoOutIndex << "\n";
        m_outlets.at(m_infoOutIndex)->send(m_sr);
    }

    // On and off
    mindev::message<> onOff{ this, "int", "Turn LSL receiving on or off",
        MIN_FUNCTION
        {
            call_onOff(inlet, std::vector<int>(args.begin(), args.end()));
            return {};
        }
    };

    void call_onOff(int inlet, const std::vector<int>& args)
    {

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
                    std::ref(m_timestamps),
                    std::ref(m_equalTimestamps),
                    std::ref(m_outlets)
                );
                t.detach();
            }
            break;

        default:
            break;
        }
    }

    void initializeAndReadLsl(
        std::unique_ptr<lsl::stream_inlet>& lsl_inlet, 
        std::atomic_bool& running, 
        std::vector<std::unique_ptr<BML::CircularBuffer>>& bufs,
        std::unique_ptr<BML::CircularBuffer>& timestamps,
        std::unique_ptr<BML::CircularBuffer>& equalTimestamps,
        std::vector<std::unique_ptr<mindev::outlet<>>>& outlets)
    {
        // Don't initialize more than one
        if (running)
        {
            return;
        }

        std::vector<lsl::stream_info> results;
        {
            m_infoMessage = "Looking for LSL stream...";
            logInfoMessage.set();
        }
        if ((getStreamProperty() == "") || (getStreamPropertyValue() == ""))
            results = lsl::resolve_stream("type", "EEG", 1, 2.0);
        else
        {
            results = lsl::resolve_stream(getStreamProperty(), getStreamPropertyValue(), 1, 2.0);
        }

        if (results.size() == 0)
        {
            m_warningMessage = "No LSL Stream Found";
            logWarningMessage.set();
            std::cout << m_warningMessage << "\n";
            return;
        }

        // Get lsl stream
        m_lslInlet = std::make_unique<lsl::stream_inlet>(results.at(0));
        m_sr = m_lslInlet->info().nominal_srate();
        m_nLslChannels = m_lslInlet->info().channel_count();

        // Log data about the stream
        m_infoMessage = "LSL Stream Found!";
        logInfoMessage.set();
        std::stringstream ss;
        ss << "Number of LSL channels: " << m_nLslChannels << ". LSL sample rate: " << m_sr << ".";
        m_infoMessage = ss.str();
        logInfoMessage.set();
        std::cout << m_infoMessage << "\n";

        // Start reading the stream
        std::vector<double> samples;
        double firstTimestamp = m_lslInlet->pull_sample(samples);
        double timestamp = firstTimestamp;
        double equalTimestamp = 0.0;
        running = true;
        metro.delay(0.0);
        while (running)
        {
            for (int i = 0; i < m_numChannels; i++)
            {
                if (samples.size() > i)
                    bufs[i]->write(samples[i]);
            }
            timestamps->write(timestamp - firstTimestamp);
            equalTimestamps->write(equalTimestamp);

            timestamp = m_lslInlet->pull_sample(samples);
            equalTimestamp += 1.0/m_sr;
        }

        m_sr = 0.0;
        m_nLslChannels = 0.0;
        m_lslInlet.reset();
    }

    std::string getStreamPropertyValue() { return m_streamPropValue; }
    std::string getStreamProperty() { return m_streamProperty; }

    int getNumChannels() { return m_numChannels; }
    
    bool lslRunning() { return m_running.load(); }
    mindev::atom getSr() { return m_sr; }
    mindev::atom getNumLslChannels() { return m_nLslChannels; }


private:
    std::atomic_bool m_running;
    std::unique_ptr<BML::CircularBuffer> m_timestamps;
    std::unique_ptr<BML::CircularBuffer> m_equalTimestamps;
    std::vector<std::unique_ptr<BML::CircularBuffer>> m_buffers;
    std::mutex m_mut;
    std::string m_mess;
    std::unique_ptr<lsl::stream_inlet> m_lslInlet;

    std::vector<std::unique_ptr<mindev::outlet<>>> m_outlets;
    int m_infoOutIndex;
    int m_timestampIndex;
    int m_numChannels;
    mindev::symbol m_streamProperty;
    mindev::symbol m_streamPropValue;

    double m_sr;
    int32_t m_nLslChannels;
    double m_metroSpeed;

    std::string m_warningMessage;
    std::string m_infoMessage;
};

MIN_EXTERNAL(BMLInput);  
