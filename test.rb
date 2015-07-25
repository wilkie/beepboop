require 'ruby-sdl-ffi/sdl'

SAMPLE_RATE = 49716
SAMPLES     = 4096

sdl_sample_rate = SAMPLE_RATE
sdl_samples = SAMPLES

def opl3_out(reg, data)
#ifdef USE_DOSBOX_OPL
#  dbopl_device.WriteReg(reg, data);
#else
#  int op = 0;
#  if (reg > 255) {
#    op = 2;
#    reg &= 0xff;
#  }
#ifdef USE_YMF262_OPL
#  YMF262Write(ymf262_device_id, op,   reg);
#  YMF262Write(ymf262_device_id, op+1, data);
#else
#  OPLWrite(fmopl_device, op,   reg);
#  OPLWrite(fmopl_device, op+1, data);
#endif
#endif
end

class CurrentPlayback
  attr_reader :del
  attr_reader :speed

  def initialize(fromFile = nil)
    @buffer = StringIO.new

    if fromFile
      @buffer = open(fromFile, 'r')
    end

    @del = 0
    @speed = 0
  end

  def playPair
    setspeed = 0

    param = 0
    command = 0

    if @del > 0
      @del -= 1
      return
    end

    loop do
      setspeed = false

      param   = @buffer.readbyte
      command = @buffer.readbyte

      case command
      when 0
        @del = param - 1
        if param == 0
          @del = 255
        end
      when 2
        if param == 0
          @speed = command << 8
          setspeed = true
        else
          setchip(param - 1)
        end
      when 0xff
        if param == 0xff
          puts "END OF SONG"
          return
        end
      else
        opl3_out(command, param)
      end
      break if ((command || setspeed) && !@buffer.eof())
    end
  end
end

playing = CurrentPlayback.new("goblins-bar.raw")

def callback(userdata, stream, length)
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

def init
  # Paranoia initialization of SDL (doesn't seem necessary)
  SDL.Init(SDL::INIT_AUDIO)

  # Create current playing song structure

  # Initialize the YMF Module
#ifdef USE_DOSBOX_OPL
  #dbopl_device.Init(sdl_sample_rate);
#elif defined(USE_YMF262_OPL)
  #ymf262_device_id = YMF262Init(1, OPL3_INTERNAL_FREQ, sdl_sample_rate);
  #YMF262ResetChip(ymf262_device_id);
#else
  #fmopl_device = OPLCreate(OPL_TYPE_YM3812, 3579545, sdl_sample_rate);
  #OPLResetChip(fmopl_device);
#endif

  # 4-waveform Select (paranoia)
  opl3_out(1, 32);

  # Enable OPL3
  opl3_out(105, 1);

  # Initialize SDL Audio
  sdl_audio_spec = SDL::AudioSpec.new
  sdl_audio_spec.freq = SAMPLE_RATE
  sdl_audio_spec.format = SDL::AUDIO_S16
  sdl_audio_spec.channels = 2
  sdl_audio_spec.samples = SAMPLES
  sdl_audio_spec.callback = method(:callback)

  have = SDL::AudioSpec.new

  ret = SDL::OpenAudio(sdl_audio_spec, have)

  if ret < 0
    # Error initializing audio
    puts "error initializing"
    return -1
  end

  puts "opened device, sample rate: #{have.freq}, samples: #{have.samples}"

  sdl_sample_rate = have.freq
  sdl_samples = have.samples

  0
end

def uninit
  # Uninit SDL
  SDL::PauseAudio(1)
  SDL::CloseAudio()

  # Uninit YMF Module
  #YMF262Shutdown();
  #OPLDestroy(fmopl_device);
end

init
SDL::PauseAudio(0)
sleep(1000)
uninit

ret = 0

if ret == -1
  puts "Error: Cannot Open Audio Device"
  exit -1
end