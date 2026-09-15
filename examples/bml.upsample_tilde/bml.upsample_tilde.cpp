#include "c74_min.h"
#include "math.h"
#include <algorithm>

#include "bml-dsp/circular-buffer.h"
#include "bml-dsp/realtime.h"

namespace mindev = c74::min;

class BMLUpsample : public mindev::object<BMLUpsample>, public mindev::vector_operator<> 
{
public:

    MIN_DESCRIPTION	{ "" };  // Description of the object
    MIN_TAGS		{ "" };  // Any tags to include
    MIN_AUTHOR		{ "" };  // Author of the objectcmake 
    MIN_RELATED		{ "" };  // Related Max Objects

    BMLUpsample() :
        m_frequency(220.0),
        m_audioSamplerate(0.0),
        m_on(false),
        m_buf(2000),
        m_resample()
    {}


    /* Inlets and outlets take three arguments inside the curly braces
    - An owner object (typically this)
    - A description
    - A type (as a string).
    See documentation for more info. */

    mindev::inlet<>  set_freq {this, "(list) LSL Data In", "list"};
    mindev::inlet<> initialize{ this, "Initialize Resampler", "bang" };
    mindev::outlet<> osc_out  { this, "Oscillator out", "signal" };
    mindev::outlet<> logs{ this, "Logs out", "list" };

    mindev::message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION{
            m_audioSamplerate = static_cast<double>(args[0]);
            return {};
        } 
    };

    mindev::message<> dataIn{ this, "list", "Raw LSL In",
        MIN_FUNCTION {

            m_buf.write(std::vector<double>(args.begin(), args.end()));
            
            return {};
        }
    };

    mindev::attribute<double> m_lslSamplerate{ this, "lslSamplerate", 512 };

    mindev::message<> initializeSampler{ this, "bang", "Initialize Resampler",
        MIN_FUNCTION
        {
            if (inlet != 1) return {};

            if (m_audioSamplerate != 0.0)
                m_resample = std::make_unique<BML::RealTime::Resample>(m_lslSamplerate, m_audioSamplerate);

            mindev::atoms outList;
            for (int i = 0; i < 2; i++)
            {
                std::stringstream ss;
                if (i == 0)
                {
                    ss << "LSL Sample Rate: " << m_lslSamplerate;
                }
                else
                {
                    ss << "Audio Sample Rate: " << m_audioSamplerate;
                }
                outList.push_back(ss.str());
            }
            logs.send(outList);

            return {};
        }
    };

    void operator()(mindev::audio_bundle input, mindev::audio_bundle output)
    {
        double numChannels = static_cast<double>(output.channel_count());
        double frameCount = static_cast<double>(output.frame_count());

        int numSamples =  static_cast<int>(floor( m_lslSamplerate * (frameCount / 44100.0) ));

        if (m_resample)
        {
            auto outVec = m_resample->resample(m_buf.read(numSamples));

            cout << outVec.size() << mindev::endl;
            //cou
        }

        for (size_t ch = 0; ch < static_cast<size_t>(numChannels); ch++)
        {
            std::for_each(
                output.samples(ch),
                output.samples(ch) + static_cast<size_t>(frameCount),
                [this](double& val)
                {
                    m_time += 1.0 / m_audioSamplerate;
                    val = std::sin(2.0 * 3.141592 * m_frequency * m_time);
                });
        }
    };

private:
    double m_audioSamplerate;
    double m_frequency;
    bool m_on;
    double m_time;
    BML::CircularBuffer m_buf;
    std::unique_ptr<BML::RealTime::Resample> m_resample;
};


MIN_EXTERNAL(BMLUpsample);  // Wrap the class name in the MIN_EXTERNAL macro
