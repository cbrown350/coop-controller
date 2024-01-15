python
import sys
sys.path.insert(0, '/home/vscode/.platformio/packages/toolchain-xtensa-esp32/share/gcc-8.4.0/python')
from libstdcxx.v6.printers import register_libstdcxx_printers
register_libstdcxx_printers (None)
end
