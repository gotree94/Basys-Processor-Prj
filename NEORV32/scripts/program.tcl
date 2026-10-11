# ============================================================================
# program.tcl - Basys3 보드에 비트스트림 프로그래밍
# ----------------------------------------------------------------------------
# vivado -mode batch -source scripts\program.tcl     (또는 GUI: Tools>Program)
# ============================================================================

open_hw_manager
connect_hw_server
open_hw_target    ;# USB-JTAG (필요시 [current_hw_target] 확인)

set bit ./bitstream/basys3.bit

if {[file exists $bit]} {
  set_property PROGRAM.FILE $bit [current_hw_device]
  program_hw_devices [current_hw_device]
  puts "DONE: programmed = $bit"
} else {
  puts "ERROR: $bit not found. Run build.tcl first."
  exit 1
}

exit 0