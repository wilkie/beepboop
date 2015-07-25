source 'https://rubygems.org'

# Specify your gem's dependencies in blooper.gemspec
gemspec

group :test do
  gem "rake"              # rakefile
  gem "minitest", "4.7.0" # test framework (specified here for prior rubies)
  gem "ansi"              # minitest colors
  gem "turn"              # minitest output
  gem "mocha"             # stubs
end

platforms :rbx do
end

gem "nice-ffi", :git => "git://github.com/wilkie/nice-ffi.git"
gem "ruby-sdl-ffi", :git => "git://github.com/wilkie/ruby-sdl-ffi.git"