-- ================================================================================
-- vga_ctrl.vhd - VGA 640x480@60 controller with 160x120x8bpp framebuffer
-- -------------------------------------------------------------------------------
-- * 25 MHz pixel clock generated from 100 MHz (clock-enable every 4 clocks)
-- * framebuffer: FB_X x FB_Y bytes, 4 pixels per 32-bit word (little-endian)
--   -> word addr  = fb_y*40 + fb_x/4, byte = fb_x mod 4
-- * 1-pixel read pipeline: address issued on tick, word consumed next tick.
-- ================================================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity vga_ctrl is
  generic (
    FB_X     : natural := 160;
    FB_Y     : natural := 120;
    FB_WORDS : natural := 4800
  );
  port (
    clk_i      : in  std_ulogic;
    rstn_i     : in  std_ulogic;
    en_i       : in  std_ulogic;                    -- display enable
    fb_addr_o  : out std_ulogic_vector(12 downto 0);-- framebuffer word address
    fb_word_i  : in  std_ulogic_vector(31 downto 0);-- framebuffer word (read)
    r_o        : out std_ulogic_vector(3 downto 0);
    g_o        : out std_ulogic_vector(3 downto 0);
    b_o        : out std_ulogic_vector(3 downto 0);
    hsync_o    : out std_ulogic;
    vsync_o    : out std_ulogic;
    vblank_o   : out std_ulogic
  );
end entity;

architecture rtl of vga_ctrl is

  constant H_TOTAL : natural := 800;   -- 640 + 16 + 96 + 48
  constant V_TOTAL : natural := 525;   -- 480 + 10 + 2 + 33

  signal h_cnt : unsigned(9 downto 0);
  signal v_cnt : unsigned(9 downto 0);
  signal pix_div : unsigned(1 downto 0);
  signal tick : std_ulogic;

  -- pipeline: pixel whose word is being read
  signal hr : unsigned(9 downto 0);
  signal vr : unsigned(9 downto 0);
  signal en_r : std_ulogic;

begin

  -- 100MHz -> 25MHz clock enable
  divider: process(clk_i, rstn_i)
  begin
    if (rstn_i = '0') then
      pix_div <= (others => '0');
    elsif rising_edge(clk_i) then
      pix_div <= pix_div + 1;
    end if;
  end process divider;
  tick <= '1' when (pix_div = "11") else '0';

  -- timing + address issue
  timing: process(clk_i, rstn_i)
    variable wx : unsigned(7 downto 0);
    variable wy : unsigned(7 downto 0);
  begin
    if (rstn_i = '0') then
      h_cnt <= (others => '0');
      v_cnt <= (others => '0');
      hr    <= (others => '0');
      vr    <= (others => '0');
      en_r  <= '0';
      fb_addr_o <= (others => '0');
    elsif rising_edge(clk_i) then
      if (tick = '1') then
        hr   <= h_cnt;                  -- remember the pixel being read now
        vr   <= v_cnt;
        en_r <= '1' when ((en_i = '1') and (h_cnt < 640) and (v_cnt < 480)) else '0';

        -- framebuffer word address (only valid inside visible area)
        wx := h_cnt(9 downto 2);        -- 0..199 (visible 0..159)
        wy := v_cnt(9 downto 2);        -- 0..131 (visible 0..119)
        if (h_cnt < 640) and (v_cnt < 480) then
          fb_addr_o <= std_ulogic_vector(
            to_unsigned(to_integer(wy) * (FB_X/4) + to_integer(wx(7 downto 2)), 13));
        else
          fb_addr_o <= (others => '0');
        end if;

        -- advance counters
        if (h_cnt = (H_TOTAL-1)) then
          h_cnt <= (others => '0');
          if (v_cnt = (V_TOTAL-1)) then v_cnt <= (others => '0');
          else v_cnt <= v_cnt + 1; end if;
        else
          h_cnt <= h_cnt + 1;
        end if;
      end if;
    end if;
  end process timing;

  -- output stage (consumes the word read during the previous tick)
  output: process(clk_i, rstn_i)
    variable bi : integer range 0 to 3;
    variable px : std_ulogic_vector(7 downto 0);
  begin
    if (rstn_i = '0') then
      r_o <= (others => '0');
      g_o <= (others => '0');
      b_o <= (others => '0');
      hsync_o <= '1';
      vsync_o <= '1';
      vblank_o <= '1';
    elsif rising_edge(clk_i) then
      if (tick = '1') then
        hsync_o <= '0' when (hr >= 656) and (hr <= 751) else '1';
        vsync_o <= '0' when (vr >= 490) and (vr <= 491) else '1';
        vblank_o <= '0' when (hr < 640) and (vr < 480) else '1';

        if (en_r = '1') and (hr < 640) and (vr < 480) then
          bi := to_integer(hr(1 downto 0));            -- byte within word
          px := fb_word_i(bi*8+7 downto bi*8);
          r_o <= px(7) & px(7) & px(6) & px(6);
          g_o <= px(5) & px(5) & px(4) & px(4);
          b_o <= px(3) & px(3) & px(2) & px(2);
        else
          r_o <= (others => '0');
          g_o <= (others => '0');
          b_o <= (others => '0');
        end if;
      end if;
    end if;
  end process output;

end architecture;