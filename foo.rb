require 'beepboop'

class MySong < Beepboop::Song
  name "hi"
  comment "this is an example"
  author "wilkie"
  tempo 100

  instrument :piano do
    attack 40
  end

  instrument :bassPan do
    attack   7
    decay    5
    sustain  15
    release  15
    volume   63
    scale    2
    waveform 0
  end

  pattern :foo do
    using :bassPan do
      step a♯3, c3 * 4, e3 * 4
      step [a♯3, ab3]
      step [a3 * 2, b3]
    end

    using :piano do
      step a3
      step a3
      step a3
    end
  end

  pattern :bar, :bassPan do
    step a3 * 10, :volume => [60, 0, 10], :shift => [10, 0, 2] 
  end

  pattern :bar
  pattern :foo
end

# step NOTE, EVENT
# step [NOTES], EVENT, [NOTES]
# arrays divide a step, multiplication enlongates
# step [a3,a3,a3]    <-- triplet
# step [a3,a3]       <-- eighth
# step [a3,a3,a3,a3] <-- sixteenth
# step a3, :shift => [0, 10, 3] <-- press a3, shift from 0 to 10 over 3 steps
# step a3, c3, e3, :shift => [0, 10, 3] <--- play coord, shift the same
# step a3, c3, :shift => [0, 10, 3], e3 <-- shift first two notes??

#song "My Song" do
#  instrument :piano do
#    attack 40
#  end
#end