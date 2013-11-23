import difflib

class trace_step(gdb.Command):
  """Execute a single assembly instruction and dump the registers"""

  def __init__(self):
    gdb.Command.__init__(self, "trace_step", gdb.COMMAND_OBSCURE)

  def invoke(self, arg, from_tty):
    gdb.execute("si")
    foo1 = gdb.execute("info registers all", False, True)
    for i in range(10):
      gdb.execute("si", False, True)
      foo2 = gdb.execute("info registers all", False, True)
      x1 = foo1.split("\n")
      x2 = foo2.split("\n")
      # print foo1
      # print foo2
      print "--"
      for a in set(x2).difference(x1):
        print a

      foo1 = foo2

trace_step()

# Sample use
# gdb foo
# source gdbext.py
# b _init
# trace_step
#
