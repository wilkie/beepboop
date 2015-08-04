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

        self.reset(stream)
      end

      def reset(stream)
        @opl.reset

        @skip = 0
        @speed = 0
        @minicnt = 0

        @done = false

        # Skip past the signature
        stream.read(8)

        # Read initial speed
        @speed = stream.read(2).unpack("S")[0]
      end

      # Reads from the input stream, writes to the output stream the number of
      # bytes requested.
      def sample(stream, output, length)
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

          numSamples = (@minicnt / refresh + 4).to_i & ~3
          if (towrite < numSamples)
            numSamples = towrite
          end

          @opl.sample(output, numSamples)

          output += (numSamples * 2 * 2) # 2 channels * 2 bytes per sample
          towrite -= numSamples

          @minicnt -= (refresh * numSamples).to_i
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

        if stream.eof?
          @done = true
        end

        return if (stream.eof? || @done)

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
              # END OF SONG command
              @done = true

              return
            end
          else
            if command >= 0x40 && command <= 0x55
              instrument = command & 0x1f
              keyscale = param >> 6
              output   = param & 0x3f
              puts "#{instrument} Key Scale Level = #{keyscale}"
              puts "#{instrument} Output Level    = #{output}"
            end
            if command >= 0x60 && command <= 0x75
              instrument = command & 0x1f
              attack = param >> 4
              decay  = param & 0xf
              puts "#{instrument} Attack Rate     = #{attack}"
              puts "#{instrument} Decay Rate      = #{decay}"
            end
            if command >= 0x80 && command <= 0x95
              instrument = command & 0x1f
              sustain = param >> 4
              release = param & 0xf
              puts "#{instrument} Sustain Level   = #{sustain}"
              puts "#{instrument} Release Level   = #{release}"
            end
            if command >= 0xe0 && command <= 0xf5
              instrument = command & 0x1f              
              puts "#{instrument} Waveform Select = #{param & 0x7}"
            end
            @opl.write(command, param)
          end
          break if ((command == 0 && setspeed == false) || stream.eof?)
        end
      end
    end
  end
end