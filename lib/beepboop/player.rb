module Beepboop
  # This class represents a media player capable of playing our tracked stuff.
  class Player
    def initialize(source)
      @buffer = StringIO.new

      if source.is_a? String
        @buffer = open(source, 'r')
      end

      @skip = 0
      @speed = 0

      @audioDevice = Beepboop::AudioDevice.new(method(:callback))
      @opl = Beepboop::Opl.new(@audioDevice.sample_rate)
    end

    # TODO: move this into a player subclass for RDOS files
    def playOPLPair
      setspeed = false

      param = 0
      command = 0

      if @skip > 0
        @skip -= 1
        return
      end

      loop do
        setspeed = false

        param   = @buffer.readbyte
        command = @buffer.readbyte

        case command
        when 0
          @skip = param - 1
          if param == 0
            @skip = 255
          end
        when 2
          if param == 0
            @speed = command << 8
            setspeed = true
          else
            self.setchip(param - 1)
          end
        when 0xff
          if param == 0xff
            puts "END OF SONG"
            return
          end
        else
          @opl.write(command, param)
        end
        break if ((command || setspeed) && !@buffer.eof())
      end
    end

    def callback
#ifdef USE_DOSBOX_OPL
#  DBOPL::OPLMixer oplmixer((int16_t*)stream);
#endif

      minicnt = 0

      towrite = length / 4

      while towrite > 0
        puts "towrite #{towrite}"
        while minicnt < 0
          minicnt += sdl_sample_rate
          playing.playPair
        end

        refresh = 1193180.0 / (playing.speed ? playing.speed : 0xffff).to_f

        i = (minicnt / refresh + 4).to_i & ~3
        if (towrite < i)
          i = towrite
        end

    #ifdef USE_DOSBOX_OPL
    #    dbopl_device.Generate(&oplmixer, i);
    #elif defined(USE_YMF262_OPL)
    #    YMF262UpdateOne(ymf262_device_id, stream_pos, nil, i);
    #else
    #    YM3812UpdateOne(fmopl_device, stream, i);
    #    // Duplicate channels
    #    i.downto 1 do |p|
    #      stream.put_uint8(p*2-1, stream.get_uint8(p-1))
    #      stream.put_uint8(p*2-2, stream.get_uint8(p-1))
    #    end
    #endif
        stream += (i * 2 * 2) # 2 channels * 2 bytes per sample
        towrite -= i

        minicnt -= (refresh * i).to_i
      end
    end
  end
end
