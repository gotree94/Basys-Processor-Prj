# ============================================================================
# build.tcl - 합성/구현/비트스트림 생성
# ----------------------------------------------------------------------------
# vivado -mode batch -source scripts\build.tcl
# ============================================================================

set proj_file ./build/basys3_neorv32.xpr

if {[file exists $proj_file]} {
  open_project $proj_file
} else {
  puts "ERROR: project not found. Run create_project.tcl first."
  exit 1
}

# launch synthesis + implementation + bitstream
launch_runs synth_1 -jobs 8
wait_on_run synth_1

if {[get_property STATUS [get_runs synth_1]] != "synth_design Complete"} {
  puts "ERROR: synthesis failed."
  exit 1
}

launch_runs impl_1 -to_step write_bitstream -jobs 8
wait_on_run impl_1

if {[get_property STATUS [get_runs impl_1]] != "write_bitstream Complete"} {
  puts "ERROR: implementation/bitstream failed."
  exit 1
}

# copy artifact
file mkdir ./bitstream
file copy -force ./build/basys3_neorv32.runs/impl_1/basys3_top.bit \
            ./bitstream/basys3.bit

puts "DONE: bitstream at ./bitstream/basys3.bit"
puts "Next: vivado -mode batch -source scripts/program.tcl"
exit 0