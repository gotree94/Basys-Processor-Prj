# ============================================================================
# create_project.tcl - NEORV32 / Basys3 Vivado 2022.2 project 생성
# ----------------------------------------------------------------------------
# 사용 예 (프로젝트 루트 NEORV32/ 에서):
#   vivado -mode batch -source scripts\create_project.tcl -tclargs ..\..\..\neorv32
#   (또는 환경변수 NEORV32_HOME 지정)
# ============================================================================

set part        "xc7a35tcpg236-1"
set top         "basys3_top"
set proj_dir    "./build"
set hdl_root    "./hdl"
set constr_root "./constrs"
set sim_root    "./sim"

# -- NEORV32 upstream RTL root (Tcl 인자 또는 NEORV32_HOME) --------------------
if {[llength $argv] > 0} {
  set neorv32_home [lindex $argv 0]
} elseif {[info exists ::env(NEORV32_HOME)]} {
  set neorv32_home $::env(NEORV32_HOME)
} else {
  puts "ERROR: specify NEORV32_HOME as first Tcl arg or set env NEORV32_HOME"
  exit 1
}
puts "NEORV32_HOME = $neorv32_home"

# -- project ------------------------------------------------------------------
create_project basys3_neorv32 $proj_dir -part $part -force
set_property target_language VHDL [current_project]
set_property top $top [current_fileset]

# -- NEORV32 SoC sources (library: neorv32) -----------------------------------
#  * upstream rtl/core/*.vhd 를 포함하되 neorv32_cfs.vhd 는 제외
#    (우리의 cfs_periph.vhd 가 대체 구현)
#  * 브랜치에 따라 디렉터리가 rtl/system 일 수 있음 (<-- 주석 참조)
set neorv32_rtl [glob -nocomplain "$neorv32_home/rtl/core/*.vhd"]
if {[llength $neorv32_rtl] == 0} {
  puts "ERROR: no RTL under \$NEORV32_HOME/rtl/core (check repo layout)"
  exit 1
}
add_files -fileset sources_1 -norecurse -library neorv32 $neorv32_rtl

set up_cfs [glob -nocomplain "$neorv32_home/rtl/core/neorv32_cfs.vhd"]
if {[llength $up_cfs] > 0} {
  remove_files -fileset sources_1 $up_cfs
}

# -- 사용자 정의 HDL -----------------------------------------------------------
#  * cfs_periph.vhd 는 neorv32 라이브러리로 컴파일 (neorv32_top 과 동일 라이브러리)
add_files -fileset sources_1 -norecurse -library neorv32 "$hdl_root/cfs_periph.vhd"
add_files -fileset sources_1 -norecurse \
          "$hdl_root/basys3_top.vhd" \
          "$hdl_root/fb_ram.vhd" \
          "$hdl_root/seg7_ctrl.vhd" \
          "$hdl_root/vga_ctrl.vhd" \
          "$hdl_root/ps2_rx.vhd"

# -- constraints ---------------------------------------------------------------
add_files -fileset constrs_1 -norecurse \
          "$constr_root/basys3.xdc"

# -- simulation -----------------------------------------------------------------
add_files -fileset sim_1 -norecurse "$sim_root/tb_basys3.vhd"
set_property top "tb_basys3" [get_filesets sim_1]

# -- implementation options (생략 가능) -----------------------------------------
set_property strategy Performance_Explore [get_runs impl_1]

puts "Project created: $proj_dir/basys3_neorv32.xpr"
puts "Next: vivado -mode batch -source scripts/build.tcl"
exit 0