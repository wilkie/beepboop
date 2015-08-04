$:.unshift File.dirname(__FILE__) + "/../ext"
$:.unshift File.dirname(__FILE__)

module Beepboop
end

# C extensions
require 'beepboop/beepboop'

# Rest of the stuff
require 'beepboop/dsl'
require 'beepboop/song'
require 'beepboop/audio_device'
require 'beepboop/opl'
require 'beepboop/player'
require 'beepboop/players'
require 'beepboop/players/rdos'