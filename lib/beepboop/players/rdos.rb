module Beepboop
  module Players
    require 'beepboop/players/opl'

    register(
      :name => 'RDos Raw Adlib', 
      :extension => 'raw',
      :description => 'This format was used by the Rdos Capture program which intercepted AdLib OPL writes to the I/O port and wrote them to this file',
      :classname => 'RDos'
    )

    # This module implements the RDOS (.raw) OPL audio tracker file.
    class RDos
      include Beepboop::Players::OPL

      # Checks to see if the given stream is an RDOSPLAY file. Returns true
      # when the stream is valid, and false otherwise.
      def self.test(stream)
        stream.read(8) == "RAWADATA"
      end

      # Creates an instance of this player with the given sample_rate.
      def initialize(stream, sample_rate)
        @opl = Beepboop::OPL.new(sample_rate)
        @sample_rate = sample_rate

        self.reset

        # Skip past the signature
        stream.read(8)

        # Read initial speed
        @speed = stream.read(2).unpack("S")[0]
      end

      def reset
        @opl.reset

        @skip = 0
        @speed = 0
        @minicnt = 0

        @done = false
      end

      # Reads from the input stream, writes to the output stream the number of
      # bytes requested.
      def sample(stream, output, length)
        puts output.address
        #ifdef USE_DOSBOX_OPL
        #  DBOPL::OPLMixer oplmixer((int16_t*)output);
        #endif

        if @done
          return false
        end

        towrite = length / 4

        while towrite > 0
          while @minicnt < 0
            @minicnt += @sample_rate
            self.playPair(stream)
          end

          refresh = 1193180.0 / (@speed > 0 ? @speed : 0xffff).to_f

          i = (@minicnt / refresh + 4).to_i & ~3
          if (towrite < i)
            i = towrite
          end

          #ifdef USE_DOSBOX_OPL
          #    dbopl_device.Generate(&oplmixer, i);
          #elif defined(USE_YMF262_OPL)
          #    YMF262UpdateOne(ymf262_device_id, stream_pos, nil, i);
          #else
          #    YM3812UpdateOne(fmopl_device, output, i);
          #    // Duplicate channels
          #    i.downto 1 do |p|
          #      output.put_uint8(p*2-1, output.get_uint8(p-1))
          #      output.put_uint8(p*2-2, output.get_uint8(p-1))
          #    end
          #endif

          output += (i * 2 * 2) # 2 channels * 2 bytes per sample
          towrite -= i

          @minicnt -= (refresh * i).to_i
        end

        true
      end

      def setchip(chip)
      end

      # Reads from the input stream and interprets the commands within. Those
      # commands are sent to the (emulated) OPL chip.
      def playPair(stream)
        setspeed = false

        param = 0
        command = 0

        if @skip > 0
          @skip -= 1
          return
        end

        return if stream.eof?

        loop do
          setspeed = false

          param   = stream.getbyte
          command = stream.getbyte

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
              @done = true
              return
            end
          else
            @opl.write(command, param)
          end
          break if ((command == 0 && setspeed == false) || stream.eof?)
        end
      end
    end
  end
end