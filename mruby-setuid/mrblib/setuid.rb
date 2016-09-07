module Kernel
  def open_unprivileged name, mode
    e = geteuid
    f = nil
    seteuid getuid
    begin
      f = File.open name, mode
    ensure
      seteuid e
    end
    f
  end

  def mkdir_p_unprivileged dir
    e = geteuid
    seteuid getuid
    begin
      mkdir_p dir
    ensure
      seteuid e
    end
    nil
  end
end
