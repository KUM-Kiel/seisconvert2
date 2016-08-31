MRuby::Gem::Specification.new('mruby-seismiks') do |spec|
  spec.license = 'GPL'
  spec.author  = 'kum-kiel'
  spec.summary = 'Process Seismiks Files'

  spec.cc.flags << '-DXXH_NAMESPACE=LZ4_'

  spec.linker.libraries << 'dl'
  spec.linker.libraries << 'pthread'

  spec.add_dependency 'mruby-sample-buffer'

  sql = "#{dir}/src/seismiks_format_1.sql"
  h = "#{build_dir}/src/seismiks_format_1.h"

  spec.cc.include_paths << "#{build_dir}/src"

  directory "#{build_dir}/src"
  file "#{build_dir}/src/seismiks.o" => ["#{dir}/src/seismiks.c", h]
  file h => [sql, "#{build_dir}/src"] do
    content = File.read(sql)
    File.open(h, 'w') do |f|
      f.puts "static const char *seismiks_format_1_sql = #{content.inspect};"
      f.puts "static const int seismiks_format_1_sql_size = #{content.length};"
    end
  end
end
