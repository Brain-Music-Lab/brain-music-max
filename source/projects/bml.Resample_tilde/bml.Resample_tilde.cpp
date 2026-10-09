#define _CRT_SECURE_NO_WARNINGS

#include <sstream>
#include <memory>
#include <cassert>

#include "c74_min.h"  // Must include to access min devkit
#include "bml-dsp/circular-buffer.h"
#include "bml-dsp/realtime.h"

namespace mindev = c74::min;

const int DATA_INLET = 0;
const int INFO_INLET = 1;

class BMLResample : public mindev::object<BMLResample>, public mindev::vector_operator<>
{

public:

    BMLResample(const mindev::atoms& args = {}) :
        m_timestampIndex(0),
        m_numChannels(8),
        m_oldFs(512.0),
        m_newFs(0.0),
        m_transitionBandwidth(50.0),
        m_inlets(),
        m_outlets(),
        m_inputTime(),
        m_outputTime(),
        m_inputBuffers(),
        m_outputBuffers(),
        m_resamplers(),
        m_frameCount(0),
        m_channelCount(0),
        m_infoOutIdx(0)
    {
        if (args.size() > 0)
            m_numChannels = args[1];

        for (int i = 0; i <= m_numChannels; i++)
        {
            std::stringstream ssIn;
            std::stringstream ssOut;

            if (i == m_numChannels)
            {
                ssIn << "Timestamps In";
                ssOut << "Timestamps Out";
                m_timestampIndex = i;
            }
            else
            {
                ssIn << "LSL In" << i + 1;
                ssOut << "LSL Out " << i + 1;
            }

            auto inlet = std::make_unique<mindev::inlet<>>(this, ssIn.str(), "list");
            m_inlets.push_back(std::move(inlet));

            auto outlet = std::make_unique<mindev::outlet<>>(this, ssOut.str(), "signal");
            m_outlets.push_back(std::move(outlet));
        }


        auto outlet = std::make_unique<mindev::outlet<>>(this, "dumpout", "list");
        m_outlets.push_back(std::move(outlet));

        for (int i = 0; i < m_numChannels; i++)
        {
            auto buffer = std::make_unique<BML::CircularBuffer>(5);
            m_inputBuffers.push_back(std::move(buffer));
            m_outputBuffers.push_back(nullptr);
            m_resamplers.push_back(nullptr);
        }

        m_infoOutIdx = m_numChannels + 1;
    }

    ~BMLResample()
    {
        
    }

    MIN_DESCRIPTION{ "" };  // Description of the object
    MIN_TAGS{ "" };  // Any tags to include
    MIN_AUTHOR{ "Daniel Ethridge" };  // Author of the objectcmake
    MIN_RELATED{ "" };  // Related Max Objects

    void call_dataIn(int inlet, const mindev::atoms& args) 
    {
        for (size_t i = 0; i < args.size(); i++)
        {
            if (args[i].type() != mindev::message_type::float_argument)
            {
                std::stringstream ss;
                ss << "Warning: Invalid data at inlet " << inlet << "!";
                cerr << ss.str() << mindev::endl;
                // m_outlets[m_dumpOutIndex]->send(message);

                return;
            }
        }

        if (inlet == m_timestampIndex)
        {
            m_inputTime.get()->write(std::vector<double>(args.begin(), args.end()));
        }
        else
        {
            m_inputBuffers[inlet].get()->write(std::vector<double>(args.begin(), args.end()));
            m_outputBuffers[inlet].get()->write(
                m_resamplers[inlet].get()->operator()(std::vector<double>(args.begin(), args.end()))
            );
        }
    }

    mindev::message<> dataIn {this, "list", "Input Data",
        MIN_FUNCTION 
        {
            call_dataIn(inlet, args);
            return {};
        }
    };

    mindev::message<> dspsetup {this, "dspsetup",
        MIN_FUNCTION {
            m_newFs = static_cast<double>(args[0]);

            size_t halfSamplerate;
            if ((static_cast<size_t>(m_newFs) % 2) == 0)
                halfSamplerate = static_cast<size_t>(m_newFs/2.0);
            else
                halfSamplerate = static_cast<size_t>((m_newFs-1) / 2.0);

            for (int i = 0; i < m_outputBuffers.size(); i++)
            {
                // m_outputBuffers[i] = std::make_unique<BML::CircularBuffer>(halfSamplerate);
                m_outputBuffers[i] = std::make_unique<BML::CircularBuffer>(10);
                m_resamplers[i] = std::make_unique<BML::RealTime::Resample>(m_oldFs, m_newFs, m_transitionBandwidth);
            }

            return {};
        }
    };

    mindev::queue<mindev::placeholder::none> output_channel_count { this,
        MIN_FUNCTION
        {
            cout 
                << "frame count - " 
                << m_frameCount << " :: channel count - " 
                << m_channelCount 
                << mindev::endl;
            return {};
        }
    };

    void operator()(mindev::audio_bundle input, mindev::audio_bundle output)
    {
        // if (m_frameCount != output.channel_count() || m_channelCount != output.frame_count())
        // {
        //     m_frameCount = output.frame_count();
        //     m_channelCount = output.channel_count();
        //     output_channel_count.set();
        // }
        size_t numChannels = static_cast<size_t>(output.channel_count());
        size_t frameCount = static_cast<size_t>(output.frame_count());

        std::vector<double> resampledData;
        for (size_t i = 0; i < numChannels; i++)
        {
            resampledData = m_outputBuffers[i].get()->read(frameCount);
            std::transform(
                resampledData.begin(),
                resampledData.end(),
                output.samples(i),
                [](double value) { return value; }
            );
        }
    }

private:
    int m_numChannels;
    int m_timestampIndex;

    double m_oldFs;
    double m_newFs;
    double m_transitionBandwidth;

    std::vector<std::unique_ptr<mindev::inlet<>>> m_inlets;
    std::vector<std::unique_ptr<mindev::outlet<>>> m_outlets;

    std::unique_ptr<BML::CircularBuffer> m_inputTime;
    std::unique_ptr<BML::CircularBuffer> m_outputTime;
    std::vector<std::unique_ptr<BML::CircularBuffer>> m_inputBuffers;
    std::vector<std::unique_ptr<BML::CircularBuffer>> m_outputBuffers;

    std::vector<std::unique_ptr<BML::RealTime::Resample>> m_resamplers;

    long m_frameCount;
    long m_channelCount;

    size_t m_infoOutIdx;
};

MIN_EXTERNAL(BMLResample);  
