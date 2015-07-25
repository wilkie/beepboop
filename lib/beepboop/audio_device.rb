module Beepboop
  class AudioDevice
    require 'ruby-sdl-ffi/sdl'

    attr_reader :sample_rate
    attr_reader :sampling_size

    def callback(userdata, stream, length)
      if @callback
        @callback.call(userdata, stream, length)
      end
    end

    def initialize(callback = nil, sample_rate = 49716, sampling_size = 4096)
      @sample_rate   = sample_rate
      @sampling_size = sampling_size
      @callback      = callback

      # Initialize SDL Audio
      sdl_audio_spec          = SDL::AudioSpec.new
      sdl_audio_spec.freq     = @sample_rate
      sdl_audio_spec.format   = SDL::AUDIO_S16
      sdl_audio_spec.channels = 2
      sdl_audio_spec.samples  = @sampling_size
      sdl_audio_spec.callback = method(:callback)

      have = SDL::AudioSpec.new

      ret = SDL::OpenAudio(sdl_audio_spec, have)

      if ret < 0
        # Error initializing audio
        raise "Error Initializing Audio"
      end

      @sample_rate   = have.freq
      @sampling_size = have.samples

      ObjectSpace.define_finalizer(self, self.class.finalize() )
    end

    def resume
      SDL::PauseAudio(0)
    end

    def pause
      SDL::PauseAudio(1)
    end

    def self.finalize
      Proc.new {
        SDL::PauseAudio(1)
        SDL::CloseAudio()
      }
    end

    # Tear down and clean up any open AudioDevice
    def uninitialize
      self.finalize
    end
  end
end
