module Beepboop
  # This class represents an OPL 2/3 audio chip. You initialize this chip with
  # the sample rate for your audio device. Based on that, it will generate tones
  # based on an internal clock. You can write to the data registers at any point
  # between sampling the chip using #write. Sample the chip using #sample
  # passing along a buffer to write the audio waveform data to.
  class OPL
    # (This is implemented in ext/beepboop/beepboop.c)
    def initialize(sample_rate)
    end

    # (This is implemented in ext/beepboop/beepboop.c)
    def write(register, data)
    end

    # (This is implemented in ext/beepboop/beepboop.c)
    def reset()
    end

    # (This is implemented in ext/beepboop/beepboop.c)
    def sample(output, numSamples)
    end
  end
end
