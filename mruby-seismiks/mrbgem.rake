MRuby::Gem::Specification.new('mruby-seismiks') do |spec|
  spec.license = 'GPL'
  spec.author  = 'kum-kiel'
  spec.summary = 'Process Seismiks Files'

  spec.linker.libraries << 'dl'
  spec.linker.libraries << 'pthread'

  spec.add_dependency 'mruby-sample-buffer'

  # sql = File.open('seismiks_format_1.sql').read
  # File.open('seismiks_format_1.h', 'w') do |f|
  #   f.puts "static const char *seismiks_format_1_sql = #{sql.inspect};"
  #   f.puts "static const int seismiks_format_1_sql_size = #{sql.length};"
  # end
end
