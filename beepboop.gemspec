# -*- encoding: utf-8 -*-
$:.push File.expand_path("../lib", __FILE__)
require "beepboop/version"

Gem::Specification.new do |s|
  s.name        = "beepboop"
  s.version     = Beepboop::VERSION
  s.platform    = Gem::Platform::RUBY
  s.authors     = ['wilkie']
  s.email       = ['wilkie@xomb.org']
  s.homepage    = "http://github.com/wilkie/beepboop"
  s.summary     = %q{}
  s.description = %q{}

  s.files         = `git ls-files`.split("\n")
  s.test_files    = `git ls-files -- {test,spec,features}/*`.split("\n")
  s.executables   = `git ls-files -- bin/*`.split("\n").map{ |f| File.basename(f) }

  s.extensions      = ['ext/beepboop/extconf.rb']

  s.require_paths = ["lib"]
end
