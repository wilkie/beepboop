$:.unshift File.dirname(__FILE__) + "/../ext"
$:.unshift File.dirname(__FILE__)

require_relative '../beepboop'
require 'beepboop/audio_device'
require 'beepboop/opl'
require 'beepboop/player'
require 'beepboop/players'
require 'beepboop/players/rdos'
