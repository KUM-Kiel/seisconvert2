MRuby::Gem::Specification.new('mruby-bin-makezft') do |spec|
  spec.license = 'GPL'
  spec.author  = 'kum-kiel'
  spec.bins    = ['makezft']

  spec.add_dependency 'mruby-zft'
  spec.add_dependency 'mruby-kumsd'
end
