-- ================================================================================
-- seg7_ctrl.vhd - 4-digit 7-segment controller (Basys3, common anode, active low)
-- -------------------------------------------------------------------------------
-- 100 MHz clk, ~3 kHz digit refresh. 4-bit HEX decoder (active-low segments).
-- raw mode bypasses the decoder (raw a..g,dp duplicated to all digits).
-- ================================================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity seg7_ctrl is
  port (
    clk_i     : in  std_ulogic;
    rstn_i    : in  std_ulogic;
    digits_i  : in  std_ulogic_vector(15 downto 0);  -- 4 x 4-bit hex
    raw_i     : in  std_ulogic_vector(7 downto 0);   -- raw pattern (a..g, dp)
    raw_en_i  : in  std_ulogic;
    cat_o     : out std_ulogic_vector(6 downto 0);   -- a..g, active low
    dp_o      : out std_ulogic;                      -- active low
    an_o      : out std_ulogic_vector(3 downto 0)    -- active low
  );
end entity;

architecture rtl of seg7_ctrl is

  signal refresh_cnt : unsigned(14 downto 0);
  signal digit_sel   : natural range 0 to 3 := 0;
  signal nibble      : std_ulogic_vector(3 downto 0);

begin

  -- refresh counter (~3 kHz per digit)
  refresh: process(clk_i, rstn_i)
  begin
    if (rstn_i = '0') then
      refresh_cnt <= (others => '0');
      digit_sel   <= 0;
    elsif rising_edge(clk_i) then
      refresh_cnt <= refresh_cnt + 1;
      if (refresh_cnt = "111111111111111") then  -- wrap -> next digit
        if (digit_sel = 3) then digit_sel <= 0; else digit_sel <= digit_sel + 1; end if;
      end if;
    end if;
  end process refresh;

  -- digit select
  with to_unsigned(digit_sel,2) select
    nibble <= digits_i(3 downto 0)   when "00",
              digits_i(7 downto 4)   when "01",
              digits_i(11 downto 8)  when "10",
              digits_i(15 downto 12) when others;

  an_o <= "1110" when (digit_sel = 0) else
          "1101" when (digit_sel = 1) else
          "1011" when (digit_sel = 2) else
          "0111";

  -- 7-segment decode (active low) + raw bypass
  cat_o <= raw_i(6 downto 0) when (raw_en_i = '1') else
           "1000000" when (nibble = x"0") else
           "1111001" when (nibble = x"1") else
           "0100100" when (nibble = x"2") else
           "0110000" when (nibble = x"3") else
           "0011001" when (nibble = x"4") else
           "0010010" when (nibble = x"5") else
           "0000010" when (nibble = x"6") else
           "1111000" when (nibble = x"7") else
           "0000000" when (nibble = x"8") else
           "0010000" when (nibble = x"9") else
           "0001000" when (nibble = x"A") else
           "0000011" when (nibble = x"B") else
           "1000110" when (nibble = x"C") else
           "0100001" when (nibble = x"D") else
           "0000110" when (nibble = x"E") else
           "0001110";

  dp_o <= '0' when (raw_en_i = '1') and (raw_i(7) = '1') else '1';

end architecture;