#define _CRT_SECURE_NO_WARNINGS

#include <sstream>
#include <memory>
#include <cassert>

#include "c74_min.h"  // Must include to access min devkit
#include "bml-dsp/circular-buffer.h"
#include "bml-dsp/realtime.h"
#include <vector>

namespace mindev = c74::min;

const int DATA_INLET = 0;
const int INFO_INLET = 1;

class BMLResample : public mindev::object<BMLResample>, public mindev::vector_operator<>
{

public:

    BMLResample(const mindev::atoms& args = {}) :
        m_infoOutIdx(1),
        m_numChannels(8),
        m_oldFs(512.0),
        m_newFs(0.0),
        m_transitionBandwidth(150.0),
        m_inlets(),
        m_outlets(),
        m_outputBuffers(),
        m_resamplers(),
        m_frameCount(0),
        m_channelCount(0),
        m_millisecondsBetweenReadies(250.0),
        m_dspSetupComplete(false),
        m_running(false),
        m_readyForLsl(false),
        m_dataToProcess(false),
        m_input()
    {
        if (args.size() > 0)
            m_numChannels = args[0];

        for (int i = 0; i < m_numChannels; i++)
        {
            std::stringstream ssIn;
            std::stringstream ssOut;

            if (i == 0)
            {
                ssIn << "Toggle On/Off, LSL In 1";
                ssOut << "LSL Out 1";
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

            m_resamplers.push_back(nullptr);
            m_input.push_back({});           
            m_resampleResult.push_back({});
            m_outputBuffers.push_back(nullptr);
            m_dataToProcess.push_back(std::make_unique<std::atomic_bool>(false));
        }

        auto outlet = std::make_unique<mindev::outlet<>>(this, "Info out", "list");
        m_outlets.push_back(std::move(outlet));
        m_infoOutIdx = static_cast<int>(m_outlets.size() - 1);
    }

    ~BMLResample()
    {
        
    }

    MIN_DESCRIPTION{ "" };  // Description of the object
    MIN_TAGS{ "" };  // Any tags to include
    MIN_AUTHOR{ "Daniel Ethridge" };  // Author of the objectcmake
    MIN_RELATED{ "" };  // Related Max Objects

    mindev::argument<int> channelsArg {this, "channels", "Number of channels in the LSL stream to process",
        MIN_ARGUMENT_FUNCTION 
        {
            m_numChannels = arg;
        }
    };

// On and off
    mindev::message<> onOff{ this, "int", "Turn LSL receiving on or off",
        MIN_FUNCTION
        {
            call_onOff(inlet, static_cast<int>(args[0]));
            return {};
        }
    };

    void call_onOff(int inlet, int onOffValue)
    {
        if (inlet != 0) return;
        std::stringstream ss;

        switch (onOffValue) 
        {
        case 0:
            if (m_running) 
            {
                m_running.store(false);
                m_readyForLsl.store(false);
            }
            break;

        case 1:
            if (!m_running) 
            {
                m_running.store(true);
                for (int i = 0; i < static_cast<int>(m_inlets.size()); i++)
                {
                    m_dataToProcess[i]->store(false);
                    m_input[i].clear();
                    m_resampleResult[i].clear();
                    m_dataToProcess[i]->store(false);

                    std::thread t(
                        &BMLResample::dataInThread,
                        this,
                        std::ref(m_resamplers[i]),
                        std::ref(m_input[i]),
                        std::ref(m_resampleResult[i]),
                        std::ref(m_outputBuffers[i]),
                        std::ref(m_dataToProcess[i]),
                        std::ref(m_running),
                        std::ref(m_readyForLsl),
                        std::ref(m_infoMessage),
                        std::ref(m_warningMessage)
                    );
                    t.detach();
                }
                
                startResampling.set();

            }
            break;

        default:
            break;
        }
    }

    void dataInThread(
        std::unique_ptr<BML::RealTime::Resample>& resampler,
        std::vector<double>& dataToResample,
        std::vector<double>& resampleResult,
        std::unique_ptr<BML::CircularBuffer>& outputBuffers,
        std::unique_ptr<std::atomic_bool>& dataToProcess,
        std::atomic_bool& running,
        std::atomic_bool& readyForLsl,
        std::string& infoMessage,
        std::string& warningMessage
    )
    {
        while (running)
        {
            if (!dataToProcess->load() || !readyForLsl.load())
                continue;

            dataToProcess->store(false);

            resampler.get()->operator()(dataToResample, resampleResult);
            outputBuffers.get()->write(resampleResult);

            if (dataToProcess->load())
            {
                warningMessage = "Resampling is going to slow. Data packet lost.";
                logWarningMessage.set();
            }
            dataToProcess->store(false);
        }
    }

    void call_samplerate()
    {
        cout << "Old sample rate: " << m_oldFs << " . New sample rate: " <<  m_newFs << mindev::endl;
        m_outlets[m_infoOutIdx]->send(mindev::atoms({m_oldFs, m_newFs}));
    }

    mindev::message<> samplerateOut { this, "samplerate", "Get the sampling rate",
        MIN_FUNCTION
        {
            call_samplerate();
            return {};
        }
    };

    mindev::message<> dataIn {this, "list", "Input Data",
        MIN_FUNCTION 
        {
            if (!m_dspSetupComplete)
            {
                return {};
            }

            for (size_t i = 0; i < args.size(); i++)
            {
                if (args[i].type() != mindev::message_type::float_argument)
                {
                    std::stringstream ss;
                    ss << "Warning: Invalid data at inlet " << inlet << "!";
                    cerr << ss.str() << mindev::endl;
                    return {};
                }
            }
            call_dataIn(inlet, std::vector<double>(args.begin(), args.end()));
            return {};
        }
    };

    void call_dataIn(int inlet, const std::vector<double>& data)
    {
        m_input[inlet] = data;
        m_dataToProcess[inlet]->store(true);
    }
    
    mindev::message<> dspsetup {this, "dspsetup",
        MIN_FUNCTION {
            m_newFs = static_cast<double>(args[0]);

            for (int i = 0; i < m_outputBuffers.size(); i++)
            {
                m_outputBuffers[i] = std::make_unique<BML::CircularBuffer>(m_newFs);
            }

            m_dspSetupComplete = true;
            startResampling.set();
            return {};
        }
    };

    mindev::queue<mindev::placeholder::none> startResampling { this, 
        MIN_FUNCTION
        {
            if (m_dspSetupComplete && m_running)
            {
                for (int i = 0; i < m_outputBuffers.size(); i++)
                {
                    m_outputBuffers[i]->resetBuffer();
                    m_resamplers[i] = std::make_unique<BML::RealTime::Resample>(m_oldFs, m_newFs, m_transitionBandwidth);
                }

                m_readyForLsl.store(true);
                engageResampler.delay(m_millisecondsBetweenReadies);
            }

            return {};
        }
    };

    mindev::timer<> engageResampler { this, 
        MIN_FUNCTION {
            for (int i = 0; i < m_outputBuffers.size(); i++)
            {
                m_outputBuffers[i]->setReadPos(0);
            }
            return {};
        }
    };

    mindev::queue<mindev::placeholder::none> logWarningMessage { this,
        MIN_FUNCTION
        {
            // std::string message("Warning: No LSL Stream found!");
            cerr << m_warningMessage << mindev::endl;
            m_outlets[m_infoOutIdx]->send(m_warningMessage);
            return {};
        }
    };

    mindev::queue<mindev::placeholder::none> logInfoMessage { this,
        MIN_FUNCTION
        {
            // std::string message("Warning: No LSL Stream found!");
            cout << m_infoMessage << mindev::endl;
            m_outlets[m_infoOutIdx]->send(m_infoMessage);
            return {};
        }
    };

    void operator()(mindev::audio_bundle input, mindev::audio_bundle output)
    {
        if (!m_running.load())
        {
            output.clear();
            return;
        }

        size_t numChannels = static_cast<size_t>(output.channel_count());
        size_t frameCount = static_cast<size_t>(output.frame_count());

        // std::vector<double> resampledData;
        for (size_t i = 0; i < numChannels; i++)
        {
            auto out = output.samples(i);
            auto resampledData = m_outputBuffers[i].get()->read(frameCount);
            // std::copy(resampledData.begin(), resampledData.end(), output.samples(i));
            std::transform(
                resampledData.begin(),
                resampledData.end(),
                output.samples(i),
                [](double value) { return value; }
            );
        }
    }

    size_t TapsInResampleFilter()
    {
        return m_resamplers[0].get()->Taps();
    }

private:
    int m_numChannels;
    int m_infoOutIndex;

    double m_oldFs;
    double m_newFs;
    double m_transitionBandwidth;

    std::vector<std::unique_ptr<mindev::inlet<>>> m_inlets;
    std::vector<std::unique_ptr<mindev::outlet<>>> m_outlets;

    // std::unique_ptr<BML::CircularBuffer> m_inputTime;
    // std::unique_ptr<BML::CircularBuffer> m_outputTime;
    // std::vector<std::unique_ptr<BML::CircularBuffer>> m_inputBuffers;

    std::vector<std::unique_ptr<BML::RealTime::Resample>> m_resamplers;
    std::vector<std::vector<double>> m_input;
    std::vector<std::vector<double>> m_resampleResult;
    std::vector<std::unique_ptr<BML::CircularBuffer>> m_outputBuffers;
    std::vector<std::unique_ptr<std::atomic_bool>> m_dataToProcess;
    std::atomic_bool m_running;
    std::atomic_bool m_readyForLsl;
    std::string m_infoMessage;
    std::string m_warningMessage;

    long m_frameCount;
    long m_channelCount;

    size_t m_infoOutIdx;

    std::atomic_bool m_dspSetupComplete;

    double m_millisecondsBetweenReadies;

};

MIN_EXTERNAL(BMLResample);  
