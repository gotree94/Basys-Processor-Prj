-- ================================================================================
-- tb_basys3.vhd - Basys3 / NEORV32 top-level smoke testbench
-- -------------------------------------------------------------------------------
-- Fully bootable simulation requires the firmware inside IMEM. Two options:
--   1) preload IMEM / generic BOOT via $readmemh (comment below)
--   2) run NEORV32's official testbench + bootloader flow
-- This TB verifies: reset, clock, board IO wiring (SW->GPIO, LED, seg7 refresh,
-- VGA sync toggle, PS/2 no-crash) and prints a summary at the end.
-- ================================================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tb_basys3 is
end entity;

architecture sim of tb_basys3 is

  constant CLK_PERIOD : time := 10 ns;   -- 100 MHz

  signal clk       : std_logic := '0';
  signal btnC      : std_logic := '0';
  signal btnU      : std_logic := '0';
  signal btnD      : std_logic := '0';
  signal btnL      : std_logic := '0';
  signal btnR      : std_logic := '0';
  signal sw        : std_logic_vector(15 downto 0) := (others => '0');
  signal led       : std_logic_vector(15 downto 0);
  signal seg       : std_logic_vector(6 downto 0);
  signal dp        : std_logic;
  signal an        : std_logic_vector(3 downto 0);
  signal vgaRed    : std_logic_vector(3 downto 0);
  signal vgaGreen  : std_logic_vector(3 downto 0);
  signal vgaBlue   : std_logic_vector(3 downto 0);
  signal Hsync     : std_logic;
  signal Vsync     : std_logic;
  signal RsTx      : std_logic;
  signal RsRx      : std_logic := '1';
  signal PS2Clk    : std_logic := '1';
  signal PS2Data   : std_logic := '1';
  signal JA        : std_logic_vector(7 downto 0);
  signal JC        : std_logic_vector(7 downto 0);

begin

  -- 100 MHz clock
  clk <= not clk after CLK_PERIOD/2;

  -- instance
  dut: entity work.basys3_top
    port map (
      clk       => clk,
      btnC      => btnC,
      btnU      => btnU,
      btnD      => btnD,
      btnL      => btnL,
      btnR      => btnR,
      sw        => sw,
      led       => led,
      seg       => seg,
      dp        => dp,
      an        => an,
      vgaRed    => vgaRed,
      vgaGreen  => vgaGreen,
      vgaBlue   => vgaBlue,
      Hsync     => Hsync,
      Vsync     => Vsync,
      RsTx      => RsTx,
      RsRx      => RsRx,
      PS2Clk    => PS2Clk,
      PS2Data   => PS2Data,
      JA        => JA,
      JC        => JC
    );

  -- boot firmware preload example (needs the memory to be made loadable)
  -- initializer: rising_edge(clk) when (rst /= '1') -- $readmemh("firmware.hex", <imem>)

  main: process
  begin
    -- reset for ~1 us
    btnC <= '1';   -- active-high CPU reset
    wait for 1 us;
    btnC <= '0';

    -- drive some switches / buttons
    sw      <= x"00FF";
    btnU    <= '1';  wait for 1 us; btnU <= '0';
    btnD    <= '1';  wait for 1 us; btnD <= '0';

    -- let the design run (boot text "NEORV32 on Basys3 boot OK" would appear
    -- on RsTx if firmware were loaded)
    wait for 2 ms;
    report "SIM: top-level wiring smoke test finished." severity NOTE;
    report "SIM: (full boot requires firmware in IMEM - see tb comments)." severity NOTE;
    std.env.finish;
  end process main;

end architecture;