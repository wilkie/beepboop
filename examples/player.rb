require 'beepboop'

puts "Playing #{ARGV[0]}"
player = Beepboop::Player.new(ARGV[0])

player.play

while not player.done?
end