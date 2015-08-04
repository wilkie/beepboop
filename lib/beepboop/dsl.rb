module Beepboop
  module DSL
    def name(name)
      puts "Defining song #{name}"
    end

    def comment(name)
    end

    def tempo(time)
    end

    def author(name)
    end

    class InstrumentFactory
      def initialize(name)
        @options = {}
        @options[:name] = name
      end

      def attack(value)
        puts "Setting attack to #{value}"
        @options[:attack] = value
      end

      def decay(value)
        puts "Setting decay to #{value}"
        @options[:decay] = value
      end

      def sustain(value)
        puts "Setting sustain to #{value}"
        @options[:sustain] = value
      end

      def release(value)
        puts "Setting release to #{value}"
        @options[:release] = value
      end

      def waveform(value)
        puts "Setting waveform to #{value}"
        @options[:waveform] = value
      end

      def scale(value)
        puts "Setting scale to #{value}"
        @options[:scale] = value
      end

      def volume(value)
        puts "Setting volume to #{value}"
        @options[:volume] = value
      end

      def frequency(value)
        puts "Setting frequency to #{value}"
        @options[:frequency] = value
      end

      def build
        Instrument.new(@options)
      end
    end

    class Instrument
      attr_reader :name
      attr_reader :attack
      attr_reader :decay
      attr_reader :sustain
      attr_reader :release
      attr_reader :waveform
      attr_reader :scale
      attr_reader :volume
      attr_reader :frequency

      def initialize(options)
        @name      = options[:name]      || "unnamed"
        @attack    = options[:attack]    || 0
        @decay     = options[:decay]     || 0
        @sustain   = options[:sustain]   || 0
        @release   = options[:release]   || 0
        @waveform  = options[:waveform]  || 0
        @scale     = options[:scale]     || 0
        @volume    = options[:volume]    || 0
        @frequency = options[:frequency] || 0
      end
    end

    class Note
      attr_reader :name
      attr_reader :length

      attr_accessor :step

      def initialize(note, step=nil)
        @name = note
        @length = 1
        @step = step
      end

      def *(length)
        @length = length
        self
      end
    end

    class PatternFactory
      def method_missing(name, *args)
        Note.new(name)
      end

      def using(instrument, *args, &block)
        @instrument = instrument
        yield
        @instrument = nil
      end

      def step(*args)
        puts "Step #{@step}"
        args.each do |arg|
          case arg
          when Note
            arg.step = @step
            puts "#{@instrument} @#{@step} #{arg.name} for #{arg.length}"
          when Array
            tickLength = arg.map(&:length).reduce(&:+)
            substep = @step.to_f
            arg.each do |note|
              puts "#{@instrument} @#{substep} #{note.name} for #{note.length}/#{tickLength}"
              substep += (note.length.to_f / tickLength.to_f)
            end
          when Hash
            volume = arg[:volume]
            if volume
              puts "Volume shift from #{volume[0]} to #{volume[1]} in #{volume[2]} steps"
              steps = volume[2].to_f
              last  = volume[1].to_i
              first = volume[0].to_i

              current = 0
              value = first

              # steps per amount
              increment = steps / (last - first)

              if first > last
                reversed = -1
                last, first = first, last
              else
                reversed = 1
              end

              (first..last).each do |i|
                puts "@#{(@step.to_f + current).round(5)} volume = #{i}"
                current += (increment * reversed)
                current = current
              end
            end
          else
          end
        end
        @step += 1
      end

      def build
      end

      def initialize(options)
        @step = 0
        @instrument = options[:instrument]
        @name = options[:name] || "unnamed"
      end
    end

    class Pattern
    end

    def instrument(name, &block)
      puts "Crafting Instrument #{name}"

      @instruments ||= []
      factory = InstrumentFactory.new(name)
      factory.instance_eval(&block)
      @instruments << factory.build
    end

    def pattern(name, instrument=nil, &block)
      if not block
        puts "Playing pattern #{name}"
        return
      end

      puts "Crafting Pattern #{name}"

      @patterns ||= []
      pattern = PatternFactory.new(:name => name,
                                   :instrument => instrument)
      pattern.instance_eval(&block)
      @patterns << pattern.build
    end

    def step(*args)
    end

    def instruments
      @instruments
    end
  end
end