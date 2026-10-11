-- ================================================================================
-- fb_ram.vhd - VGA framebuffer, true dual-port block RAM
-- -------------------------------------------------------------------------------
-- port A : CPU write (byte-enable) + read   -- shared word address
-- port B : VGA continuous read
-- size   : FB_WORDS x 32 bit  (default 4800 = 160x120x8bpp/4px)
-- ================================================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity fb_ram is
  generic (
    FB_WORDS : natural := 4800
  );
  port (
    clk_i      : in  std_ulogic;
    -- port A: CPU read/write (word address 0..FB_WORDS-1)
    a_addr_i   : in  std_ulogic_vector(12 downto 0);
    a_wen_i    : in  std_ulogic;                       -- write enable
    a_wbe_i    : in  std_ulogic_vector(3 downto 0);    -- byte enables
    a_wdata_i  : in  std_ulogic_vector(31 downto 0);
    a_rdata_o  : out std_ulogic_vector(31 downto 0);   -- sync read
    -- port B: VGA read (word address 0..FB_WORDS-1)
    b_addr_i   : in  std_ulogic_vector(12 downto 0);
    b_rdata_o  : out std_ulogic_vector(31 downto 0)    -- sync read
  );
end entity;

architecture rtl of fb_ram is

  type ram_t is array (0 to FB_WORDS-1) of std_ulogic_vector(31 downto 0);
  -- (주의: 합성 시 TDP block RAM 으로 매핑되도록 설정 상속)
  signal mem : ram_t := (others => (others => '0'));

begin

  port_a: process(clk_i)
  begin
    if rising_edge(clk_i) then
      a_rdata_o <= mem(to_integer(unsigned(a_addr_i)));       -- read
      if (a_wen_i = '1') then                                 -- write (byte mask)
        for i in 0 to 3 loop
          if (a_wbe_i(i) = '1') then
            mem(to_integer(unsigned(a_addr_i)))(i*8+7 downto i*8) <=
              a_wdata_i(i*8+7 downto i*8);
          end if;
        end loop;
      end if;
    end if;
  end process port_a;

  port_b: process(clk_i)
  begin
    if rising_edge(clk_i) then
      b_rdata_o <= mem(to_integer(unsigned(b_addr_i)));       -- read only
    end if;
  end process port_b;

end architecture;