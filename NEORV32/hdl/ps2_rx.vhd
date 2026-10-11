-- ================================================================================
-- ps2_rx.vhd - PS/2 keyboard host receiver (scancode)
-- -------------------------------------------------------------------------------
-- Samples PS2Data on the falling edge of PS2Clk. Captures 11 bits
-- (start + 8 data + parity + stop); parity is ignored for this demo.
-- rx_valid_o pulses for one clock when a new scancode has been received.
-- ================================================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity ps2_rx is
  port (
    clk_i      : in  std_ulogic;
    rstn_i     : in  std_ulogic;
    ps2_clk_i  : in  std_ulogic;               -- raw PS/2 clock (10-16 kHz)
    ps2_dat_i  : in  std_ulogic;               -- raw PS/2 data
    rx_data_o  : out std_ulogic_vector(7 downto 0);
    rx_valid_o : out std_ulogic                -- 1-clock pulse
  );
end entity;

architecture rtl of ps2_rx is

  -- synchronizers
  signal clk_s, dat_s   : std_ulogic;
  signal clk_s2, dat_s2 : std_ulogic;

  signal shift : std_ulogic_vector(10 downto 0);
  signal cnt   : unsigned(3 downto 0);         -- 0..10
  signal fall  : std_ulogic;

begin

  -- 2-stage synchronizer
  sync: process(clk_i, rstn_i)
  begin
    if (rstn_i = '0') then
      clk_s  <= '1';
      clk_s2 <= '1';
      dat_s  <= '1';
      dat_s2 <= '1';
    elsif rising_edge(clk_i) then
      clk_s  <= ps2_clk_i;
      clk_s2 <= clk_s;
      dat_s  <= ps2_dat_i;
      dat_s2 <= dat_s;
    end if;
  end process sync;

  fall <= '1' when (clk_s2 = '1') and (clk_s = '0') else '0';

  rx: process(clk_i, rstn_i)
  begin
    if (rstn_i = '0') then
      shift      <= (others => '0');
      cnt        <= (others => '0');
      rx_data_o  <= (others => '0');
      rx_valid_o <= '0';
    elsif rising_edge(clk_i) then
      rx_valid_o <= '0';
      -- sample on falling edge of PS/2 clock (start + 8 data + parity + stop)
      if (fall = '1') then
        shift <= dat_s2 & shift(10 downto 1);
        cnt   <= cnt + 1;
      end if;
      -- one cycle after the 11th bit (stop) -> check validity and output
      if (cnt = 11) then
        cnt <= (others => '0');
        if (shift(10) = '1') and (shift(0) = '0') then  -- stop='1', start='0'
          rx_data_o  <= shift(8 downto 1);
          rx_valid_o <= '1';
        end if;
      end if;
    end if;
  end process rx;

end architecture;