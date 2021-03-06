MRuby::Build.new do |conf|
  # load specific toolchain settings

  # Gets set by the VS command prompts.
  if ENV['VisualStudioVersion'] || ENV['VSINSTALLDIR']
    toolchain :visualcpp
  else
    toolchain :gcc
  end

  enable_debug

  # include the default GEMs
  conf.gembox 'default'
  conf.gem 'mruby-sample-buffer'
  conf.gem 'mruby-kumsd'
  conf.gem 'mruby-seed'
  conf.gem 'mruby-seismiks'
  conf.gem 'mruby-zft'
  #conf.gem '../mruby-hexdump'
  conf.gem 'mruby-bin-makeseed'
  conf.gem 'mruby-bin-makezft'
  conf.gem 'mruby-mkdir'
  conf.gem 'mruby-setuid'
  conf.gem github: 'iij/mruby-io'
  conf.gem github: 'mattn/mruby-onig-regexp'
end
