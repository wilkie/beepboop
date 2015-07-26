module Beepboop
  # This class represents a media player capable of playing our tracked stuff.

  class Player
    def initialize(source)
      @audioDevice = Beepboop::AudioDevice.new(method(:callback))

      @buffer = StringIO.new

      if source.is_a? String
        @buffer = open(source, 'rb')

        # Discover the player for this stream
        if not self.discoverPlayer()
          raise "No available player for this media type."
        end
      end

      self.stop
    end

    def discoverPlayer
      Beepboop::Players::players.each do |player|
        result = player[:class].test(@buffer)
        @buffer.rewind
        if result
          @player = player[:class].new(@buffer, @audioDevice.sample_rate)
          break
        end
      end

      if @player.nil?
        false
      else
        true
      end
    end

    def stop
      self.pause
      @done = false
      @buffer.rewind
      @player.reset(@buffer)
    end

    def done?
      @done
    end

    def play
      @audioDevice.resume
    end

    def pause
      @audioDevice.pause
    end

    def callback(userdata, stream, length)
      begin
        @done = !@player.sample(@buffer, stream, length)
      rescue Exception => e
        puts e.backtrace
        puts e
      end
    end
  end
end
